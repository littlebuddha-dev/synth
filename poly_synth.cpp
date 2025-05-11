// synth/poly_synth.cpp
#include "poly_synth.h"
#include "effects/audio_effect.h" 
#include "voice.h"
#include "waveform.h"
#include <algorithm>
#include <cmath>
#include <iostream>

PolySynth::PolySynth(int sr, int maxNumVoices)
    : sampleRate(sr), maxVoices(maxNumVoices), lfo(sr), currentNoteTimestamp(0),
      modulationWheelValue(0.0f), wheelModSource(WheelModSource::LFO),
      wheelModToFreqAAmount(0.0f), wheelModToFreqBAmount(0.0f),
      wheelModToPWAAmount(0.0f), wheelModToPWBAmount(0.0f),
      wheelModToFilterAmount(0.0f),
      wheelModNoiseGenerator(std::random_device{}()),
      wheelModNoiseDistribution(-1.0f, 1.0f), unisonEnabled(false),
      unisonDetuneCents(7.0f), lastUnisonNote(-1), lastUnisonVelocity(0.0f),
      glideEnabled(false), glideTimeSetting(0.05f),
      masterTuneCents(0.0f), 
      analogPitchDriftDepth_(0.0f), 
      analogPWDriftDepth_(0.0f),
      pitchBendValue_(0.0f),        // Initialize pitch bend
      pitchBendRangeSemitones_(2.0f) // Default pitch bend range +/- 2 semitones
{
  for (int i = 0; i < maxVoices; ++i) {
    voices.emplace_back(Voice(sampleRate, 16)); 
  }
  for (int i = 0; i < static_cast<int>(LfoDestination::NumDestinations); ++i) {
    lfoModAmounts[i] = 0.0f;
  }
  
  setAnalogPitchDriftDepth(analogPitchDriftDepth_);
  setAnalogPWDriftDepth(analogPWDriftDepth_);
}

void PolySynth::noteOn(int midiNote, float velocity) {
  
  float tunedFreq =
      440.0f * std::pow(2.0f, ((static_cast<float>(midiNote) - 69.0f) * 100.0f +
                               masterTuneCents) /
                                  1200.0f);

  if (unisonEnabled) {
    lastUnisonNote = midiNote;
    lastUnisonVelocity = velocity;

    
    int numActiveVoices = voices.size();

    for (int i = 0; i < numActiveVoices; ++i) {
      float detuneFactor = 0.0f;
      if (numActiveVoices > 1) {
        detuneFactor =
            (static_cast<float>(i) / static_cast<float>(numActiveVoices - 1) -
             0.5f) *
            2.0f;
      }
      if (numActiveVoices <= 1 && i == 0) {
        detuneFactor = 0.0f;
      }

      float detunedFreq =
          tunedFreq *
          std::pow(2.0f, (unisonDetuneCents * detuneFactor) / 1200.0f);

      voices[i].setNoteOnTimestamp(currentNoteTimestamp);
      voices[i].noteOn(detunedFreq, velocity, midiNote, glideEnabled,
                       glideTimeSetting);
    }
    currentNoteTimestamp++;
  } else {
    Voice *voice = findFreeVoice();
    if (voice) {
      voice->setNoteOnTimestamp(currentNoteTimestamp++);
      
      voice->noteOn(tunedFreq, velocity, midiNote, glideEnabled,
                    glideTimeSetting);
    }
  }
}

void PolySynth::noteOff(int midiNote) {
  if (unisonEnabled) {
    if (midiNote == lastUnisonNote) {
      for (auto &voice : voices) {
        if (voice.isActive() && voice.getNoteNumber() == midiNote) {
          voice.noteOff();
        }
      }
      lastUnisonNote = -1;
      lastUnisonVelocity = 0.0f;
    }
  } else {
    for (auto &voice : voices) {
      if (voice.isActive() && voice.getNoteNumber() == midiNote) {
        voice.noteOff();
      }
    }
  }
}

float PolySynth::process() {
  float mixed = 0.0f;
  int activeVoiceCount = 0;
  // static int frameCounter = 0; // This was for debug, can be removed or kept if needed

  float lfoValue = lfo.step();

  float wheelModNoiseValue = wheelModNoiseDistribution(wheelModNoiseGenerator);
  float activeWheelModSourceValue = 0.0f;
  if (wheelModSource == WheelModSource::LFO) {
    activeWheelModSourceValue = lfoValue;
  } else {
    activeWheelModSourceValue = wheelModNoiseValue;
  }

  float wheel_mod_freqA_semitones = activeWheelModSourceValue *
                                    modulationWheelValue *
                                    wheelModToFreqAAmount * 12.0f;
  float wheel_mod_freqB_semitones = activeWheelModSourceValue *
                                    modulationWheelValue *
                                    wheelModToFreqBAmount * 12.0f;
  float wheel_mod_pwA_offset = activeWheelModSourceValue *
                               modulationWheelValue * wheelModToPWAAmount *
                               0.49f;
  float wheel_mod_pwB_offset = activeWheelModSourceValue *
                               modulationWheelValue * wheelModToPWBAmount *
                               0.49f;
  float wheel_mod_filter_hz_offset = activeWheelModSourceValue *
                                     modulationWheelValue *
                                     wheelModToFilterAmount * 2000.0f;

  LfoModulationValues currentLfoModulations;
  currentLfoModulations.osc1FreqMod =
      lfoValue * lfoModAmounts[static_cast<int>(LfoDestination::VCO1_Freq)];
  currentLfoModulations.osc2FreqMod =
      lfoValue * lfoModAmounts[static_cast<int>(LfoDestination::VCO2_Freq)];
  currentLfoModulations.osc1PwMod =
      lfoValue * lfoModAmounts[static_cast<int>(LfoDestination::VCO1_PW)];
  currentLfoModulations.osc2PwMod =
      lfoValue * lfoModAmounts[static_cast<int>(LfoDestination::VCO2_PW)];
  currentLfoModulations.vcfCutoffMod =
      lfoValue * lfoModAmounts[static_cast<int>(LfoDestination::VCF_Cutoff)];

  currentLfoModulations.osc1FreqMod += wheel_mod_freqA_semitones;
  currentLfoModulations.osc2FreqMod += wheel_mod_freqB_semitones;
  currentLfoModulations.wheelOsc1PwOffset = wheel_mod_pwA_offset;
  currentLfoModulations.wheelOsc2PwOffset = wheel_mod_pwB_offset;
  currentLfoModulations.vcfCutoffMod += wheel_mod_filter_hz_offset;


  for (int i = 0; i < voices.size(); ++i) {
    if (voices[i].isActive()) {
      float singleVoiceOutput = voices[i].process(currentLfoModulations, pitchBendValue_, pitchBendRangeSemitones_);
      mixed += singleVoiceOutput;
      activeVoiceCount++;
    }
  }
  
  if (activeVoiceCount == 0) {
    return 0.0f;
  }

  float normalizationFactor;
  if (unisonEnabled) {
    normalizationFactor = static_cast<float>(std::max(1, maxVoices / 2)) * 3.0f;
    if (normalizationFactor < 1.0f) {
      normalizationFactor = 1.0f;
    }
  } else {
    normalizationFactor = static_cast<float>(std::max(1, maxVoices / 2));
     if (normalizationFactor < 1.0f) { // Ensure normalizationFactor is at least 1
      normalizationFactor = 1.0f;
    }
  }

  if (mixed == 0.0f) {
    return 0.0f;
  }

  float final_output = mixed / normalizationFactor;
  
  float effected_output = final_output;
  for (const auto &effect : effectsChain) {
    if (effect && effect->isEnabled()) {
      effected_output = effect->processSample(effected_output);
    }
  }
  return effected_output;
}

Voice *PolySynth::findFreeVoice() {
  for (auto &voice : voices) {
    if (voice.isTrulyIdle()) {
      return &voice;
    }
  }

  Voice *quietestReleasingVoice = nullptr;
  float minAmpEnvLevel = 2.0f;

  for (auto &voice : voices) {
    if (!voice.isGateOpen() && voice.areEnvelopesActive()) {
      float currentAmpLevel = voice.getAmpEnvLevel();
      if (currentAmpLevel < minAmpEnvLevel) {
        minAmpEnvLevel = currentAmpLevel;
        quietestReleasingVoice = &voice;
      }
    }
  }
  if (quietestReleasingVoice) {
    return quietestReleasingVoice;
  }

  Voice *oldestSustainingVoice = nullptr;
  unsigned long long oldestTimestamp = currentNoteTimestamp;

  for (auto &voice : voices) {
    if (voice.isGateOpen()) {
      if (voice.getNoteOnTimestamp() < oldestTimestamp) {
        oldestTimestamp = voice.getNoteOnTimestamp();
        oldestSustainingVoice = &voice;
      }
    }
  }
  if (oldestSustainingVoice) {
    return oldestSustainingVoice;
  }

  return voices.empty() ? nullptr : &voices[0];
}

Voice *PolySynth::findVoiceForNote(int midiNote) {
  for (auto &voice : voices) {
    if (voice.isActive() && voice.getNoteNumber() == midiNote) {
      return &voice;
    }
  }
  return nullptr;
}

void PolySynth::setWaveform(Waveform wf) {
  for (auto &voice : voices)
    voice.setWaveform(wf);
}
void PolySynth::setNoiseLevel(float level) {
  for (auto &voice : voices)
    voice.setNoiseLevel(level);
}
void PolySynth::setOsc1Level(float level) {
  for (auto &voice : voices)
    voice.setOsc1Level(level);
}
void PolySynth::setOsc2Level(float level) {
  for (auto &voice : voices)
    voice.setOsc2Level(level);
}
void PolySynth::setVCOBDetuneCents(float cents) {
  for (auto &voice : voices)
    voice.setVCOBDetuneCents(cents);
}
void PolySynth::setVCOBLowFreqEnabled(bool enabled) {
  for (auto &voice : voices)
    voice.setVCOBLowFreqEnabled(enabled);
}
void PolySynth::setVCOBFreqKnob(float value) {
  for (auto &voice : voices)
    voice.setVCOBFreqKnob(value);
}
void PolySynth::setVCOBKeyFollowEnabled(bool enabled) {
  for (auto &voice : voices)
    voice.setVCOBKeyFollowEnabled(enabled);
}
void PolySynth::setFilterEnvVelocitySensitivity(float amount) {
  for (auto &voice : voices)
    voice.setFilterEnvVelocitySensitivity(amount);
}

void PolySynth::setAmpVelocitySensitivity(float amount) {
  for (auto &voice : voices)
    voice.setAmpVelocitySensitivity(amount);
}

void PolySynth::setSyncEnabled(bool enabled) {
  for (auto &voice : voices)
    voice.setSyncEnabled(enabled);
}

void PolySynth::setPMFilterEnvToFreqAAmount(float amount) {
  for (auto &voice : voices)
    voice.setPMFilterEnvToFreqAAmount(amount);
}
void PolySynth::setPMFilterEnvToPWAAmount(float amount) {
  for (auto &voice : voices)
    voice.setPMFilterEnvToPWAAmount(amount);
}
void PolySynth::setPMFilterEnvToFilterCutoffAmount(float amount) {
  for (auto &voice : voices)
    voice.setPMFilterEnvToFilterCutoffAmount(amount);
}
void PolySynth::setPMOscBToFreqAAmount(float amount) {
  for (auto &voice : voices)
    voice.setPMOscBToFreqAAmount(amount);
}
void PolySynth::setPMOscBToPWAAmount(float amount) {
  for (auto &voice : voices)
    voice.setPMOscBToPWAAmount(amount);
}
void PolySynth::setPMOscBToFilterCutoffAmount(float amount) {
  for (auto &voice : voices)
    voice.setPMOscBToFilterCutoffAmount(amount);
}

void PolySynth::setAmpEnvelope(const EnvelopeParams &p) {
  for (auto &voice : voices)
    voice.setAmpEnvelope(p);
}
void PolySynth::setFilterEnvelope(const EnvelopeParams &p) {
  for (auto &voice : voices)
    voice.setFilterEnvelope(p);
}
void PolySynth::setPulseWidth(float width) {
  for (auto &voice : voices)
    voice.setPulseWidth(width);
}
void PolySynth::setPWMDepth(float depth) {
  for (auto &voice : voices)
    voice.setPWMDepth(depth);
}
void PolySynth::setLfoRate(float rateHz) { lfo.setRate(rateHz); }
void PolySynth::setLfoWaveform(LfoWaveform wf) { lfo.setWaveform(wf); }
void PolySynth::setLfoAmountToVco1Freq(float semitones) {
  lfoModAmounts[static_cast<int>(LfoDestination::VCO1_Freq)] = semitones;
}
void PolySynth::setLfoAmountToVco2Freq(float semitones) {
  lfoModAmounts[static_cast<int>(LfoDestination::VCO2_Freq)] = semitones;
}
void PolySynth::setLfoAmountToVco1Pw(float normalizedAmount) {
  lfoModAmounts[static_cast<int>(LfoDestination::VCO1_PW)] =
      std::clamp(normalizedAmount, 0.0f, 1.0f);
}
void PolySynth::setLfoAmountToVco2Pw(float normalizedAmount) {
  lfoModAmounts[static_cast<int>(LfoDestination::VCO2_PW)] =
      std::clamp(normalizedAmount, 0.0f, 1.0f);
}
void PolySynth::setLfoAmountToVcfCutoff(float hzOffset) {
  lfoModAmounts[static_cast<int>(LfoDestination::VCF_Cutoff)] = hzOffset;
}
void PolySynth::setVCFBaseCutoff(float hz) {
  for (auto &voice : voices)
    voice.setVCFBaseCutoff(hz);
}
void PolySynth::setVCFResonance(float q) {
  for (auto &voice : voices)
    voice.setVCFResonance(q);
}
void PolySynth::setVCFKeyFollow(float f) {
  for (auto &voice : voices)
    voice.setVCFKeyFollow(f);
}
void PolySynth::setVCFEnvelopeAmount(float amt) {
  for (auto &voice : voices)
    voice.setVCFEnvelopeAmount(amt);
}

void PolySynth::setModulationWheelValue(float value) {
  modulationWheelValue = std::clamp(value, 0.0f, 1.0f);
}

void PolySynth::setWheelModSource(WheelModSource source) {
  wheelModSource = source;
}

void PolySynth::setWheelModAmountToFreqA(float amount) {
  wheelModToFreqAAmount = std::clamp(amount, 0.0f, 1.0f);
}

void PolySynth::setWheelModAmountToFreqB(float amount) {
  wheelModToFreqBAmount = std::clamp(amount, 0.0f, 1.0f);
}

void PolySynth::setWheelModAmountToPWA(float amount) {
  wheelModToPWAAmount = std::clamp(amount, 0.0f, 1.0f);
}

void PolySynth::setWheelModAmountToPWB(float amount) {
  wheelModToPWBAmount = std::clamp(amount, 0.0f, 1.0f);
}

void PolySynth::setWheelModAmountToFilter(float amount) {
  wheelModToFilterAmount = std::clamp(amount, 0.0f, 1.0f);
}

void PolySynth::setUnisonEnabled(bool enabled) {
  if (unisonEnabled && !enabled) {
    for (auto &voice : voices) {
      if (voice.isActive()) {
      }
    }
    lastUnisonNote = -1;
  }
  unisonEnabled = enabled;
  if (enabled) {
  }
}

void PolySynth::setUnisonDetuneCents(float cents) {
  unisonDetuneCents = std::max(0.0f, cents);
}

void PolySynth::setGlideEnabled(bool enabled) { glideEnabled = enabled; }

void PolySynth::setGlideTime(float timeSeconds) {
  glideTimeSetting = std::max(0.0f, timeSeconds);
}

void PolySynth::setMasterTuneCents(float cents) {
  masterTuneCents = cents;
}

void PolySynth::setPitchBend(float value) {
    pitchBendValue_ = std::clamp(value, -1.0f, 1.0f);
}

void PolySynth::setPitchBendRange(float semitones) {
    pitchBendRangeSemitones_ = std::max(0.0f, semitones); // Range should be non-negative
}

void PolySynth::addEffect(std::unique_ptr<AudioEffect> effect) {
  effectsChain.push_back(std::move(effect));
}

void PolySynth::clearEffects() { effectsChain.clear(); }

void PolySynth::setAnalogPitchDriftDepth(float cents) {
    analogPitchDriftDepth_ = std::max(0.0f, cents); 
    for (auto& voice : voices) {
        voice.setPitchDriftDepth(analogPitchDriftDepth_);
    }
}

void PolySynth::setAnalogPWDriftDepth(float depth) {
    analogPWDriftDepth_ = std::clamp(depth, 0.0f, 0.45f); 
    for (auto& voice : voices) {
        voice.setPWDriftDepth(analogPWDriftDepth_);
    }
}

void PolySynth::setOscHarmonicAmplitude(int oscNum, int harmonicIndex, float amplitude) {
    if (harmonicIndex < 0 || harmonicIndex >= 10) { 
        return;
    }

    for (auto& voice : voices) {
        if (oscNum == 1) {
            voice.setOsc1HarmonicAmplitude(harmonicIndex, amplitude);
        } else if (oscNum == 2) {
            voice.setOsc2HarmonicAmplitude(harmonicIndex, amplitude);
        }
    }
}