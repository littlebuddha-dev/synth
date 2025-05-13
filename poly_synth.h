// synth/poly_synth.h
#pragma once
#include "envelope.h" 
#include "lfo.h"      
#include "voice.h" 
#include "waveform.h" 
#include "synth_parameters.h" 
#include <memory>     
#include <vector>
#include <utility> 

enum class LfoDestination {
  None = 0,
  VCO1_Freq = 1,
  VCO2_Freq = 2,
  VCO1_PW = 3,
  VCO2_PW = 4,
  VCF_Cutoff = 5,
  NumDestinations 
};

enum class WheelModSource { LFO, NOISE };

class AudioEffect; 

struct StereoSample {
    float L = 0.0f;
    float R = 0.0f;
};

class PolySynth {
public:
  PolySynth(int sampleRate = 44100, int maxVoices = 16);
  void noteOn(int midiNote, float velocity);
  void noteOff(int midiNote);
  StereoSample process(); 

  void setWaveform(Waveform wf); 
  void setOsc1Level(float);
  void setOsc2Level(float);
  void setNoiseLevel(float level);      
  void setRingModLevel(float level);    
  void setVCOBDetuneCents(float cents); 
  void setSyncEnabled(bool enabled);
  void setVCOBLowFreqEnabled(bool enabled);
  void setVCOBFreqKnob(float value); 
  void setVCOBKeyFollowEnabled(bool enabled); 
  void setFilterEnvVelocitySensitivity(float amount);
  void setAmpVelocitySensitivity(float amount);
  void setPulseWidth(float width); 
  void setPWMDepth(float depth); 

  void setXModOsc2ToOsc1FMAmount(float amount);
  void setXModOsc1ToOsc2FMAmount(float amount);

  void setPMFilterEnvToFreqAAmount(float amount);
  void setPMFilterEnvToPWAAmount(float amount);
  void setPMFilterEnvToFilterCutoffAmount(float amount);
  void setPMOscBToPWAAmount(float amount);
  void setPMOscBToFilterCutoffAmount(float amount);

  void setFilterType(SynthParams::FilterType type); 
  void setVCFBaseCutoff(float hz);
  void setVCFResonance(float q);
  void setVCFKeyFollow(float f);
  void setVCFEnvelopeAmount(float amt);

  void setAmpEnvelope(const EnvelopeParams &);
  void setFilterEnvelope(const EnvelopeParams &);

  void setLfoRate(float rateHz);
  void setLfoWaveform(LfoWaveform wf);
  void setLfoAmountToVco1Freq(float semitones); 
  void setLfoAmountToVco2Freq(float semitones);
  void setLfoAmountToVco1Pw(float normalizedAmount); 
  void setLfoAmountToVco2Pw(float normalizedAmount);
  void setLfoAmountToVcfCutoff(float hzOffset); 

  void setModulationWheelValue(float value); 
  void setWheelModSource(WheelModSource source);
  void setWheelModAmountToFreqA(float amount);  
  void setWheelModAmountToFreqB(float amount);  
  void setWheelModAmountToPWA(float amount);    
  void setWheelModAmountToPWB(float amount);    
  void setWheelModAmountToFilter(float amount); 

  void setUnisonEnabled(bool enabled);
  void setUnisonDetuneCents(float cents);
  void setUnisonStereoSpread(float spread); 

  void setGlideEnabled(bool enabled);
  void setGlideTime(float timeSeconds); 

  void setMasterTuneCents(float cents);

  void setPitchBend(float value); 
  void setPitchBendRange(float semitones); 

  void addEffect(std::unique_ptr<AudioEffect> effect);
  void clearEffects();
  AudioEffect* getEffect(size_t index); // To get reverb for parameter setting


  void setAnalogPitchDriftDepth(float cents); 
  void setAnalogPWDriftDepth(float depth);    

  void setOscHarmonicAmplitude(int oscNum, int harmonicIndex, float amplitude); 

  void setMixerDrive(float drive);
  void setMixerPostGain(float gain);
  int getSampleRate() const { return sampleRate; }
  
private:
  std::vector<Voice> voices;
  int sampleRate;
  int maxVoices;

  LFO lfo; 

  float lfoModAmounts[static_cast<int>(LfoDestination::NumDestinations)];

  float modulationWheelValue = 0.0f;
  WheelModSource wheelModSource = WheelModSource::LFO;
  float wheelModToFreqAAmount = 0.0f;
  float wheelModToFreqBAmount = 0.0f;
  float wheelModToPWAAmount = 0.0f;
  float wheelModToPWBAmount = 0.0f;
  float wheelModToFilterAmount = 0.0f;

  std::default_random_engine wheelModNoiseGenerator;
  std::uniform_real_distribution<float> wheelModNoiseDistribution;

  bool unisonEnabled = false;
  float unisonDetuneCents = 7.0f;
  float unisonStereoSpread_ = 0.5f; 
  int lastUnisonNote = -1;
  float lastUnisonVelocity = 0.0f;

  bool glideEnabled = false;
  float glideTimeSetting = 0.05f;

  float analogPitchDriftDepth_; 
  float analogPWDriftDepth_;   

  float masterTuneCents = 0.0f; 

  float pitchBendValue_; 
  float pitchBendRangeSemitones_; 

  std::vector<std::unique_ptr<AudioEffect>> effectsChain;

  unsigned long long currentNoteTimestamp = 0;

  Voice *findFreeVoice();
  Voice *findVoiceForNote(int midiNote);
};