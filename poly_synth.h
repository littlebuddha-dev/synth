// synth/poly_synth.h
#pragma once
#include "envelope.h" // For EnvelopeParams
#include "lfo.h"      // For LFO class and LfoWaveform enum
#include "voice.h"
#include "waveform.h" // For Waveform enum
#include <memory>     // For std::unique_ptr
#include <vector>
#include "synth_parameters.h"


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

class PolySynth {
public:
  PolySynth(int sampleRate = 44100, int maxVoices = 16);
  void noteOn(int midiNote, float velocity);
  void noteOff(int midiNote);
  float process(); // Generates one sample

  // Global Voice Parameters
  void setWaveform(Waveform wf); // Sets for both OSCs in all voices
  void setOsc1Level(float);
  void setOsc2Level(float);
  void setNoiseLevel(float level);      // Global noise level for all voices
  void setVCOBDetuneCents(float cents); // For VCO B fine tune
  void setSyncEnabled(bool enabled);
  void setVCOBLowFreqEnabled(bool enabled);
  void setVCOBFreqKnob(float value); // 0.0 to 1.0
  void setFilterEnvVelocitySensitivity(float amount);
  void setAmpVelocitySensitivity(float amount);
  void setPulseWidth(float width); // Base PW for both OSCs
  void setPWMDepth(
      float depth); // General PWM depth (might be combined with LFO amount)

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
  void setLfoAmountToVco1Freq(float semitones); // Max semitones deviation
  void setLfoAmountToVco2Freq(float semitones);
  void setLfoAmountToVco1Pw(float normalizedAmount); // 0-1 scales PWM depth
  void setLfoAmountToVco2Pw(float normalizedAmount);
  void setLfoAmountToVcfCutoff(float hzOffset); // Max Hz deviation

  // Wheel Modulation Settings
  void setModulationWheelValue(float value); // 0.0 to 1.0
  void setWheelModSource(WheelModSource source);
  void setWheelModAmountToFreqA(float amount);  // Amount for VCO1/A Freq
  void setWheelModAmountToFreqB(float amount);  // Amount for VCO2/B Freq
  void setWheelModAmountToPWA(float amount);    // Amount for VCO1/A PW
  void setWheelModAmountToPWB(float amount);    // Amount for VCO2/B PW
  void setWheelModAmountToFilter(float amount); // Amount for VCF Cutoff

  // Unison Settings
  void setUnisonEnabled(bool enabled);
  void setUnisonDetuneCents(float cents);

  // Glide/Portamento Settings
  void setGlideEnabled(bool enabled);
  void setGlideTime(float timeSeconds); // Time to glide between notes

  // Master Tune Setting
  void setMasterTuneCents(float cents);

  // Effects Management
  void addEffect(std::unique_ptr<AudioEffect> effect);
  void clearEffects();

  // Analog Drift Parameters
  void setAnalogPitchDriftDepth(float cents); // Max pitch deviation in cents
  void setAnalogPWDriftDepth(float depth);    // Max PW deviation (0.0 to ~0.45)
  
   // --- Generic Parameter Access ---
    // Returns false if parameter ID is invalid or value is out of range
    bool setParameter(SynthParams::ParamID paramID, float value);
    bool setParameterInt(SynthParams::ParamID paramID, int value); // For int/bool/enum params
    float getParameter(SynthParams::ParamID paramID); // Might need getParameterInt too
    // int getParameterInt(SynthParams::ParamID paramID);

    // For parameters like envelopes that take multiple values
    bool setEnvelopeParams(SynthParams::ParamID baseParamID, const EnvelopeParams& params);
    // EnvelopeParams getEnvelopeParams(SynthParams::ParamID baseParamID);


private:
  std::vector<Voice> voices;
  int sampleRate;
  int maxVoices;

  LFO lfo; // Shared LFO for all voices

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
  int lastUnisonNote = -1;
  float lastUnisonVelocity = 0.0f;

  bool glideEnabled = false;
  float glideTimeSetting = 0.05f;

  float analogPitchDriftDepth_; 
  float analogPWDriftDepth_;   

  float masterTuneCents = 0.0f; // Master tuning in cents relative to A4=440Hz

  std::vector<std::unique_ptr<AudioEffect>> effectsChain;

  unsigned long long currentNoteTimestamp = 0;

  Voice *findFreeVoice();
  Voice *findVoiceForNote(int midiNote);
};