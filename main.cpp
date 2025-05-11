// synth/main.cpp
#include "effects/reverb_effect.h"
#include "envelope.h"
#include "lfo.h"
#include "poly_synth.h"
#include "waveform.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <portaudio.h>
#include <vector> // Required for std::vector

PolySynth synth(44100.0f, 8); 
constexpr float sampleRate = 44100.0f;
ReverbEffect *mainReverb = nullptr; 

int callback(const void *, void *outputBuffer, unsigned long framesPerBuffer,
             const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *) {
  float *out = static_cast<float *>(outputBuffer);
  for (unsigned long i = 0; i < framesPerBuffer; ++i) {
    StereoSample sample = synth.process();
    *out++ = sample.L; // Left channel
    *out++ = sample.R; // Right channel
  }
  return paContinue;
}

void resetPolyModAmounts(PolySynth &s) {
  s.setPMFilterEnvToFreqAAmount(0.0f);
  s.setPMFilterEnvToPWAAmount(0.0f);
  s.setPMFilterEnvToFilterCutoffAmount(0.0f);
  s.setPMOscBToFreqAAmount(0.0f);
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

void setupBasicSynthSound(PolySynth &s, bool resetWheelAndPoly = true) {
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

  if (resetWheelAndPoly) {
    resetPolyModAmounts(s);
    resetWheelModAmounts(s);
  }
  s.setUnisonEnabled(false);
  s.setUnisonDetuneCents(7.0f);
  s.setUnisonStereoSpread(0.7f);


  s.setGlideEnabled(false);
  s.setGlideTime(0.05f);

  s.setMasterTuneCents(0.0f); 
  s.setPitchBend(0.0f); // Reset pitch bend
  s.setPitchBendRange(2.0f); // Default +/- 2 semitones

  if (mainReverb) {
    mainReverb->setEnabled(false); 
  }
  
  s.setAnalogPitchDriftDepth(0.0f);
  s.setAnalogPWDriftDepth(0.0f);
  
  // Reset harmonic amplitudes to default (fundamental only) for Osc1 and Osc2
  // Assuming max 16 harmonics internally. Loop up to that for safety.
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
  // Output 2 channels for stereo
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
  std::cout << "\n--- Basic Sound Check (C4 Saw) ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.noteOn(60, 100); // C4
  Pa_Sleep(1000);
  synth.noteOff(60);
  Pa_Sleep(1000);


  // --- Moog-style Bass Test ---
  std::cout << "\n--- Moog-style Bass Test (Saw, LPF sweep) ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Saw);
  synth.setOsc1Level(1.0f);
  synth.setOsc2Level(0.0f); // Moog typically single osc or slightly detuned second
  synth.setVCFBaseCutoff(300.0f); // Start with low cutoff
  synth.setVCFResonance(0.3f);   // Some resonance
  synth.setVCFEnvelopeAmount(0.8f); // Strong envelope modulation
  EnvelopeParams filterEnvParamsBass = {0.01f, 0.5f, 0.0f, 0.3f}; // Fast attack, med decay, no sustain for pluck/sweep
  synth.setFilterEnvelope(filterEnvParamsBass);
  EnvelopeParams ampEnvParamsBass = {0.01f, 0.8f, 0.0f, 0.5f}; // Similar amp envelope
  synth.setAmpEnvelope(ampEnvParamsBass);

  std::cout << "Playing C3 (MIDI 48) Moog-style bass..." << std::endl;
  synth.noteOn(48, 120); 
  Pa_Sleep(2000);
  synth.noteOff(48);
  Pa_Sleep(1000);

  std::cout << "Playing F3 (MIDI 53) Moog-style bass..." << std::endl;
  synth.noteOn(53, 120);
  Pa_Sleep(2000);
  synth.noteOff(53);
  Pa_Sleep(1000);


  // --- SuperSaw Test ---
  std::cout << "\n--- SuperSaw Test ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Saw); // Base waveform
  synth.setOsc1Level(1.0f);
  synth.setOsc2Level(0.0f); // SuperSaw is typically based on one oscillator type multi-stacked
  synth.setUnisonEnabled(true);
  synth.setUnisonDetuneCents(15.0f); // Moderate detune for thickness
  synth.setUnisonStereoSpread(0.9f); // Wide stereo spread
  // Filter slightly open to let the rich harmonics through
  synth.setVCFBaseCutoff(8000.0f);
  synth.setVCFEnvelopeAmount(0.2f);
  synth.setVCFResonance(0.1f);
  EnvelopeParams ampEnvSuper = {0.05f, 1.0f, 0.8f, 0.5f}; // Longer release for pads
  synth.setAmpEnvelope(ampEnvSuper);

  std::cout << "Playing C4 (MIDI 60) SuperSaw..." << std::endl;
  synth.noteOn(60, 100);
  Pa_Sleep(3000);
  synth.noteOff(60);
  Pa_Sleep(2000);

  std::cout << "Playing G4 (MIDI 67) SuperSaw chord..." << std::endl;
  synth.noteOn(60, 100); // C4
  synth.noteOn(64, 90);  // E4
  synth.noteOn(67, 95);  // G4
  Pa_Sleep(4000);
  synth.noteOff(60);
  synth.noteOff(64);
  synth.noteOff(67);
  Pa_Sleep(2000);
  synth.setUnisonEnabled(false); // Reset for next tests


  // --- Harmonic Control Tests (Additive Waveform) ---
  std::cout << "\n--- Additive Synthesis: Odd Harmonics Emphasis (Square-like) ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Additive);
  synth.setOsc1Level(1.0f);
  synth.setOsc2Level(0.0f);
  for (int i = 0; i < 10; ++i) { // Using 0-9 for harmonic indices
      if (i % 2 == 0) { // Odd harmonics (index 0, 2, 4, ... corresponds to 1st, 3rd, 5th...)
          synth.setOscHarmonicAmplitude(1, i, 1.0f / (i + 1)); // 1/n amplitude for odd
      } else {
          synth.setOscHarmonicAmplitude(1, i, 0.0f); // Zero for even
      }
  }
  std::cout << "Playing C4 (MIDI 60) with odd harmonics..." << std::endl;
  synth.noteOn(60, 100);
  Pa_Sleep(3000);
  synth.noteOff(60);
  Pa_Sleep(1000);

  std::cout << "\n--- Additive Synthesis: High-order Harmonics 1/n Attenuation (Saw-like) ---" << std::endl;
  setupBasicSynthSound(synth); // Resets harmonics
  synth.setWaveform(Waveform::Additive);
  synth.setOsc1Level(1.0f);
  for (int i = 0; i < 10; ++i) {
      synth.setOscHarmonicAmplitude(1, i, 0.6f / (i + 1)); // 0.6 factor to keep overall level down
  }
  std::cout << "Playing C4 (MIDI 60) with 1/n harmonic decay..." << std::endl;
  synth.noteOn(60, 100);
  Pa_Sleep(3000);
  synth.noteOff(60);
  Pa_Sleep(1000);

  std::cout << "\n--- Additive Synthesis: Emphasize 3rd and 5th Harmonics ---" << std::endl;
  setupBasicSynthSound(synth); // Resets harmonics
  synth.setWaveform(Waveform::Additive);
  synth.setOsc1Level(1.0f);
  synth.setOscHarmonicAmplitude(1, 0, 1.0f);  // Fundamental (1st harmonic)
  synth.setOscHarmonicAmplitude(1, 1, 0.1f);  // 2nd harmonic (low)
  synth.setOscHarmonicAmplitude(1, 2, 0.7f);  // 3rd harmonic (emphasized)
  synth.setOscHarmonicAmplitude(1, 3, 0.15f); // 4th harmonic (low)
  synth.setOscHarmonicAmplitude(1, 4, 0.5f);  // 5th harmonic (emphasized)
  for (int i = 5; i < 10; ++i) {
       synth.setOscHarmonicAmplitude(1, i, 0.2f / (i - 3)); // Rapid decay for higher ones
  }
  std::cout << "Playing C4 (MIDI 60) with 3rd & 5th harmonics emphasized..." << std::endl;
  synth.noteOn(60, 100);
  Pa_Sleep(3000);
  synth.noteOff(60);
  Pa_Sleep(1000);


  // --- Ring Modulator Test ---
  std::cout << "\n--- Ring Modulator Test ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setOsc1Level(1.0f); // Modulator
  synth.setOsc2Level(1.0f); // Carrier (though roles are symmetric in RM)
  synth.setRingModLevel(0.8f); // Strong ring modulation
  // To make RM audible, osc1 and osc2 levels in the mixer should typically be low or zero if only RM sound is desired
  // For this test, let's hear RM mixed with original oscs a bit.
  // Let's set original osc levels lower to highlight RM.
  synth.setOsc1Level(0.2f); 
  synth.setOsc2Level(0.2f); 

  synth.setWaveform(Waveform::Sine); // Sine waves make RM effects clearer
  
  // VCO B set to a fixed, non-key-following frequency for classic RM effects
  synth.setVCOBKeyFollowEnabled(false);
  // Set VCO B frequency knob to produce a ratio. E.g., if C4 is 261Hz, this could be around 400Hz.
  // FreqKnob: (0.5 is no change from base). (0.5 + (N_semitones / 60.0f))
  // Let's set VCO B to be a fixed pitch, e.g., equivalent to G4 (392 Hz) when C4 is played for OSC1
  // Base freq for non-key-follow is targetKeyFreq (if note on) or a default.
  // For simplicity in test, after noteOn(60), OSC1 is C4. We want OSC2 fixed.
  // We can use setVCOBFreqKnob to detune it significantly from OSC1's pitch.
  // To achieve a specific fixed frequency for Osc2 is a bit indirect with current controls
  // if KeyFollow is off AND vcoBFixedBaseFreq_ is not explicitly set by some other mechanism.
  // Let's try setting VCO B Freq Knob for a ~ M3 interval if keyfollow was on, then turn KF off.
  synth.setVCOBFreqKnob(0.5f + (4.0f / 60.0f)); // Approx +4 semitones offset if base was same

  std::cout << "Playing C4 (MIDI 60) with Ring Modulation (Sine waves)..." << std::endl;
  synth.noteOn(60, 100); // Osc1 will be C4. Osc2 will be C4+offset if KF on, or fixed if KF off.
                         // With KF off, vcoBFixedBaseFreq_ was set to C4 on noteOn.
                         // Then FreqKnob applies offset. So Osc2 = C4 + 4 semitones = E4 approx.
  Pa_Sleep(3000);
  synth.noteOff(60);
  Pa_Sleep(1000);

  std::cout << "Playing C4 (MIDI 60) with Ring Modulation (Saw waves for harsher RM)..." << std::endl;
  synth.setWaveform(Waveform::Saw);
  synth.noteOn(60,100);
  Pa_Sleep(3000);
  synth.noteOff(60);
  Pa_Sleep(1000);


  // --- Pitch Bend Test (already exists, ensure it works with stereo) ---
  std::cout << "\n--- Pitch Bend Test (Stereo Check) ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setOsc1Level(1.0f);
  synth.setOsc2Level(0.0f);
  synth.setWaveform(Waveform::Saw);
  synth.setPitchBendRange(2.0f); 

  std::cout << "Playing C4 (MIDI 60). Bending up..." << std::endl;
  synth.noteOn(60, 100); 
  Pa_Sleep(500);
  for (int i = 0; i <= 100; ++i) {
      synth.setPitchBend(static_cast<float>(i) / 100.0f); 
      Pa_Sleep(10); 
  }
  Pa_Sleep(500);
  synth.noteOff(60);
  Pa_Sleep(500);
  synth.setPitchBend(0.0f); 
  
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