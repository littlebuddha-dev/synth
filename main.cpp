// synth/main.cpp
#include "effects/reverb_effect.h"
#include "envelope.h"
#include "lfo.h"
#include "poly_synth.h"
#include "waveform.h"
#include "synth_parameters.h" // For SynthParams::FilterType
#include <cmath>
#include <iostream>
#include <memory>
#include <portaudio.h>
#include <vector> 

PolySynth synth(44100.0f, 8); 
constexpr float sampleRate = 44100.0f;
ReverbEffect *mainReverb = nullptr; 

int callback(const void *, void *outputBuffer, unsigned long framesPerBuffer,
             const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *) {
  float *out = static_cast<float *>(outputBuffer);
  for (unsigned long i = 0; i < framesPerBuffer; ++i) {
    StereoSample sample = synth.process();
    *out++ = sample.L; 
    *out++ = sample.R; 
  }
  return paContinue;
}

void resetPolyModAmounts(PolySynth &s) {
  s.setPMFilterEnvToFreqAAmount(0.0f);
  s.setPMFilterEnvToPWAAmount(0.0f);
  s.setPMFilterEnvToFilterCutoffAmount(0.0f);
  s.setPMOscBToPWAAmount(0.0f);
  s.setPMOscBToFilterCutoffAmount(0.0f);
}

void resetWheelModAmounts(PolySynth &s) {
  s.setWheelModAmountToFreqA(0.0f);
  s.setWheelModAmountToFreqB(0.0f);
  s.setWheelModAmountToPWA(0.0f);
  s.setWheelModAmountToPWB(0.0f);
  s.setWheelModAmountToFilter(0.0f);
  s.setModulationWheelValue(0.0f);
}

void setupBasicSynthSound(PolySynth &s, bool resetWheelAndPolyAndFM = true) {
  s.setFilterType(SynthParams::FilterType::LPF24); // Default to LPF24
  s.setWaveform(Waveform::Saw);
  s.setOsc1Level(1.0f);
  s.setOsc2Level(0.0f);
  s.setNoiseLevel(0.0f);
  s.setRingModLevel(0.0f);
  s.setVCOBDetuneCents(0.0f);
  s.setSyncEnabled(false);
  s.setPulseWidth(0.5f);

  s.setVCOBLowFreqEnabled(false);
  s.setVCOBFreqKnob(0.5f); 
  s.setVCOBKeyFollowEnabled(true); 

  s.setFilterEnvVelocitySensitivity(0.0f);
  s.setAmpVelocitySensitivity(0.7f);

  s.setVCFBaseCutoff(5000.0f);
  s.setVCFResonance(0.05f);
  s.setVCFKeyFollow(0.0f);
  s.setVCFEnvelopeAmount(0.5f);

  EnvelopeParams ampEnvParams = {0.01f, 0.3f, 0.7f, 0.5f};
  s.setAmpEnvelope(ampEnvParams);
  EnvelopeParams filterEnvParams = {0.05f, 0.4f, 0.2f, 0.8f};
  s.setFilterEnvelope(filterEnvParams);

  s.setLfoRate(0.0f); 
  s.setLfoWaveform(LfoWaveform::Triangle); 
  s.setLfoAmountToVco1Freq(0.0f);
  s.setLfoAmountToVco2Freq(0.0f);
  s.setLfoAmountToVco1Pw(0.0f);
  s.setLfoAmountToVco2Pw(0.0f);
  s.setLfoAmountToVcfCutoff(0.0f);

  if (resetWheelAndPolyAndFM) { 
    resetPolyModAmounts(s);
    resetWheelModAmounts(s);
    s.setXModOsc1ToOsc2FMAmount(0.0f); 
    s.setXModOsc2ToOsc1FMAmount(0.0f);
  }
  s.setUnisonEnabled(false);
  s.setUnisonDetuneCents(7.0f);
  s.setUnisonStereoSpread(0.7f);


  s.setGlideEnabled(false);
  s.setGlideTime(0.05f);

  s.setMasterTuneCents(0.0f); 
  s.setPitchBend(0.0f); 
  s.setPitchBendRange(2.0f); 

  if (mainReverb) {
    mainReverb->setEnabled(false); 
  }
  
  s.setAnalogPitchDriftDepth(0.0f);
  s.setAnalogPWDriftDepth(0.0f);
  
  for (int oscNum = 1; oscNum <= 2; ++oscNum) {
      for (int i = 0; i < 16; ++i) { 
          s.setOscHarmonicAmplitude(oscNum, i, (i == 0) ? 1.0f : 0.0f);
      }
  }
}

int main() {
  if (Pa_Initialize() != paNoError) {
    std::cerr << "PortAudio initialization error!" << std::endl;
    return 1;
  }
  
  auto reverbInstance = std::make_unique<ReverbEffect>(sampleRate);
  mainReverb = reverbInstance.get(); 
  synth.addEffect(std::move(reverbInstance));
  
  PaStream *stream;
  PaError err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, sampleRate, 256,
                                     callback, nullptr);
  if (err != paNoError) {
    std::cerr << "PortAudio stream open error: " << Pa_GetErrorText(err)
              << std::endl;
    Pa_Terminate();
    return 1;
  }

  err = Pa_StartStream(stream);
  if (err != paNoError) {
    std::cerr << "PortAudio stream start error: " << Pa_GetErrorText(err)
              << std::endl;
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 1;
  }

  std::cout << "Synth running... Test sequence will start." << std::endl;

  // --- Basic Sound Check ---
  std::cout << "\n--- Basic Sound Check (C4 Saw, LPF24) ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.noteOn(60, 100); 
  Pa_Sleep(1000);
  synth.noteOff(60);
  Pa_Sleep(1000);

  // --- Filter Type Tests ---
  std::cout << "\n--- Filter Type Tests (Saw wave, sweeping cutoff with ENV) ---" << std::endl;
  SynthParams::FilterType typesToTest[] = {
      SynthParams::FilterType::LPF24,
      SynthParams::FilterType::LPF12,
      SynthParams::FilterType::HPF12,
      SynthParams::FilterType::BPF12,
      SynthParams::FilterType::NOTCH
  };
  const char* typeNames[] = {"LPF24", "LPF12", "HPF12", "BPF12", "NOTCH"};

  for (int i = 0; i < 5; ++i) {
      std::cout << "Testing Filter Type: " << typeNames[i] << std::endl;
      setupBasicSynthSound(synth);
      synth.setFilterType(typesToTest[i]);
      synth.setWaveform(Waveform::Saw);
      synth.setOsc1Level(1.0f);
      synth.setVCFBaseCutoff(typesToTest[i] == SynthParams::FilterType::HPF12 ? 100.0f : 8000.0f); // HPF needs low base to sweep up
      synth.setVCFResonance(0.4f); // Moderate resonance to hear character
      synth.setVCFEnvelopeAmount(typesToTest[i] == SynthParams::FilterType::HPF12 ? -0.7f : 0.7f); // HPF sweep down (effectively opening)
      EnvelopeParams filterEnvSweep = {0.1f, 1.0f, 0.0f, 0.5f};
      synth.setFilterEnvelope(filterEnvSweep);
      
      synth.noteOn(48, 110); // C3
      Pa_Sleep(2500);
      synth.noteOff(48);
      Pa_Sleep(1000);
  }
  // --- End Filter Type Tests ---


  // --- Cross Modulation (FM) Test (Remains largely unchanged but uses LPF24 default) ---
  std::cout << "\n--- Cross Modulation (FM) Test ---" << std::endl;
  setupBasicSynthSound(synth); // This will set LPF24 by default
  synth.setWaveform(Waveform::Sine); 
  synth.setOsc1Level(1.0f);
  synth.setOsc2Level(0.0f); 
  synth.setVCOBKeyFollowEnabled(true); 
  synth.setVCOBFreqKnob(0.5f + (7.0f / 60.0f)); 
  
  std::cout << "Test 1: Osc2 -> Osc1 FM (gentle, harmonic)" << std::endl;
  synth.setXModOsc2ToOsc1FMAmount(0.15f); 
  synth.setXModOsc1ToOsc2FMAmount(0.0f);
  synth.noteOn(48, 100); 
  Pa_Sleep(2000);
  synth.noteOff(48);
  Pa_Sleep(500);


  // (Keep other tests like Moog Bass, SuperSaw, Additive, RingMod, PitchBend as they were,
  //  they will now use LPF24 by default from setupBasicSynthSound)


  std::cout << "Stopping synth..." << std::endl;
  err = Pa_StopStream(stream);
  if (err != paNoError) {
    std::cerr << "PortAudio stream stop error: " << Pa_GetErrorText(err)
              << std::endl;
  }
  err = Pa_CloseStream(stream);
  if (err != paNoError) {
    std::cerr << "PortAudio stream close error: " << Pa_GetErrorText(err)
              << std::endl;
  }
  Pa_Terminate();
  std::cout << "Synth stopped." << std::endl;
  return 0;
}