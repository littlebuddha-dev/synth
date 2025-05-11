// synth/poly_synth.cpp
#include "poly_synth.h"
#include "effects/audio_effect.h" // Include the base effect class
#include "effects/reverb_effect.h"
#include "voice.h"
#include "waveform.h"
#include "envelope.h"
#include "synth_parameters.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

bool PolySynth::setParameter(SynthParams::ParamID paramID, float value) {
    // value の範囲チェックは各セッターに任せるか、ここでも行う
    // ここでは、既存のセッターを呼び出す形にする
    switch (paramID) {
        case SynthParams::ParamID::MasterTuneCents: setMasterTuneCents(value); break;
        case SynthParams::ParamID::Osc1Level: setOsc1Level(value); break;
        case SynthParams::ParamID::Osc2Level: setOsc2Level(value); break;
        case SynthParams::ParamID::NoiseLevel: setNoiseLevel(value); break;
        case SynthParams::ParamID::VCOBDetuneCents: setVCOBDetuneCents(value); break;
        case SynthParams::ParamID::VCOBFreqKnob: setVCOBFreqKnob(value); break;
        // ... (float を取るパラメータの case を追加) ...
        case SynthParams::ParamID::FilterEnvVelocitySensitivity: setFilterEnvVelocitySensitivity(value); break;
        case SynthParams::ParamID::AmpVelocitySensitivity: setAmpVelocitySensitivity(value); break;
        case SynthParams::ParamID::PulseWidth: setPulseWidth(value); break;
        case SynthParams::ParamID::PWMDepth: setPWMDepth(value); break;

        case SynthParams::ParamID::PMFilterEnvToFreqAAmount: setPMFilterEnvToFreqAAmount(value); break;
        case SynthParams::ParamID::PMFilterEnvToPWAAmount: setPMFilterEnvToPWAAmount(value); break;
        case SynthParams::ParamID::PMFilterEnvToFilterCutoffAmount: setPMFilterEnvToFilterCutoffAmount(value); break;
        case SynthParams::ParamID::PMOscBToFreqAAmount: setPMOscBToFreqAAmount(value); break;
        case SynthParams::ParamID::PMOscBToPWAAmount: setPMOscBToPWAAmount(value); break;
        case SynthParams::ParamID::PMOscBToFilterCutoffAmount: setPMOscBToFilterCutoffAmount(value); break;

        case SynthParams::ParamID::VCFBaseCutoff: setVCFBaseCutoff(value); break;
        case SynthParams::ParamID::VCFResonance: setVCFResonance(value); break;
        case SynthParams::ParamID::VCFKeyFollow: setVCFKeyFollow(value); break;
        case SynthParams::ParamID::VCFEnvelopeAmount: setVCFEnvelopeAmount(value); break;
        
        case SynthParams::ParamID::LfoRate: setLfoRate(value); break;
        case SynthParams::ParamID::LfoAmountToVco1Freq: setLfoAmountToVco1Freq(value); break;
        case SynthParams::ParamID::LfoAmountToVco2Freq: setLfoAmountToVco2Freq(value); break;
        case SynthParams::ParamID::LfoAmountToVco1Pw: setLfoAmountToVco1Pw(value); break;
        case SynthParams::ParamID::LfoAmountToVco2Pw: setLfoAmountToVco2Pw(value); break;
        case SynthParams::ParamID::LfoAmountToVcfCutoff: setLfoAmountToVcfCutoff(value); break;

        case SynthParams::ParamID::ModulationWheelValue: setModulationWheelValue(value); break;
        case SynthParams::ParamID::WheelModAmountToFreqA: setWheelModAmountToFreqA(value); break;
        case SynthParams::ParamID::WheelModAmountToFreqB: setWheelModAmountToFreqB(value); break;
        case SynthParams::ParamID::WheelModAmountToPWA: setWheelModAmountToPWA(value); break;
        case SynthParams::ParamID::WheelModAmountToPWB: setWheelModAmountToPWB(value); break;
        case SynthParams::ParamID::WheelModAmountToFilter: setWheelModAmountToFilter(value); break;
        
        case SynthParams::ParamID::UnisonDetuneCents: setUnisonDetuneCents(value); break;
        case SynthParams::ParamID::GlideTime: setGlideTime(value); break;

        case SynthParams::ParamID::AnalogPitchDriftDepth: setAnalogPitchDriftDepth(value); break;
        case SynthParams::ParamID::AnalogPWDriftDepth: setAnalogPWDriftDepth(value); break;

        // Effects (Reverb example)
        case SynthParams::ParamID::ReverbDryWetMix:
            if (!effectsChain.empty() && dynamic_cast<ReverbEffect*>(effectsChain.back().get())) { // 最後のeffectがReverbと仮定
                dynamic_cast<ReverbEffect*>(effectsChain.back().get())->setDryWetMix(value);
            } else return false;
            break;
        case SynthParams::ParamID::ReverbRoomSize:
             if (!effectsChain.empty() && dynamic_cast<ReverbEffect*>(effectsChain.back().get())) {
                dynamic_cast<ReverbEffect*>(effectsChain.back().get())->setRoomSize(value);
            } else return false;
            break;
        // ... (他のリバーブパラメータも同様に) ...

        // int/bool/enum を取るパラメータは setParameterInt で処理
        default:
            // または、float を int にキャストして setParameterInt を呼ぶか、エラーを返す
            return setParameterInt(paramID, static_cast<int>(value));
            // return false; // Invalid parameter for float value
    }
    return true;
}

bool PolySynth::setParameterInt(SynthParams::ParamID paramID, int value) {
    switch (paramID) {
        case SynthParams::ParamID::Waveform: setWaveform(static_cast<Waveform>(value)); break;
        case SynthParams::ParamID::SyncEnabled: setSyncEnabled(static_cast<bool>(value)); break;
        case SynthParams::ParamID::VCOBLowFreqEnabled: setVCOBLowFreqEnabled(static_cast<bool>(value)); break;
        // ... (int, bool, enum を取るパラメータの case を追加) ...
        case SynthParams::ParamID::LfoWaveform: setLfoWaveform(static_cast<LfoWaveform>(value)); break;
        case SynthParams::ParamID::WheelModSource: setWheelModSource(static_cast<WheelModSource>(value)); break;
        case SynthParams::ParamID::UnisonEnabled: setUnisonEnabled(static_cast<bool>(value)); break;
        case SynthParams::ParamID::GlideEnabled: setGlideEnabled(static_cast<bool>(value)); break;

        // Effects (Reverb example)
        case SynthParams::ParamID::ReverbEnabled:
            if (!effectsChain.empty() && dynamic_cast<ReverbEffect*>(effectsChain.back().get())) {
                dynamic_cast<ReverbEffect*>(effectsChain.back().get())->setEnabled(static_cast<bool>(value));
            } else return false;
            break;

        // float を取るパラメータは setParameter で処理
        default:
            // または、int を float にキャストして setParameter を呼ぶか、エラーを返す
            // return setParameter(paramID, static_cast<float>(value));
            return false; // Invalid parameter for int value
    }
    return true;
}

// getParameter の実装は省略（必要に応じて同様に switch 文で実装）
float PolySynth::getParameter(SynthParams::ParamID paramID) {
    // TODO: Implement getter logic
    // 例:
    // switch(paramID) {
    //    case SynthParams::ParamID::Osc1Level: return voices.empty() ? 0.0f : voices[0].getOsc1Level(); // 最初のボイスの値、またはグローバル値を取得
    // ...
    // }
    std::cerr << "getParameter for ID " << static_cast<int>(paramID) << " not fully implemented." << std::endl;
    return 0.0f; // Placeholder
}


bool PolySynth::setEnvelopeParams(SynthParams::ParamID baseParamID, const EnvelopeParams& params) {
    if (baseParamID == SynthParams::ParamID::AmpEnvAttack) { // AmpEnvAttackを代表IDとする
        setAmpEnvelope(params);
        return true;
    } else if (baseParamID == SynthParams::ParamID::FilterEnvAttack) { // FilterEnvAttackを代表IDとする
        setFilterEnvelope(params);
        return true;
    }
    return false;
}

// EnvelopeParams PolySynth::getEnvelopeParams(SynthParams::ParamID baseParamID) {
//    // TODO: Implement getter
//    return {};
// }

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
      masterTuneCents(0.0f), // Initialize masterTuneCents
      analogPitchDriftDepth_(0.0f), // Initialize analog drift depth
      analogPWDriftDepth_(0.0f)     // Initialize analog drift depth
{
  for (int i = 0; i < maxVoices; ++i) {
    voices.emplace_back(Voice(sampleRate, 16));
  }
  for (int i = 0; i < static_cast<int>(LfoDestination::NumDestinations); ++i) {
    lfoModAmounts[i] = 0.0f;
  }
  // Initialize voices with current drift settings (if any were set before voices are fully ready)
  // This ensures voices created here get the initial drift depth.
  // More robustly, Voice constructor could take these, or a full reset method could exist.
  setAnalogPitchDriftDepth(analogPitchDriftDepth_);
  setAnalogPWDriftDepth(analogPWDriftDepth_);
}

void PolySynth::noteOn(int midiNote, float velocity) {
  // Calculate base frequency including master tune
  // The formula is: A4_freq * 2^(( (MIDI_note - 69) * 100_cents +
  // master_tune_cents ) / 1200_cents)
  float tunedFreq =
      440.0f * std::pow(2.0f, ((static_cast<float>(midiNote) - 69.0f) * 100.0f +
                               masterTuneCents) /
                                  1200.0f);

  if (unisonEnabled) {
    lastUnisonNote = midiNote;
    lastUnisonVelocity = velocity;

    // tunedFreq is already master-tuned and corresponds to the base MIDI note
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
      // Pass the master-tuned frequency to the voice
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
  static int frameCounter = 0;

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

  if (unisonEnabled && frameCounter % (sampleRate / 2) == 0) {
  }

  for (int i = 0; i < voices.size(); ++i) {
    if (voices[i].isActive()) {
      float singleVoiceOutput = voices[i].process(currentLfoModulations);
      if (unisonEnabled && frameCounter % (sampleRate / 10) == 0 && i < 2) {
      }
      mixed += singleVoiceOutput;
      activeVoiceCount++;
    }
  }
  if (unisonEnabled && frameCounter % (sampleRate / 2) == 0 &&
      activeVoiceCount > 0) {
  }

  if (activeVoiceCount == 0) {
    if (unisonEnabled && frameCounter % (sampleRate / 2) == 0) {
    }
    frameCounter++;
    if (frameCounter >= sampleRate * 5)
      frameCounter = 0;
    return 0.0f;
  }

  float normalizationFactor;
  if (unisonEnabled) {
    normalizationFactor = static_cast<float>(std::max(1, maxVoices / 2)) * 3.0f;
    if (normalizationFactor < 1.0f) {
      normalizationFactor = 1.0f;
    }
    if (unisonEnabled && frameCounter % (sampleRate / 2) == 0 &&
        activeVoiceCount > 0) {
    }
  } else {
    normalizationFactor = static_cast<float>(std::max(1, maxVoices / 2));
  }

  if (mixed == 0.0f) {
    if (unisonEnabled && frameCounter % (sampleRate / 2) == 0 &&
        activeVoiceCount > 0) {
    }
    frameCounter++;
    if (frameCounter >= sampleRate * 5)
      frameCounter = 0;
    return 0.0f;
  }

  float final_output = mixed / normalizationFactor;

  if (unisonEnabled && frameCounter % (sampleRate / 2) == 0 &&
      activeVoiceCount > 0) {
  }

  frameCounter++;
  if (frameCounter >= sampleRate * 5)
    frameCounter = 0;

  // Process through effects chain
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
  // Typical range for master tune might be +/- 100 cents (1 semitone)
  // Or +/- 50 cents for finer control. Clamping can be added if desired.
  // For now, allow any float value.
  masterTuneCents = cents;
}

void PolySynth::addEffect(std::unique_ptr<AudioEffect> effect) {
  effectsChain.push_back(std::move(effect));
}

void PolySynth::clearEffects() { effectsChain.clear(); }

void PolySynth::setAnalogPitchDriftDepth(float cents) {
    analogPitchDriftDepth_ = std::max(0.0f, cents); // Depth should not be negative
    for (auto& voice : voices) {
        voice.setPitchDriftDepth(analogPitchDriftDepth_);
    }
}

void PolySynth::setAnalogPWDriftDepth(float depth) {
    // Depth for PW can be 0 to ~0.49 (max change to PW)
    // Let's clamp to 0.0 - 0.45 for safety, preventing full flip.
    analogPWDriftDepth_ = std::clamp(depth, 0.0f, 0.45f); 
    for (auto& voice : voices) {
        voice.setPWDriftDepth(analogPWDriftDepth_);
    }
}