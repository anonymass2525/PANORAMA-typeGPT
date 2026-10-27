// Host-independent JUCE integration smoke test for Windows.
// Runs with the same Processor/Editor implementation as the VST3 target.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

namespace {
int failures=0;
void require(bool cond,const char*message){
 if(!cond){std::cerr << "FAIL: " << message << "\n"; ++failures;}
}
double render(PanoramaProcessor& proc, int frames){
 juce::AudioBuffer<float> audio(2,frames);
 audio.clear();
 juce::MidiBuffer midi;
 midi.addEvent(juce::MidiMessage::noteOn(1,60,static_cast<juce::uint8>(100)),0);
 midi.addEvent(juce::MidiMessage::noteOff(1,60),std::max(1,frames-1));
 proc.processBlock(audio,midi);
 double energy=0;
 for(int ch=0;ch<2;ch++)
  for(int i=0;i<frames;i++){
   const float x=audio.getSample(ch,i);
   require(std::isfinite(x),"audio contains NaN / Inf");
   energy+=double(x)*x;
  }
 return energy;
}
}
int main(){
 juce::ScopedJuceInitialiser_GUI gui;
 // Construction must work even when an editor has not been created.
 PanoramaProcessor synth;
 require(synth.getNumPrograms()==panorama::kPresetCount,"expected 32 factory programs");
 require(synth.acceptsMidi() && synth.hasEditor(),"MIDI / editor capability");
 for(const int rate:{44100,48000,96000}){
  synth.prepareToPlay(rate,512);
  for(const int index:{0,16,24,31}){
   synth.loadPreset(index);
   require(synth.getCurrentProgram()==index,"factory program selection");
   const auto patch=synth.getPatchFromParameters();
   require(patch.engine==panorama::factoryPresets()[index].engine,"synthesis engine mapping");
   const double energy=render(synth,512);
   require(energy>1.e-12,"non-silent rendered factory preset");
   synth.releaseResources();
   synth.prepareToPlay(rate,512);
  }
 }
 // Reforged patch is deliberately generated from a Harmonic Additive source.
 synth.loadPreset(24);
 const auto reforged=synth.getPatchFromParameters();
 require(reforged.engine==panorama::Engine::Modular,"Midnight Geometry Reforged is modular");
 require(reforged.graph.nodes[0].type==panorama::NodeType::Harmonic,
         "Reforged uses harmonic generator");
 // Persist an edited host parameter, selected program, and modular structure.
 if(auto*gain=synth.params.getParameter("gain"))
  gain->setValueNotifyingHost(gain->convertTo0to1(0.31f));
 else require(false,"gain parameter is registered");
 juce::MemoryBlock state;
 synth.getStateInformation(state);
 require(state.getSize()>0,"serialization generates a state blob");
 PanoramaProcessor restored;
 restored.prepareToPlay(48000,512);
 restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
 const auto restoredPatch=restored.getPatchFromParameters();
 require(restored.getCurrentProgram()==24,"selected preset restored");
 require(std::abs(restoredPatch.gain-0.31f)<0.002f,"edited gain restored");
 require(restoredPatch.graph.nodes[0].type==panorama::NodeType::Harmonic,
         "modular routing restored");
 require(restoredPatch.modRoutes[0].source==reforged.modRoutes[0].source,
         "modulation lane source restored");
 require(render(restored,512)>1.e-12,"restored state produces audio");
 // Opening, resizing and destroying the actual editor (with attached sliders
 // and comboboxes) catches many construction/lifetime issues before pluginval.
 {
  std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
  require(editor!=nullptr,"editor creation");
  if(editor){
   editor->setSize(1130,900);
   editor->setSize(800,620);
   require(editor->getWidth()==800,"editor resizing");
   require(render(restored,512)>1.e-12,"audio while editor exists");
  }
 }
 std::cout<<"PANORAMA JUCE integration: "<<(failures==0?"PASS":"FAIL")
          <<" (failures="<<failures<<")\n";
 return failures==0?0:1;
}
