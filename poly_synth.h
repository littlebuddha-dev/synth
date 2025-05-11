// synth/poly_synth.h
#pragma once
#include "envelope.h" // For EnvelopeParams
#include "lfo.h"      // For LFO class and LfoWaveform enum
#include "voice.h"
#include "waveform.h" // For Waveform enum
#include <memory>     // For std::unique_ptr
#include <vector>
#include <utility> // For std::pair

// LFO Destinations
enum class LfoDestination {
  None = 0,
  VCO1_Freq = 1,
  VCO2_Freq = 2,
  VCO1_PW = 3,
  VCO2_PW = 4,
  VCF_Cutoff = 5,
  NumDestinations // Keep last for array sizing
};

// Wheel Modulation Source
enum class WheelModSource { LFO, NOISE };

class AudioEffect; // Forward declaration

struct StereoSample {
    float L = 0.0f;
    float R = 0.0f;
};

class PolySynth {
public:
  PolySynth(int sampleRate = 44100, int maxVoices = 16);
  void noteOn(int midiNote, float velocity);
  void noteOff(int midiNote);
  StereoSample process(); // Generates one stereo sample pair

  // Global Voice Parameters
  void setWaveform(Waveform wf); // Sets for both OSCs in all voices
  void setOsc1Level(float);
  void setOsc2Level(float);
  void setNoiseLevel(float level);      // Global noise level for all voices
  void setRingModLevel(float level);    // Global ring mod level for all voices
  void setVCOBDetuneCents(float cents); // For VCO B fine tune
  void setSyncEnabled(bool enabled);
  void setVCOBLowFreqEnabled(bool enabled);
  void setVCOBFreqKnob(float value); // 0.0 to 1.0
  void setVCOBKeyFollowEnabled(bool enabled); 
  void setFilterEnvVelocitySensitivity(float amount);
  void setAmpVelocitySensitivity(float amount);
  void setPulseWidth(float width); // Base PW for both OSCs
  void setPWMDepth(
      float depth); 

  // PolyMod Parameters
  void setPMFilterEnvToFreqAAmount(float amount);
  void setPMFilterEnvToPWAAmount(float amount);
  void setPMFilterEnvToFilterCutoffAmount(float amount);
  void setPMOscBToFreqAAmount(float amount);
  void setPMOscBToPWAAmount(float amount);
  void setPMOscBToFilterCutoffAmount(float amount);

  // Filter Parameters (Global for all voices)
  void setVCFBaseCutoff(float hz);
  void setVCFResonance(float q);
  void setVCFKeyFollow(float f);
  void setVCFEnvelopeAmount(float amt);

  // Envelope Parameters (Global for all voices)
  void setAmpEnvelope(const EnvelopeParams &);
  void setFilterEnvelope(const EnvelopeParams &);

  // LFO Parameters
  void setLfoRate(float rateHz);
  void setLfoWaveform(LfoWaveform wf);
  void setLfoAmountToVco1Freq(float semitones); 
  void setLfoAmountToVco2Freq(float semitones);
  void setLfoAmountToVco1Pw(float normalizedAmount); 
  void setLfoAmountToVco2Pw(float normalizedAmount);
  void setLfoAmountToVcfCutoff(float hzOffset); 

  // Wheel Modulation Settings
  void setModulationWheelValue(float value); 
  void setWheelModSource(WheelModSource source);
  void setWheelModAmountToFreqA(float amount);  
  void setWheelModAmountToFreqB(float amount);  
  void setWheelModAmountToPWA(float amount);    
  void setWheelModAmountToPWB(float amount);    
  void setWheelModAmountToFilter(float amount); 

  // Unison Settings
  void setUnisonEnabled(bool enabled);
  void setUnisonDetuneCents(float cents);
  void setUnisonStereoSpread(float spread); // 0.0 (mono) to 1.0 (max spread)

  // Glide/Portamento Settings
  void setGlideEnabled(bool enabled);
  void setGlideTime(float timeSeconds); 

  // Master Tune Setting
  void setMasterTuneCents(float cents);

  // Pitch Bend Settings
  void setPitchBend(float value); // -1.0 to 1.0
  void setPitchBendRange(float semitones); // e.g., 2.0 for +/- 2 semitones

  // Effects Management
  void addEffect(std::unique_ptr<AudioEffect> effect);
  void clearEffects();

  // Analog Drift Parameters
  void setAnalogPitchDriftDepth(float cents); 
  void setAnalogPWDriftDepth(float depth);    

  // Harmonic Amplitude Parameters (for Additive waveform)
  void setOscHarmonicAmplitude(int oscNum, int harmonicIndex, float amplitude); // harmonicIndex is 0-based

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

  float pitchBendValue_; // -1.0 to 1.0, center is 0.0
  float pitchBendRangeSemitones_; // Default +/- 2 semitones

  std::vector<std::unique_ptr<AudioEffect>> effectsChain;

  unsigned long long currentNoteTimestamp = 0;

  Voice *findFreeVoice();
  Voice *findVoiceForNote(int midiNote);
};