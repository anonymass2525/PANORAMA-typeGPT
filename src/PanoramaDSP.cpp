#include "PanoramaDSP.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace panorama {
namespace {
constexpr double pi=3.14159265358979323846, twoPi=2*pi;
inline double clamp(double x,double lo,double hi){return std::max(lo,std::min(hi,x));}
inline double fract(double x){return x-std::floor(x);}
inline double polyBlep(double phase,double dt){
 if(phase<dt){double t=phase/dt;return t+t-t*t-1;}
 if(phase>1-dt){double t=(phase-1)/dt;return t*t+t+t+1;}
 return 0.0;
}
inline double noteHz(int midi){return 440.0*std::pow(2.0,(midi-69)/12.0);}
inline double dbSmooth(double x){return std::tanh(x*1.25)*.85;}
inline uint32_t hashStep(uint32_t& state){state^=state<<13;state^=state>>17;state^=state<<5;return state;}
inline double randBi(uint32_t& state){return ((hashStep(state)&0xffffff)/8388608.0)-1.0;}
}
bool compileGraph(const GraphPatch& graph,std::array<int,kGraphNodes>& order) noexcept {
 // Any node may feed any other node. Output is fixed to node 9; cycles are forbidden.
 if(graph.nodes[kGraphNodes-1].type!=NodeType::Output)return false;
 std::array<int,kGraphNodes> inDegree{};
 for(int i=0;i<kGraphNodes;i++) {
   const auto &n=graph.nodes[i];
   if(static_cast<int>(n.type)<0||static_cast<int>(n.type)>12)return false;
   for(int source:{n.a,n.b}) {
     if(source< -1||source>=kGraphNodes)return false;
     if(source==i)return false;
     if(source>=0)inDegree[i]++;
   }
 }
 std::array<bool,kGraphNodes> visited{};
 for(int k=0;k<kGraphNodes;k++) {
   int chosen=-1;
   for(int i=0;i<kGraphNodes;i++)if(!visited[i]&&inDegree[i]==0){chosen=i;break;}
   if(chosen<0)return false;
   order[k]=chosen;visited[chosen]=true;
   for(int i=0;i<kGraphNodes;i++)if(!visited[i]){
     if(graph.nodes[i].a==chosen)inDegree[i]--;
     if(graph.nodes[i].b==chosen)inDegree[i]--;
   }
 }
 return true;
}
bool validateModRoutes(const GraphPatch& graph,const std::array<ModRoute,2>& routes) noexcept {
 std::array<int,kGraphNodes> audioOrder{};
 if(!compileGraph(graph,audioOrder))return false;
 // Audio modulation dependencies extend the DAG. Reject control edges that close a cycle,
 // including those involving audio edges and both modulation lanes.
 std::array<std::array<bool,kGraphNodes>,kGraphNodes> reachable{};
 for(int target=0;target<kGraphNodes;target++){
  for(int upstream:{graph.nodes[target].a,graph.nodes[target].b})
   if(upstream>=0)reachable[upstream][target]=true;
 }
 for(const auto&route:routes){
  if(route.source<0||route.source>5+kGraphNodes||route.target<0||route.target>=kGraphNodes||route.destination<0||route.destination>3)return false;
  if(!std::isfinite(route.depth)||route.depth<-1.f||route.depth>1.f)return false;
  // Muted modulation lanes carry no audio dependency; permit editing them freely.
  if(route.source>=6 && route.depth!=0.f){const int upstream=route.source-6;
   if(upstream==route.target)return false;
   reachable[upstream][route.target]=true;
  }
 }
 for(int via=0;via<kGraphNodes;via++)
  for(int from=0;from<kGraphNodes;from++)
   for(int to=0;to<kGraphNodes;to++)
    if(reachable[from][via]&&reachable[via][to])reachable[from][to]=true;
 for(int i=0;i<kGraphNodes;i++)if(reachable[i][i])return false;
 return true;
}
void Synth::Envelope::trigger(const Patch& p,double sr){
 stage=Stage::Attack;value=0;
 const auto alpha=[sr](double seconds){return 1.0-std::exp(-1.0/(sr*std::max(.001,seconds)));};
 coefA=alpha(p.attack);coefD=alpha(p.decay);coefR=alpha(p.release*.45);
}
void Synth::Envelope::release(){if(stage!=Stage::Off) stage=Stage::Release;}
double Synth::Envelope::step(const Patch& p,double sr){
 if(stage==Stage::Off)return 0;
 (void)sr;
 switch(stage){
 case Stage::Attack: value+=(1.0-value)*coefA;if(value>.999) {value=1;stage=Stage::Decay;}break;
 case Stage::Decay: value+=(p.sustain-value)*coefD;if(std::fabs(p.sustain-value)<.0005) stage=Stage::Sustain;break;
 case Stage::Sustain: value+=(p.sustain-value)*.005;break;
 case Stage::Release: value+=(0.0-value)*coefR;if(value<.00008){value=0;stage=Stage::Off;}break;
 case Stage::Off:break;
 }
 return value;
}
void Synth::Voice::start(int midi,float velocity,int ch,uint64_t count,double sr,const Patch& p){
 active=true;releasedByKey=false;note=midi;channel=ch;age=count;vel=clamp(velocity,0,1);baseHz=noteHz(midi);
 for(int i=0;i<5;i++)ph[i]=fract(double(i)*.174+double((midi*17)%59)/97.);
 modPhase=0;subphase=0;lfoPhase=0;filt1=filt2=lpPrev=feedback=smear=crossDelay=0;noiseEnv=1;random=0x9E3779B9u^(uint32_t(midi+5)*0x85ebca6bu)^(uint32_t(count)*3u);
 env.trigger(p,sr);
 graphLfo=graphLfoB=0;
 for(int node=0;node<kGraphNodes;node++){
   auto& st=graphStates[node];st=GraphState{};
   st.rng=random^(0x9e3779b9u*static_cast<uint32_t>(node+1));
   for(int j=0;j<3;j++)st.unisonPhase[j]=fract(ph[j]+node*.13);
   for(int j=0;j<8;j++){
     auto&m=st.modal[j];
     constexpr double ratios[]={1.,1.493,2.11,2.98,4.21,5.37,6.94,9.43};
     const auto&def=p.graph.nodes[node];
     double tuning=.64+def.color*.85;
     double hz=clamp(baseHz*ratios[j]*tuning,25.,sr*.44);
     m.cosw=2.*std::cos(twoPi*hz/sr);
     m.r=clamp(1.-(0.00018+j*.00011)*(1.5-.93*def.color)*48000./sr,.90,.99997);
     m.energy=(1./(1.+j*.46))*((j%3==0)?1.:-.83);
   }
 }
 noiseDecay=std::exp(-1.0/(sr*(.014+.060*(1-p.damping))));
 // Inharmonic partial ratios deliberately tuned away from strict harmonic series.
 constexpr double ratios[]={1.,1.499,2.14,2.997,4.01,5.53,7.21,10.9};
 for(int i=0;i<8;i++){
   auto &m=modes[i];m.y1=m.y2=0;
   double shift=1.+.13*double(p.motion)*std::sin(i*2.5+midi*.21);
   const double hz=clamp(baseHz*ratios[i]*shift,40.,sr*.43);
   double q=clamp(.997-((.00008+i*.00018)+p.damping*(.00025+i*.0007))*48000./sr,.90,.99993);
   m.r=q; m.cosw=2.*std::cos(twoPi*hz/sr);
   m.energy=(1./(1.+i*.8))*((i%3==0)?1.:-.74);
 }
}
double Synth::Voice::process(double sr,const Patch& p,double bendMultiplier,double baseCutoff,float wheel,Sample& stereo,
    const std::array<int,kGraphNodes>& order,const std::array<double,kGraphNodes>& filterCoeffs){
 if(!active)return 0;
 if(p.engine==Engine::Modular){ stereo=processGraph(sr,p,bendMultiplier,wheel,order,filterCoeffs);return .5*(stereo.left+stereo.right); }
 const double envVal=env.step(p,sr);
 if(!env.active()){active=false;return 0;}
 const double lfoRate=.11+2.3*p.motion + 3.5*wheel;
 lfoPhase=fract(lfoPhase+lfoRate/sr);
 double lfo=std::sin(twoPi*lfoPhase);
 const double freq=clamp(baseHz*bendMultiplier*(1.0+.00103972*p.motion*lfo),10.,sr*.43);
 const double dt=freq/sr;
 // Independent, band-limited VA oscillators; five unison oscillators per MIDI voice.
 double vaL=0,vaR=0;
 const int n=(p.unison>.34?5:3);
 for(int i=0;i<n;i++){
   double d=(i-(n-1)*.5)*p.detune*.011;
   const double di=clamp(dt*(1+d),.0000001,.48);
   double phase=ph[i];
   double saw=2*phase-1-polyBlep(phase,di);
   const double pulseWidth=.5+.16*std::sin(twoPi*(lfoPhase*.35+i*.067));
   double pulse=(phase<pulseWidth?1.:-1.)+polyBlep(phase,di)-polyBlep(fract(phase-pulseWidth),di);
   double osc=(.68*saw+.20*pulse+.12*std::sin(twoPi*phase));
   double pan=(i-(n-1)*.5)/((n-1)*.5);
   vaL+=osc*(.5-.22*pan);vaR+=osc*(.5+.22*pan);
   ph[i]=fract(phase+di);
 }
 vaL/=n*.5;vaR/=n*.5;
 const double va=.5*(vaL+vaR);
 // Nested modulator waveform -> phase modulation + iterative fold; bandwidth limited by 2x base.
 modPhase=fract(modPhase+clamp(dt*(1.50+2.5*p.fm),0.,.46));
 const double mod=std::sin(twoPi*modPhase)+.24*std::sin(twoPi*modPhase*2);
 // Routed feedback is explicitly delayed by one sample and hard-clamped.
 double phaseFeedback=(p.routing==3 && p.engine==Engine::Hybrid)
     ? crossDelay*(0.025+0.21*clamp(p.crossDepth,0.,1.))
     : feedback*.025;
 double carrier=std::sin(twoPi*(subphase+p.fm*.58*mod+phaseFeedback));
 double fractal=carrier;
 const int folds=1+(int)std::floor(p.fold*3.2);
 for(int i=0;i<folds;i++)fractal=std::sin((1.3+.42*p.fold)*fractal+(.22+.06*i)*carrier);
 // Soft blend preserves pitched fundamental and avoids excess inharmonic fold energy.
 const double fmOut=(.53*carrier+.47*fractal)*.94;
 subphase=fract(subphase+dt);
 // Modal excitation receives either impulse-noise, VA, algorithmic signal or their mixture.
 double trans=randBi(random)*noiseEnv;
 noiseEnv*=noiseDecay;
 const double drive=clamp(.57*va+.36*fmOut,-1.7,1.7);
 double excitation=trans*(.05+.44*p.brightness) + .09*drive;
 const int route=(p.engine==Engine::Hybrid)?std::clamp(p.routing,0,4):0;
 const double depth=clamp(p.crossDepth,0.,1.);
 const double rawRing=va*fmOut;
 // Audio-rate routing is real, not a post-mix selector. Only the exciter signal
 // changes here; all modal modes receive it *before* their recursive filter.
 if(route==1) excitation=trans*(.05+.44*p.brightness)*(.15+.6*(1-depth)) +
          (.09+.34*depth)*va;
 else if(route==2) excitation=trans*(.05+.44*p.brightness)*(.15+.6*(1-depth)) +
          (.09+.40*depth)*fmOut;
 else if(route==3) excitation=trans*(.05+.44*p.brightness)*(.3-.2*depth) +
          (.09+.66*depth)*rawRing;
 else if(route==4) excitation=trans*(.05+.44*p.brightness)*(.35-.15*depth) +
          (.09+.29*depth)*va;
 double modal=0, modalL=0, modalR=0;
 for(int i=0;i<8;i++){
  auto &m=modes[i];
  double y=m.r*m.cosw*m.y1-m.r*m.r*m.y2+excitation*m.energy*.09;
  m.y2=m.y1;m.y1=y;
  const double contribution=y/(1.+i*.23);
  modal+=contribution;
  // Alternate modal partials spatially; zero width retains the original mono sound.
  const double width=clamp(p.stereoResonance,0.,1.);
  const double pan=(i%2==0?1.:-1.)*(.32+.065*i)*width;
  modalL+=contribution*(1.+pan);
  modalR+=contribution*(1.-pan);
 }
 // nonlinear, controlled soft saturation; no unstable feedback in prototype
 modal=std::tanh(modal*.70);
 const double physical=std::tanh((modal+.18*drive)*(.75+1.4*p.resonanceBody));
 const double physicalL=std::tanh((modalL+.18*drive)*(.75+1.4*p.resonanceBody));
 const double physicalR=std::tanh((modalR+.18*drive)*(.75+1.4*p.resonanceBody));
 double outL=0,outR=0;
 switch(p.engine){
 case Engine::CrystalVA:outL=vaL;outR=vaR;break;
 case Engine::FractalDigital:outL=outR=fmOut;break;
 case Engine::PhysicalResonance:outL=outR=physical;break;
 case Engine::Modular: break; // Handled by processGraph() before the legacy switch.
 case Engine::Hybrid:{
  // Cross-synthesis: algorithmic + VA excitation drives resonators; resonant energy affects PM next sample.
  feedback=clamp(modal,-1.,1.);
  // Explicit one-sample state: previous modal output feeds the next sample's PM.
  crossDelay=feedback;
  if(route==0){
    const double x=(1-p.blend)*(.55*va+.45*fmOut)+p.blend*physical;
    outL=x+.10*(vaL-va);outR=x+.10*(vaR-va);
  }else if(route==4){
    // The resonator modulates the VA amplitude (audio-rate cross-synthesis).
    const double amL=vaL*(.70+depth*.60*physicalL);
    const double amR=vaR*(.70+depth*.60*physicalR);
    outL=(1-p.blend)*amL+p.blend*physicalL;
    outR=(1-p.blend)*amR+p.blend*physicalR;
  }else{
    // Modes are the central instrument; oscillator feed-through retains pitch clarity.
    const double direct=(route==1?va:(route==2?fmOut:rawRing));
    const double amt=(1-p.blend)*direct;
    outL=amt+p.blend*physicalL+.05*(vaL-va);
    outR=amt+p.blend*physicalR+.05*(vaR-va);
  }
  break;
 }
 }
 // Low-pass nonlinear TPT-style approximation via 2 one-poles with coefficient smoothing.
 double cf=clamp(baseCutoff+freq*(.25*p.brightness),30.,sr*.44);
 cf*=1.+.08*p.motion*lfo; cf=clamp(cf,30.,sr*.44);
 const double g=1-std::exp(-twoPi*cf/sr);
 auto lp=[&](double x){filt1+=(std::tanh(x*(1+.8*p.resonance)) - filt1)*g;
     filt2+=(filt1-filt2)*g;
     return filt2;};
 // one common filter preserves unique voice articulation; stereo differences preserved pre-filter
 double mono=lp(.5*(outL+outR));
 double spread=.5*(outL-outR)*(.6+p.unison*.5);
 double gain=(.15+.70*vel)*envVal*p.gain;
 stereo.left=float(dbSmooth((mono+spread)*gain));
 stereo.right=float(dbSmooth((mono-spread)*gain));
 return mono;
}
// v0.3 Modular: independent per-voice, per-node stereo DSP with no allocation or lock.
// Input dependency order is pre-compiled in Synth::setPatch. Graph state resets on note-on.
Sample Synth::Voice::processGraph(double sr,const Patch& p,double bendMultiplier,float wheel,
    const std::array<int,kGraphNodes>& order,const std::array<double,kGraphNodes>& filterCoeffs){
 const double amp=env.step(p,sr);
 if(!env.active()){active=false;return {};}
 const double freq=clamp(baseHz*bendMultiplier,10.,sr*.43);
 const double dt=freq/sr;
 graphLfo=fract(graphLfo+(0.11+p.motion*1.6+wheel*4.0)/sr);
 graphLfoB=fract(graphLfoB+(0.37+p.motion*2.9+wheel*.8)/sr);
 const double lfo=std::sin(twoPi*graphLfo);
 const double lfoB=std::sin(twoPi*graphLfoB);
 std::array<Sample,kGraphNodes> values{};
 const auto source=[&](int n)->Sample{return n<0?Sample{}:values[n];};
 for(int slot=0;slot<kGraphNodes;slot++){
   const int i=order[slot];
   const auto&node=p.graph.nodes[i];auto& st=graphStates[i];
   const auto a=source(node.a),b=source(node.b);
   // Modulation is evaluated once per sample from the graph's current sample.
   // Graph order additionally incorporates audio-rate modulation dependencies.
   double color=node.color,level=node.level,pitch=0.,phaseOffset=0.;
   for(const auto&route:p.modRoutes){
     if(route.source==0||route.target!=i||route.depth==0.f)continue;
     double value=0.;
     switch(route.source){
      case 1:value=lfo;break;
      case 2:value=lfoB;break;
      case 3:value=2.*amp-1.;break;
      case 4:value=2.*vel-1.;break;
      case 5:value=2.*wheel-1.;break;
      default:{const auto sample=source(route.source-6);value=clamp(.5*(sample.left+sample.right),-1.,1.);break;}
     }
     const double mod=route.depth*value;
     switch(route.destination){
      case 0:color=clamp(color+mod*.5,0.,1.);break;
      case 1:level=clamp(level+mod*.5,0.,1.);break;
      case 2:pitch+=mod*12.;break;
      case 3:phaseOffset+=mod*.22;break;
     }
   }
   const double oscillatorDt=(std::abs(pitch)<1.e-12)?dt:clamp(dt*std::exp2(clamp(pitch,-24.,24.)/12.),.0000001,.45);
   double l=0,r=0;
   switch(node.type){
   case NodeType::Off:break;
   case NodeType::VA:{
     const double rate=oscillatorDt*(.98+.13*color+.015*lfo*p.motion);
     for(int j=0;j<3;j++){
       const double detune=(j-1)*(.001+.023*p.detune)*p.unison;
       const double inc=clamp(rate*(1.+detune),.0000001,.46);
       const double ph=st.unisonPhase[j];
       const double saw=2*ph-1-polyBlep(ph,inc);
       double w=(.82*color+.10)*saw+(.84-.74*color)*std::sin(twoPi*(ph+phaseOffset));
       const double pan=(j-1)*(.08+.22*p.unison);
       l+=w*(1.-pan)/3.;r+=w*(1.+pan)/3.;
       st.unisonPhase[j]=fract(ph+inc);
     }
     break;
   }
   case NodeType::Digital:{
     const double mInc=clamp(oscillatorDt*(1.25+color*4.),.0000001,.45);
     const double mod=std::sin(twoPi*st.mod);
     const double inputMod=.5*(a.left+a.right);
     st.mod=fract(st.mod+mInc);
     const double wave=std::sin(twoPi*(st.phase+phaseOffset)+mod*(.3+3.8*color)+inputMod*(.2+2.*p.fm));
     st.phase=fract(st.phase+oscillatorDt);
     l=r=wave;break;
   }
   case NodeType::Noise:{
     // 0.0 means short impulse, 1.0 means almost sustained textured excitation.
     const double n=randBi(st.rng);
     const double decay=.92+.07999*color;
     st.noiseLevel*=decay;
     const double envelope=.25+.75*st.noiseLevel;
     l=r=n*envelope;break;
   }
   case NodeType::Ring:l=a.left*b.left*2.;r=a.right*b.right*2.;break;
   case NodeType::Mix:{
     const double mix=color;
     l=a.left*(1.-mix)+b.left*mix;
     r=a.right*(1.-mix)+b.right*mix;
     break;
   }
   case NodeType::Fold:{
     const double fold=1.1+4.5*color;
     l=std::sin(a.left*fold+(.25*color)*std::sin(2.3*a.left));
     r=std::sin(a.right*fold+(.25*color)*std::sin(2.3*a.right));
     break;
   }
   case NodeType::Modal:{
     const double excitationL=clamp(a.left,-2.,2.);
     const double excitationR=clamp(a.right,-2.,2.);
     // Modal bank shares oscillation modes between channels, but the individual
     // partials pan and resonate differently; zero-cost small stereo spread.
     for(int j=0;j<8;j++){
       auto&m=st.modal[j];
       const double x=.5*(excitationL+excitationR);
       double y=m.r*m.cosw*m.y1-m.r*m.r*m.y2 + x*m.energy*.019;
       m.y2=m.y1;m.y1=y;
       double pan=(j%2?-.23:.23)*(.3+.7*p.stereoResonance);
       l+=y*(1.-pan)/(1.+j*.3);r+=y*(1.+pan)/(1.+j*.3);
     }
     l=std::tanh(l*(.9+p.resonanceBody+color*.45));r=std::tanh(r*(.9+p.resonanceBody+color*.45));
     break;
   }
   case NodeType::LowPass:{
     const double c=clamp(filterCoeffs[i]*(.25+1.5*color),.00001,.99);
     st.filterL+=(a.left-st.filterL)*c;
     st.filterR+=(a.right-st.filterR)*c;
     l=st.filterL;r=st.filterR;break;
   }
   case NodeType::Gain:l=a.left;r=a.right;break;
   case NodeType::Harmonic:{
     // A sample-free additive oscillator: additive partials are dynamically tilted
     // towards high or low harmonics. Do not evaluate partials above 0.44*sr.
     double w=0.,norm=0.;
     const double hz=sr*oscillatorDt;
     // Interpolate two precomputed timbral spectral envelopes instead of
     // evaluating 12 expensive power functions for every sample.
     constexpr double rich[]={1.,.61557,.46346,.37893,.32413,.28529,.25613,.23326,.2148,.19953,.18665,.17562};
     constexpr double pure[]={1.,.15389,.0515,.02368,.01297,.00786,.00512,.00351,.00251,.00186,.00143,.00112};
     for(int harmonic=1;harmonic<=12&&harmonic*hz<.44*sr;harmonic++){
       const double weight=rich[harmonic-1]*(1.-color)+pure[harmonic-1]*color;
       w+=weight*std::sin(twoPi*(harmonic*(st.phase+phaseOffset)));
       norm+=weight;
     }
     const double wave=norm>0?w/norm:0.;
     l=wave*(1.-.12*p.unison);r=wave*(1.+.12*p.unison);
     st.phase=fract(st.phase+oscillatorDt);
     break;
   }
   case NodeType::PhaseWarp:{
     // Phase distortion using a continuous bend function. Extreme bends can alias;
     // limited distortion depth is intentional in this first prototype.
     const double phase=fract(st.phase+phaseOffset);
     const double bend=.18+.64*color;
     const double warped=(phase<bend)?(phase*.5/bend):(.5+(phase-bend)*.5/(1.-bend));
     const double wave=.72*std::sin(twoPi*warped)+.28*std::sin(twoPi*phase);
     l=r=wave;
     st.phase=fract(st.phase+oscillatorDt);
     break;
   }
   case NodeType::Output:{
     // Stereo output is a final two-input sum; Level controls total output.
     l=a.left + b.left*color;
     r=a.right + b.right*color;
     break;
   }
   }
   // Per-node gain is deliberately applied after each operation.
   values[i]={float(std::tanh(l*level*1.2)),float(std::tanh(r*level*1.2))};
 }
 const auto last=values[kGraphNodes-1];
 const double gain=(.15+.7*vel)*amp*p.gain;
 return {float(dbSmooth(last.left*gain)),float(dbSmooth(last.right*gain))};
}
Synth::Synth(){patch_=factoryPresets()[0];prepare(48000);}
void Synth::prepare(double rate){
 // Changing host sample rate must rebuild all rate-dependent graph and filter
 // coefficients even when the host does not re-send the current patch.
 // Reset voice state first; the patch and modulation wiring remain intact.
 sr_=clamp(rate,8000.,192000.);
 reset();
 setPatch(patch_);
}
void Synth::reset(){for(auto &v:voices_)v=Voice{};nextAge_=0;bend_=wheel_=0;bendMultiplier_=1.;sustain_=false;dcLX_=dcLY_=dcRX_=dcRY_=softL_=softR_=0;}
void Synth::setPatch(const Patch& p){
 patch_=p;patch_.cutoff=clamp(p.cutoff,.01,1);patch_.gain=clamp(p.gain,0,1);
 patch_.routing=std::clamp(p.routing,0,4);
 patch_.crossDepth=clamp(p.crossDepth,0,1);
 patch_.stereoResonance=clamp(p.stereoResonance,0,1);
 for(int i=0;i<kGraphNodes;i++) {
   auto &node=patch_.graph.nodes[i];
   node.level=float(clamp(node.level,0,1));node.color=float(clamp(node.color,0,1));
 }
 graphValid_=compileGraph(patch_.graph,graphOrder_);
 for(auto &m:patch_.modRoutes){
   m.source=std::clamp(m.source,0,5+kGraphNodes);
   m.target=std::clamp(m.target,0,kGraphNodes-1);
   m.destination=std::clamp(m.destination,0,3);
   m.depth=std::isfinite(m.depth)?float(clamp(m.depth,-1.,1.)):0.f;
 }
 graphValid_=graphValid_&&validateModRoutes(patch_.graph,patch_.modRoutes);
 // Sorting includes both audio and audio-rate modulation dependencies.
 // Re-run a deterministic Kahn sort of the combined dependency graph.
 if(graphValid_){
  std::array<bool,kGraphNodes> seen{};
  for(int slot=0;slot<kGraphNodes;slot++){
    int chosen=-1;
    for(int i=0;i<kGraphNodes;i++)if(!seen[i]){
      const auto &node=patch_.graph.nodes[i];
      bool ready=(node.a<0||seen[node.a])&&(node.b<0||seen[node.b]);
      for(const auto&m:patch_.modRoutes)if(m.source>=6&&m.depth!=0.f&&m.target==i&&!seen[m.source-6])ready=false;
      if(ready){chosen=i;break;}
    }
    if(chosen<0){graphValid_=false;break;}
    graphOrder_[slot]=chosen;seen[chosen]=true;
  }
 }
 cutoffHz_=80.0*std::pow((sr_*.42)/80.,patch_.cutoff);
 for(int i=0;i<kGraphNodes;i++){
   auto& node=patch_.graph.nodes[i];
   double freq=clamp(cutoffHz_*(.09+2.5*node.color),40.,sr_*.43);
   graphFilterCoeffs_[i]=1.-std::exp(-twoPi*freq/sr_);
 }
}
void Synth::noteOn(int note,float velocity,int channel){
 if(note<0||note>127)return;
 if(velocity<=0){noteOff(note,channel);return;}
 Voice* candidate=nullptr;
 for(auto &v:voices_)if(!v.active){candidate=&v;break;}
 if(!candidate)candidate=&*std::min_element(voices_.begin(),voices_.end(),[](const Voice&a,const Voice&b){return a.age<b.age;});
 candidate->start(note,velocity,channel,++nextAge_,sr_,patch_);
}
void Synth::noteOff(int note,int channel){for(auto &v:voices_)if(v.active&&v.note==note&&v.channel==channel&&!v.releasedByKey){v.releasedByKey=true;if(!sustain_)v.env.release();}}
void Synth::sustain(bool down){if(sustain_&&!down)for(auto &v:voices_)if(v.active&&v.releasedByKey)v.env.release();sustain_=down;}
void Synth::allNotesOff(){sustain_=false;for(auto&v:voices_)v.env.release();}
void Synth::pitchBend(float semitones){bend_=float(clamp(semitones,-24,24));bendMultiplier_=std::exp2(bend_/12.0);}
void Synth::modWheel(float w){wheel_=float(clamp(w,0,1));}
int Synth::activeVoiceCount()const{int n=0;for(const auto&v:voices_)if(v.active)n++;return n;}
Sample Synth::processOne(){
 if(patch_.engine==Engine::Modular&&!graphValid_)return {};
 double l=0,r=0;
 for(auto &v:voices_)if(v.active){Sample s;v.process(sr_,patch_,bendMultiplier_,cutoffHz_,wheel_,s,graphOrder_,graphFilterCoeffs_);l+=s.left;r+=s.right;}
 // harmless voice headroom, smooth clip and DC blocker.
 l=std::tanh(l*.8);r=std::tanh(r*.8);
 double outL=l-dcLX_+.995*dcLY_;dcLX_=l;dcLY_=outL;
 double outR=r-dcRX_+.995*dcRY_;dcRX_=r;dcRY_=outR;
 return {float(outL),float(outR)};
}
void Synth::render(float* left,float* right,int n,const MidiEvent* events,size_t eventCount){
 if(n<=0||!left||!right)return;
 size_t ei=0;
 for(int i=0;i<n;i++){
  while(events&&ei<eventCount&&events[ei].sample<=i){const auto&e=events[ei++];
   switch(e.type){
   case MidiEvent::Type::NoteOn:noteOn(e.note,e.value,e.channel);break;
   case MidiEvent::Type::NoteOff:noteOff(e.note,e.channel);break;
   case MidiEvent::Type::PitchBend:pitchBend(e.value);break;
   case MidiEvent::Type::Sustain:sustain(e.value>=.5);break;
   case MidiEvent::Type::ModWheel:modWheel(e.value);break;
   case MidiEvent::Type::AllNotesOff:allNotesOff();break;
   }
  }
  const Sample s=processOne();left[i]=s.left;right[i]=s.right;
 }
}
}
