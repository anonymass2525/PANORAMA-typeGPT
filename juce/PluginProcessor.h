#pragma once
#include <JuceHeader.h>
#include "PanoramaDSP.h"
class PanoramaProcessor final: public juce::AudioProcessor {
public:
 PanoramaProcessor();
 ~PanoramaProcessor() override=default;
 void prepareToPlay(double sampleRate,int samplesPerBlock) override;
 void releaseResources() override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override{return true;}
 const juce::String getName() const override{return "PANORAMA typeGPT v0.4.3";}
 bool acceptsMidi() const override{return true;}
 bool producesMidi() const override{return false;}
 bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return 8.;}
 int getNumPrograms() override{return panorama::kPresetCount;}
 int getCurrentProgram() override{return currentProgram.load();}
 void setCurrentProgram(int n) override{loadPreset(n);}
 const juce::String getProgramName(int n) override;
 void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 void loadPreset(int index);
 panorama::Patch getPatchFromParameters() const;
 juce::AudioProcessorValueTreeState params;
 static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
private:
 panorama::Synth synth;
 std::atomic<int> currentProgram{0};
 void handleMidi(const juce::MidiMessage& m);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanoramaProcessor)
};
