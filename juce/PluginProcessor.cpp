#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace {
using P = juce::AudioParameterFloat;
// Literal IDs avoid transient std::string allocations in processBlock().
constexpr const char* graphTypeIds[] = {"gType0", "gType1", "gType2", "gType3", "gType4", "gType5", "gType6", "gType7"};
constexpr const char* graphAIds[] = {"gA0", "gA1", "gA2", "gA3", "gA4", "gA5", "gA6", "gA7", "gA8"};
constexpr const char* graphBIds[] = {"gB0", "gB1", "gB2", "gB3", "gB4", "gB5", "gB6", "gB7", "gB8"};
constexpr const char* graphLevelIds[] = {"gLevel0", "gLevel1", "gLevel2", "gLevel3", "gLevel4", "gLevel5", "gLevel6", "gLevel7", "gLevel8"};
constexpr const char* graphColorIds[] = {"gColor0", "gColor1", "gColor2", "gColor3", "gColor4", "gColor5", "gColor6", "gColor7", "gColor8"};
constexpr const char* modSourceIds[] = {"modSource0", "modSource1"};
constexpr const char* modTargetIds[] = {"modTarget0", "modTarget1"};
constexpr const char* modDestIds[] = {"modDest0", "modDest1"};
constexpr const char* modDepthIds[] = {"modDepth0", "modDepth1"};
void addParameter(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& v,const juce::String&id,const juce::String&name,float a,float b,float def){
 v.push_back(std::make_unique<P>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(a,b),def));
}
}
juce::AudioProcessorValueTreeState::ParameterLayout PanoramaProcessor::createLayout(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
 v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"engine",1},"Synthesis Engine",juce::StringArray{"Crystal VA","Fractal Digital","Physical Resonance","Hybrid Cross-Synthesis","Modular Patchbay"},0));
 addParameter(v,"cutoff","Filter Cutoff",0.01f,1.f,.84f);
 addParameter(v,"resonance","Filter Resonance",0.f,1.f,.15f);
 addParameter(v,"attack","Attack (s)",.001f,3.f,.015f);
 addParameter(v,"decay","Decay (s)",.002f,3.f,.23f);
 addParameter(v,"sustain","Sustain",0.f,1.f,.76f);
 addParameter(v,"release","Release (s)",.01f,4.f,.48f);
 addParameter(v,"detune","Unison Detune",0.f,1.f,.29f);
 addParameter(v,"unison","Unison",0.f,1.f,.93f);
 addParameter(v,"fm","FM Index",0.f,1.f,.08f);
 addParameter(v,"fold","Fractal Fold",0.f,1.f,.12f);
 addParameter(v,"body","Resonator Body",0.f,1.f,.2f);
 addParameter(v,"damping","Damping",0.f,1.f,.42f);
 addParameter(v,"motion","Motion",0.f,1.f,.28f);
 addParameter(v,"blend","Cross-Synthesis Mix",0.f,1.f,.50f);
 addParameter(v,"brightness","Exciter Brightness",0.f,1.f,.86f);
 addParameter(v,"gain","Master",0.f,1.f,.51f);
 v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"routing",1},"Cross Routing",juce::StringArray{"Classic","VA to Resonator","FM to Resonator","Ring to Resonator to PM","Resonator to VA AM"},0));
 addParameter(v,"crossDepth","Routing Depth",0.f,1.f,0.f);
 addParameter(v,"stereoResonance","Modal Stereo Width",0.f,1.f,0.f);
 // v0.3: genuine audio-rate graph: nine nodes, each with editable type, inputs, and two controls.
 const juce::StringArray nodeTypes{"Off","VA","Digital FM","Noise","Ring","Mix","Wavefolder","Modal Resonator","Low-pass","Gain","Harmonic Additive","Phase Warp"};
 juce::StringArray inputChoices{"None"};
 for(int i=0;i<panorama::kGraphNodes;i++)inputChoices.add("Node " + juce::String(i+1));
 for(int i=0;i<panorama::kGraphNodes;i++){
  const auto sid=juce::String(i),label="Node " + juce::String(i+1);
  if(i<panorama::kGraphNodes-1)
   v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"gType"+sid,1},label+" Type",nodeTypes,0));
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"gA"+sid,1},label+" Input A",inputChoices,0));
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"gB"+sid,1},label+" Input B",inputChoices,0));
  addParameter(v,"gLevel"+sid,label+" Level",0.f,1.f,.8f);
  addParameter(v,"gColor"+sid,label+" Color",0.f,1.f,.5f);
 }
 // v0.4: two fully automatable, patch-serializable modulation lanes.
 juce::StringArray modSources{"Off","LFO A","LFO B","Envelope","Velocity","Mod Wheel"};
 for(int i=0;i<panorama::kGraphNodes;i++)modSources.add("Audio Node " + juce::String(i+1));
 juce::StringArray modTargets;
 for(int i=0;i<panorama::kGraphNodes;i++)modTargets.add("Node " + juce::String(i+1));
 juce::StringArray modDests{"Color","Level","Pitch","Phase"};
 for(int i=0;i<2;i++){
  const auto id=juce::String(i);
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modSource"+id,1},"Mod " + id + " Source",modSources,0));
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modTarget"+id,1},"Mod " + id + " Target",modTargets,0));
  v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modDest"+id,1},"Mod " + id + " Destination",modDests,0));
  addParameter(v,"modDepth"+id,"Mod " + id + " Depth",-1.f,1.f,0.f);
 }
 return {v.begin(),v.end()};
}
PanoramaProcessor::PanoramaProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),params(*this,nullptr,"PANORAMA",createLayout()) {loadPreset(0);}
void PanoramaProcessor::prepareToPlay(double sr,int){synth.prepare(sr);}
void PanoramaProcessor::releaseResources(){synth.allNotesOff();}
bool PanoramaProcessor::isBusesLayoutSupported(const BusesLayout&b) const {return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()||b.getMainOutputChannelSet()==juce::AudioChannelSet::mono();}
const juce::String PanoramaProcessor::getProgramName(int n){return n>=0&&n<panorama::kPresetCount?panorama::factoryPresets()[(size_t)n].name:juce::String();}
void PanoramaProcessor::loadPreset(int index){
 if(index<0||index>=panorama::kPresetCount)return;
 currentProgram.store(index);
 const auto&p=panorama::factoryPresets()[(size_t)index];
 auto write=[this](const char*id,float v){if(auto*pr=params.getParameter(id))pr->setValueNotifyingHost(pr->convertTo0to1(v));};
 write("engine",(float)(int)p.engine);
 write("cutoff",p.cutoff);write("resonance",p.resonance);write("attack",p.attack);write("decay",p.decay);
 write("sustain",p.sustain);write("release",p.release);write("detune",p.detune);write("unison",p.unison);
 write("fm",p.fm);write("fold",p.fold);write("body",p.resonanceBody);write("damping",p.damping);
 write("motion",p.motion);write("blend",p.blend);write("brightness",p.brightness);write("gain",p.gain);
 write("routing",(float)p.routing);write("crossDepth",p.crossDepth);write("stereoResonance",p.stereoResonance);
 for(int i=0;i<panorama::kGraphNodes;i++){
   const auto&n=p.graph.nodes[i];
   if(i<panorama::kGraphNodes-1)write(graphTypeIds[i],(float)((int)n.type>=11?(int)n.type-1:(int)n.type));
   write(graphAIds[i],(float)(n.a+1));write(graphBIds[i],(float)(n.b+1));
   write(graphLevelIds[i],n.level);write(graphColorIds[i],n.color);
 }
 for(int i=0;i<2;i++){
   const auto&m=p.modRoutes[(size_t)i];
   write(modSourceIds[i],(float)m.source);
   write(modTargetIds[i],(float)m.target);
   write(modDestIds[i],(float)m.destination);
   write(modDepthIds[i],m.depth);
 }
}
panorama::Patch PanoramaProcessor::getPatchFromParameters()const{
 const int index=juce::jlimit(0,panorama::kPresetCount-1,currentProgram.load());
 auto p=panorama::factoryPresets()[(size_t)index];
 auto get=[this](const char*id)->float{return params.getRawParameterValue(id)->load();};
 p.engine=static_cast<panorama::Engine>(juce::jlimit(0,4,(int)std::round(get("engine"))));
 p.cutoff=get("cutoff");p.resonance=get("resonance");p.attack=get("attack");p.decay=get("decay");
 p.sustain=get("sustain");p.release=get("release");p.detune=get("detune");p.unison=get("unison");
 p.fm=get("fm");p.fold=get("fold");p.resonanceBody=get("body");p.damping=get("damping");
 p.motion=get("motion");p.blend=get("blend");p.brightness=get("brightness");p.gain=get("gain");
 p.routing=juce::jlimit(0,4,(int)std::round(get("routing")));
 p.crossDepth=get("crossDepth");p.stereoResonance=get("stereoResonance");
 for(int i=0;i<panorama::kGraphNodes;i++){
   auto&n=p.graph.nodes[i];
   // Node 9 is a fixed Output and intentionally has no gType8 parameter.
   if(i<panorama::kGraphNodes-1){
     const int uiType=juce::jlimit(0,11,(int)std::round(get(graphTypeIds[i])));
     n.type=static_cast<panorama::NodeType>(uiType>=10?uiType+1:uiType);
   }else n.type=panorama::NodeType::Output;
   n.a=juce::jlimit(-1,panorama::kGraphNodes-1,(int)std::round(get(graphAIds[i]))-1);
   n.b=juce::jlimit(-1,panorama::kGraphNodes-1,(int)std::round(get(graphBIds[i]))-1);
   n.level=get(graphLevelIds[i]);n.color=get(graphColorIds[i]);
 }
 for(int i=0;i<2;i++){
   auto&m=p.modRoutes[(size_t)i];
   m.source=juce::jlimit(0,5+panorama::kGraphNodes,(int)std::round(get(modSourceIds[i])));
   m.target=juce::jlimit(0,panorama::kGraphNodes-1,(int)std::round(get(modTargetIds[i])));
   m.destination=juce::jlimit(0,3,(int)std::round(get(modDestIds[i])));
   m.depth=get(modDepthIds[i]);
 }
 return p;
}
void PanoramaProcessor::handleMidi(const juce::MidiMessage& m){
 if(m.isNoteOn())synth.noteOn(m.getNoteNumber(),m.getFloatVelocity(),m.getChannel());
 else if(m.isNoteOff())synth.noteOff(m.getNoteNumber(),m.getChannel());
 else if(m.isAllNotesOff()||m.isAllSoundOff())synth.allNotesOff();
 else if(m.isPitchWheel())synth.pitchBend(2.f*(m.getPitchWheelValue()-8192)/8192.f);
 else if(m.isController()){
  if(m.getControllerNumber()==64)synth.sustain(m.getControllerValue()>=64);
  if(m.getControllerNumber()==1)synth.modWheel(m.getControllerValue()/127.f);
 }
}
void PanoramaProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals guard;
 const int count=audio.getNumSamples();
 if(audio.getNumChannels()<1)return;
 // update patch once per processing block; no allocations on audio callback
 synth.setPatch(getPatchFromParameters());
 float* l=audio.getWritePointer(0);float* r=audio.getNumChannels()>1?audio.getWritePointer(1):nullptr;
 juce::MidiBuffer::Iterator it(midi);
 juce::MidiMessage next;
 int nextAt=0;
 bool pending=it.getNextEvent(next,nextAt);
 for(int i=0;i<count;i++){
  while(pending&&nextAt<=i){handleMidi(next);pending=it.getNextEvent(next,nextAt);}
  const auto s=synth.processOne();l[i]=s.left;if(r)r[i]=s.right;
 }
 for(int c=2;c<audio.getNumChannels();c++)audio.clear(c,0,count);
}
void PanoramaProcessor::getStateInformation(juce::MemoryBlock& b){
 auto copy=params.copyState();copy.setProperty("factoryProgram",currentProgram.load(),nullptr);
 auto xml=copy.createXml();copyXmlToBinary(*xml,b);
}
void PanoramaProcessor::setStateInformation(const void* data,int size){
 auto xml=getXmlFromBinary(data,size);
 if(xml && xml->hasTagName(params.state.getType().toString())){
  const int program=juce::jlimit(0,panorama::kPresetCount-1,(int)xml->getIntAttribute("factoryProgram",0));
  params.replaceState(juce::ValueTree::fromXml(*xml));currentProgram.store(program);
 }
}
juce::AudioProcessorEditor* PanoramaProcessor::createEditor(){return new PanoramaEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new PanoramaProcessor();}
