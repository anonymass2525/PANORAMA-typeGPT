#include "PanoramaDSP.h"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG // run all assertions even in Release builds
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>
using namespace panorama;
static std::vector<float> take(int preset,int note=60){
 Synth s;s.prepare(48000);s.setPatch(factoryPresets()[preset]);s.noteOn(note,.85f);
 std::vector<float> a(24000),b(24000);
 s.render(a.data(),b.data(),(int)a.size());
 s.noteOff(note);return a;
}
static double rms(const std::vector<float>&a){double s=0;for(float v:a){assert(std::isfinite(v));s+=double(v)*v;}return std::sqrt(s/a.size());}
int main(){
 Synth s;s.prepare(48000);assert(s.activeVoiceCount()==0);assert(kPresetCount==32);
 std::vector<std::vector<float>> captures;
 for(int i=0;i<kPresetCount;i++){
  auto x=take(i);const auto r=rms(x);std::cout<<i+1<<" "<<factoryPresets()[i].name<<" rms="<<r<<"\n";
  assert(r>.00015 && r<.95);
  captures.push_back(std::move(x));
 }
 // Every adjacent preset must differ as produced signals, not only in naming.
 for(int i=1;i<kPresetCount;i++){
  double mse=0;for(size_t j=0;j<captures[i].size();j++){const double d=captures[i][j]-captures[i-1][j];mse+=d*d;}
  mse/=captures[i].size(); assert(mse>1e-8);
 }
 s.setPatch(factoryPresets()[0]);
 s.noteOn(60,1);s.noteOn(64,1);s.noteOn(67,1);assert(s.activeVoiceCount()==3);
 s.allNotesOff();std::vector<float>l(48000*6),r(l.size());s.render(l.data(),r.data(),(int)l.size());
 assert(s.activeVoiceCount()==0);
 // event sample-accuracy: no sound before start note.
 s.prepare(48000);s.setPatch(factoryPresets()[0]);
 std::vector<float>a(8000),b(8000);
 MidiEvent es[]={ {MidiEvent::Type::NoteOn,1000,60,1,1},{MidiEvent::Type::NoteOff,4000,60,1,0} };
 s.render(a.data(),b.data(),8000,es,2);
 for(int i=0;i<1000;i++)assert(a[i]==0&&b[i]==0);
 assert(rms(a)>.0001);
 // effect of physical -> direct generated sound; hybrid should be different from other three.
 for(int i=0;i<4;i++)assert(rms(captures[i*3])>.00015);
 // Explicit cross-synthesis ablation: changing physical contribution must change actual samples.
 auto hybrid=factoryPresets()[10];
 hybrid.engine=Engine::Hybrid;
 Synth dry;dry.prepare(48000);hybrid.blend=0;dry.setPatch(hybrid);dry.noteOn(60,.8f);
 Synth wet;wet.prepare(48000);hybrid.blend=1;wet.setPatch(hybrid);wet.noteOn(60,.8f);
 double crossDifference=0;
 for(int i=0;i<4096;i++){
  const auto a=dry.processOne(),b=wet.processOne();
  crossDifference+=(a.left-b.left)*(a.left-b.left);
 }
 assert(crossDifference/4096.0>.000001);
 // v0.2: a change of cross-routing topology must alter audible PCM.
 // Also prove crossDepth is not merely a visual parameter.
 auto renderPatch=[](Patch p){ Synth synth;synth.prepare(48000);synth.setPatch(p);
  synth.noteOn(60,.9f);std::vector<float>v(12000);
  for(auto &sample:v)sample=synth.processOne().left;
  return v;};
 auto compare=[](const auto&a,const auto&b){double mse=0.;
  for(size_t i=0;i<a.size();i++){const double d=a[i]-b[i];mse+=d*d;}
  return mse/a.size();};
 auto baseline=factoryPresets()[12]; baseline.routing=0;
 const auto classic=renderPatch(baseline);
 for(int route=1;route<=4;route++){
   baseline.routing=route;baseline.crossDepth=.9f;baseline.stereoResonance=.83f;
   auto audio=renderPatch(baseline);
   assert(rms(audio)>.0001);
   assert(compare(classic,audio)>1.e-6);
   auto zeroDepth=baseline;zeroDepth.crossDepth=0;
   assert(compare(audio,renderPatch(zeroDepth))>1.e-8);
 }
 // Ensure changing routed stereo width affects channels for identical notes.
 auto wide=factoryPresets()[15];wide.routing=4;wide.stereoResonance=1;
 Synth wideSynth;wideSynth.prepare(48000);wideSynth.setPatch(wide);wideSynth.noteOn(58,.9f);
 double stereoDiff=0;
 for(int i=0;i<12000;i++){auto sm=wideSynth.processOne();
  assert(std::isfinite(sm.left)&&std::isfinite(sm.right));
  stereoDiff+=double(sm.left-sm.right)*(sm.left-sm.right);}
 assert(stereoDiff/12000>1.e-9);
 // polyphony ceiling + finite data at several sample rates
 for(int rate:{44100,48000,96000}){
  s.prepare(rate);s.setPatch(factoryPresets()[10]);for(int n=36;n<70;n++)s.noteOn(n,.8f);
  assert(s.activeVoiceCount()==kMaxVoices);
  std::vector<float>x(16384),y(16384);
  s.render(x.data(),y.data(),(int)x.size());assert(rms(x)>.0001);assert(rms(y)>.0001);
 }
 // v0.3 genuine graph tests: compiled order, edit effect, invalid cycles, state reset.
 for(int i=16;i<kPresetCount;i++){
   const auto patch=factoryPresets()[i];
   assert(patch.engine==Engine::Modular);
   std::array<int,kGraphNodes> order{};
   assert(compileGraph(patch.graph,order));
   assert(order[kGraphNodes-1]==8); // All factory graphs have the fixed OUT as final sink.
   const auto audio=renderPatch(patch);
   assert(rms(audio)>.001 && rms(audio)<.98);
   // Routing changes must alter generated PCM without replacing the engine.
   auto edited=patch;
   edited.graph.nodes[8].a=0;
   edited.graph.nodes[8].b=1;
   edited.graph.nodes[8].color=.66f;
   const auto different=renderPatch(edited);
   assert(compare(audio,different)>1e-7);
   // Knobs modify audio-rate processing for a connected source.
   auto colorEdited=patch;
   colorEdited.graph.nodes[8].level=.10f;
   assert(compare(audio,renderPatch(colorEdited))>1e-8);
 }
 // Out-of-index order is supported by real topological sorting.
 {
   auto patch=factoryPresets()[16];
   patch.graph.nodes[0].type=NodeType::Gain;
   patch.graph.nodes[0].a=7;
   patch.graph.nodes[7].type=NodeType::VA;
   patch.graph.nodes[7].a=-1;patch.graph.nodes[7].b=-1;
   patch.graph.nodes[8].a=0;patch.graph.nodes[8].b=-1;
   std::array<int,kGraphNodes> order{};
   assert(compileGraph(patch.graph,order));
   assert(std::find(order.begin(),order.end(),7)<std::find(order.begin(),order.end(),0));
   assert(rms(renderPatch(patch))>.0001);
 }
 {
   auto patch=factoryPresets()[16];
   patch.graph.nodes[0].a=4;
   patch.graph.nodes[4].a=0;
   std::array<int,kGraphNodes> order{};
   assert(!compileGraph(patch.graph,order));
   Synth noCycle;noCycle.prepare(48000);noCycle.setPatch(patch);assert(!noCycle.graphValid());
   noCycle.noteOn(60,1.f);
   for(int i=0;i<256;i++){const auto z=noCycle.processOne();assert(z.left==0.f&&z.right==0.f);}
   patch.graph.nodes[0].a=-1;
   noCycle.setPatch(patch);assert(noCycle.graphValid());
   noCycle.noteOn(60,1.f);
   double energy=0;
   for(int i=0;i<4096;i++){const auto z=noCycle.processOne();assert(std::isfinite(z.left)&&std::isfinite(z.right));energy+=z.left*z.left;}
   assert(energy>1e-5);
 }
 // Note sample-scheduling and multivoice stability also hold for modular patches.
 for(int rate:{44100,48000,96000}){
   Synth synth;synth.prepare(rate);synth.setPatch(factoryPresets()[23]);
   for(int n=40;n<76;n++)synth.noteOn(n,.92f);
   assert(synth.activeVoiceCount()==kMaxVoices);
   for(int i=0;i<1500;i++){
     auto sample=synth.processOne();
     assert(std::isfinite(sample.left)&&std::isfinite(sample.right));
     assert(std::fabs(sample.left)<1.01 && std::fabs(sample.right)<1.01);
   }
 }
 // v0.4.2: prepare() must rederive filter coefficients and graph order.
 // The very same patch rendered after 48k -> 96k reconfiguration must match
 // a fresh 96k instance, even without an intervening setPatch() call.
 for(int idx:{18,23,24,29,31}){
   const auto patch=factoryPresets()[idx];
   Synth reused;reused.prepare(48000);reused.setPatch(patch);reused.prepare(96000);
   Synth fresh;fresh.prepare(96000);fresh.setPatch(patch);
   assert(reused.graphValid()&&fresh.graphValid());
   reused.noteOn(57,.9f);fresh.noteOn(57,.9f);
   for(int i=0;i<2048;i++){
     const auto a=reused.processOne(),b=fresh.processOne();
     assert(std::isfinite(a.left)&&std::isfinite(a.right));
     assert(a.left==b.left&&a.right==b.right);
   }
 }
 // v0.4: modulation influences actual PCM (LFO, envelope, note signal), not just UI.
 for(int idx=24;idx<kPresetCount;idx++){
   auto patch=factoryPresets()[idx];
   assert(validateModRoutes(patch.graph,patch.modRoutes));
   auto audio=renderPatch(patch);
   assert(rms(audio)>.0001 && rms(audio)<.98);
   auto ablated=patch;
   ablated.modRoutes[0].depth=0.f;
   assert(compare(audio,renderPatch(ablated))>1e-10);
   auto ablated2=patch;
   ablated2.modRoutes[1].depth=0.f;
   assert(compare(audio,renderPatch(ablated2))>1e-10);
 }
 // v0.4.1: an unconnected (zero-depth) audio modulation lane must never
 // silence a patch while a user edits source/target before raising its depth.
 {
   auto patch=factoryPresets()[24];
   patch.modRoutes[0]={6,0,0,0.f}; // node0->self, but disabled
   assert(validateModRoutes(patch.graph,patch.modRoutes));
   Synth synth;synth.prepare(48000);synth.setPatch(patch);
   assert(synth.graphValid());
   assert(rms(renderPatch(patch))>.0001);
   patch.modRoutes[0].depth=.7f;
   assert(!validateModRoutes(patch.graph,patch.modRoutes));
 }
 // The mod graph is independent of, but must remain acyclic with, the signal graph.
 {
  auto patch=factoryPresets()[24];
  patch.modRoutes[0]={6,0,0,.7f}; // node0 modulates itself
  assert(!validateModRoutes(patch.graph,patch.modRoutes));
  Synth invalid;invalid.prepare(48000);invalid.setPatch(patch);
  assert(!invalid.graphValid());
  invalid.noteOn(60,.8f);
  for(int n=0;n<512;n++){const auto x=invalid.processOne();assert(x.left==0.f&&x.right==0.f);}
  // If audio goes from node0 -> node3, node3 cannot audio-modulate node0.
  patch.modRoutes[0]={9,0,0,.6f};
  assert(!validateModRoutes(patch.graph,patch.modRoutes));
 }
 for(int rate:{44100,48000,96000}){
  for(int idx=24;idx<kPresetCount;idx++){
   Synth synth;synth.prepare(rate);synth.setPatch(factoryPresets()[idx]);assert(synth.graphValid());
   for(int n=48;n<65;n+=4)synth.noteOn(n,.88f);
   for(int j=0;j<2400;j++){
    const auto z=synth.processOne();
    assert(std::isfinite(z.left)&&std::isfinite(z.right));
    assert(std::abs(z.left)<1.02f&&std::abs(z.right)<1.02f);
   }
  }
 }
 std::cout<<"PANORAMA DSP ALL TESTS PASS\n";
}
