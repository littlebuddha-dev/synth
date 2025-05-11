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

PolySynth synth(44100.0f, 8); 
constexpr float sampleRate = 44100.0f;
ReverbEffect *mainReverb = nullptr; 

int callback(const void *, void *out, unsigned long frames,
             const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *) {
  float *outBuffer = static_cast<float *>(out);
  for (unsigned long i = 0; i < frames; ++i) {
    outBuffer[i] = synth.process();
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
  
  for (int i = 0; i < 10; ++i) {
      s.setOscHarmonicAmplitude(1, i, (i == 0) ? 1.0f : 0.0f);
  }
  for (int i = 0; i < 10; ++i) { 
      s.setOscHarmonicAmplitude(2, i, (i == 0) ? 1.0f : 0.0f);
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
  PaError err = Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, sampleRate, 256,
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

  std::cout << "Synth running... Press Enter to stop (if interactive)."
            << std::endl;

  setupBasicSynthSound(synth);

  // --- Pitch Bend Test ---
  std::cout << "\n--- Pitch Bend Test ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setOsc1Level(1.0f);
  synth.setOsc2Level(0.0f);
  synth.setWaveform(Waveform::Saw);
  synth.setPitchBendRange(2.0f); // +/- 2 semitones

  std::cout << "Playing C4 (MIDI 60). Bending up..." << std::endl;
  synth.noteOn(60, 100); // C4
  Pa_Sleep(1000);
  for (int i = 0; i <= 100; ++i) {
      synth.setPitchBend(static_cast<float>(i) / 100.0f); // Bend up to +2 semitones
      Pa_Sleep(10); // Smooth bend over 1 second
  }
  Pa_Sleep(1000);

  std::cout << "Bending down..." << std::endl;
  for (int i = 100; i >= -100; --i) {
      synth.setPitchBend(static_cast<float>(i) / 100.0f); // Bend down to -2 semitones
      Pa_Sleep(10); // Smooth bend over 2 seconds
  }
  Pa_Sleep(1000);

  std::cout << "Returning to center..." << std::endl;
  for (int i = -100; i <= 0; ++i) {
      synth.setPitchBend(static_cast<float>(i) / 100.0f); // Bend back to center
      Pa_Sleep(10); // Smooth bend over 1 second
  }
  Pa_Sleep(1000);
  synth.noteOff(60);
  Pa_Sleep(500);
  synth.setPitchBend(0.0f); // Ensure it's reset
  // --- End Pitch Bend Test ---


  // --- LFO Random Step (Sample & Hold) Test ---
  std::cout << "\n--- LFO Random Step (S&H) Test ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Sine); 
  synth.setOsc1Level(1.0f);
  synth.setOsc2Level(0.0f);
  synth.setVCFBaseCutoff(8000.0f); 
  synth.setVCFEnvelopeAmount(0.0f); 

  synth.setLfoWaveform(LfoWaveform::RandomStep);
  synth.setLfoRate(4.0f); 
  synth.setLfoAmountToVco1Freq(6.0f); 

  std::cout << "Playing C4 (MIDI 60) with LFO RandomStep modulating VCO1 pitch (4Hz, +/-6 semitones)..." << std::endl;
  synth.noteOn(60, 100); 
  Pa_Sleep(5000); 
  synth.noteOff(60);
  Pa_Sleep(1000);

  std::cout << "LFO RandomStep modulating VCF Cutoff (8Hz, +/-1000Hz)..." << std::endl;
  synth.setLfoAmountToVco1Freq(0.0f); 
  synth.setLfoAmountToVcfCutoff(1000.0f); 
  synth.setLfoRate(8.0f); 
  synth.setVCFBaseCutoff(2000.0f); 
  synth.setWaveform(Waveform::Saw); 

  synth.noteOn(60, 100);
  Pa_Sleep(5000);
  synth.noteOff(60);
  Pa_Sleep(1000);

  setupBasicSynthSound(synth); 
  // --- End LFO Random Step Test ---


  // --- VCO B Key Follow Test ---
  std::cout << "\n--- VCO B Key Follow Test ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Saw); 
  synth.setOsc1Level(0.7f);
  synth.setOsc2Level(0.7f);
  synth.setVCOBDetuneCents(0.0f);   
  synth.setVCOBFreqKnob(0.5f);     

  std::cout << "Test 1: Key Follow ON (default). Play C4, then G4. Both VCOs should track." << std::endl;
  synth.setVCOBKeyFollowEnabled(true);
  synth.noteOn(60, 100); 
  Pa_Sleep(1500);
  synth.noteOff(60);
  Pa_Sleep(200);
  synth.noteOn(67, 100); 
  Pa_Sleep(1500);
  synth.noteOff(67);
  Pa_Sleep(500);

  std::cout << "Test 2: Key Follow OFF. Play C4 (fixed pitch for VCO B set here), then G4." << std::endl;
  synth.setVCOBKeyFollowEnabled(false);
  synth.noteOn(60, 100); 
  std::cout << "  VCO B fixed pitch should now be based on C4." << std::endl;
  Pa_Sleep(1500);
  synth.noteOff(60);
  Pa_Sleep(200);

  std::cout << "  Playing G4. VCO A tracks to G4, VCO B should remain at C4 pitch." << std::endl;
  synth.noteOn(67, 100); 
  Pa_Sleep(1500);
  synth.noteOff(67);
  Pa_Sleep(500);

  std::cout << "Test 3: Key Follow OFF, VCO B Freq Knob to +1 Octave. Play C4, then G4." << std::endl;
  synth.setVCOBFreqKnob(0.5f + (12.0f / 60.0f));                                                 
  std::cout << "  VCO B Freq Knob set to make VCO B one octave above its base (C4 -> C5)." << std::endl;
  synth.noteOn(60, 100); 
  Pa_Sleep(1500);
  synth.noteOff(60);
  Pa_Sleep(200);
  std::cout << "  Playing G4. VCO A tracks to G4, VCO B should remain at C5." << std::endl;
  synth.noteOn(67, 100); 
  Pa_Sleep(1500);
  synth.noteOff(67);
  Pa_Sleep(500);
  
  setupBasicSynthSound(synth); 
  


  std::cout << "\n--- Starting Reverb Effect Tests ---" << std::endl;
  setupBasicSynthSound(synth);                                
  if (mainReverb) {
    std::cout << "MAIN: Setting up reverb for test..." << std::endl; 
    mainReverb->setEnabled(true); 
    mainReverb->setDryWetMix(0.9f); 
    mainReverb->setRoomSize(0.9f);
    mainReverb->setDamping(0.8f);
    std::cout << "MAIN: Reverb isEnabled: " << mainReverb->isEnabled()
              << std::endl; 
  } else {
    std::cout << "MAIN: mainReverb is NULL!" << std::endl; 
  }
  synth.noteOn(60, 100.0f);
  Pa_Sleep(2500);
  synth.noteOff(60);
  Pa_Sleep(2000);

  std::cout << "\n--- Reverb Effect Test 2 ---" << std::endl;
  if (mainReverb) {
    mainReverb->setEnabled(true);
    mainReverb->setDryWetMix(0.9f); 
    mainReverb->setRoomSize(0.9f);  
    mainReverb->setDamping(0.5f);   
  }
  synth.noteOn(67, 100.0f);
  Pa_Sleep(2500);
  synth.noteOff(67);
  Pa_Sleep(2000);

  if (mainReverb) {
    mainReverb->setEnabled(false); 
  }


  std::cout << "\n--- Analog Drift Test ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Pulse); 
  synth.setOsc1Level(1.0);
  synth.setOsc2Level(0.0); 
  synth.setAnalogPitchDriftDepth(2.5f); 
  synth.setAnalogPWDriftDepth(0.15f);   
  
  std::cout << "Playing note C4 (60) with analog drift for 5 seconds..." << std::endl;
  synth.noteOn(60, 100);
  Pa_Sleep(5000); 
  synth.noteOff(60);
  Pa_Sleep(1000);
  synth.setAnalogPitchDriftDepth(0.0f);
  synth.setAnalogPWDriftDepth(0.0f);
  
  
  std::cout << "\n--- Analog Drift Test (Stronger) ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Pulse); 
  synth.setOsc1Level(1.0);
  synth.setOsc2Level(0.0); 

  float strongPitchDriftCents = 7.0f; 
  float strongPWDriftDepth = 0.35f;   

  synth.setAnalogPitchDriftDepth(strongPitchDriftCents);
  synth.setAnalogPWDriftDepth(strongPWDriftDepth);   
  
  std::cout << "Playing note C4 (60) with stronger analog drift for 5 seconds..." << std::endl;
  synth.noteOn(60, 100);
  Pa_Sleep(5000); 
  synth.noteOff(60);
  Pa_Sleep(1000);
  setupBasicSynthSound(synth); 

  std::cout << "\n--- Additive Synthesis Test ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Additive); 
  synth.setOsc1Level(1.0f);
  synth.setOsc2Level(0.0f); 

  for (int i = 0; i < 10; ++i) {
      synth.setOscHarmonicAmplitude(1, i, 0.0f);
  }

  std::cout << "Playing C4 (MIDI 60) with fundamental only..." << std::endl;
  synth.setOscHarmonicAmplitude(1, 0, 1.0f); 
  synth.noteOn(60, 100);
  Pa_Sleep(2000);
  synth.noteOff(60);
  Pa_Sleep(500);

  std::cout << "Playing C4 with fundamental and 3rd harmonic (square-ish)..." << std::endl;
  synth.setOscHarmonicAmplitude(1, 0, 1.0f); 
  synth.setOscHarmonicAmplitude(1, 2, 0.5f); 
  synth.noteOn(60, 100);
  Pa_Sleep(2000);
  synth.noteOff(60);
  Pa_Sleep(500);
  synth.setOscHarmonicAmplitude(1, 2, 0.0f); 

  std::cout << "Playing C4 with first 5 odd harmonics (approximating square)..." << std::endl;
  synth.setOscHarmonicAmplitude(1, 0, 1.0f);    
  synth.setOscHarmonicAmplitude(1, 2, 1.0f/3.0f); 
  synth.setOscHarmonicAmplitude(1, 4, 1.0f/5.0f); 
  synth.setOscHarmonicAmplitude(1, 6, 1.0f/7.0f); 
  synth.setOscHarmonicAmplitude(1, 8, 1.0f/9.0f); 
  synth.noteOn(60, 100);
  Pa_Sleep(3000);
  synth.noteOff(60);
  Pa_Sleep(500);
  
  setupBasicSynthSound(synth);
  
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