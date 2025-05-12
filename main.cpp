// synth/main.cpp
#include "effects/reverb_effect.h"
#include "envelope.h"
#include "lfo.h"
#include "poly_synth.h"
#include "waveform.h"
#include "synth_parameters.h" 
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
  s.setFilterType(SynthParams::FilterType::LPF24); 
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

  s.setMixerDrive(0.0f);      
  s.setMixerPostGain(1.0f);   

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

  if (mainReverb) { // Ensure mainReverb is not null before using
    mainReverb->setEnabled(false); // Default reverb to off
    // Set default reverb parameters that sound reasonable when turned on
    mainReverb->setDryWetMix(0.3f);
    mainReverb->setRoomSize(0.5f);
    mainReverb->setDamping(0.5f);
    mainReverb->setWetGain(1.0f);
    mainReverb->setRT60(1.2f);
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
  setupBasicSynthSound(synth); // Reverb is OFF by default here
  synth.noteOn(60, 100); 
  Pa_Sleep(1000);
  synth.noteOff(60);
  Pa_Sleep(1000);

  // --- Mixer Overdrive Test ---
  std::cout << "\n--- Mixer Overdrive Test (Sine Wave C3) ---" << std::endl;
  setupBasicSynthSound(synth);
  synth.setWaveform(Waveform::Sine);
  synth.setOsc1Level(0.7f); 
  synth.setVCFBaseCutoff(20000.0f); 
  synth.setVCFEnvelopeAmount(0.0f); 

  std::cout << "Mixer Drive: 0.0 (Clean Sine)" << std::endl;
  synth.setMixerDrive(0.0f);
  synth.setMixerPostGain(1.0f); 
  synth.noteOn(48, 120); 
  Pa_Sleep(1500);
  synth.noteOff(48);
  Pa_Sleep(500);

  std::cout << "Mixer Drive: 0.7 (Medium Drive)" << std::endl;
  synth.setMixerDrive(0.7f);
  synth.setMixerPostGain(0.5f); 
  synth.noteOn(48, 120);
  Pa_Sleep(1500);
  synth.noteOff(48);
  Pa_Sleep(1000);
  
  // --- Filter Type Tests ---
  std::cout << "\n--- Filter Type Tests (Saw wave, sweeping cutoff with ENV) ---" << std::endl;
  SynthParams::FilterType typesToTest[] = {
      SynthParams::FilterType::LPF24, SynthParams::FilterType::LPF12,
      SynthParams::FilterType::HPF12, SynthParams::FilterType::BPF12,
      SynthParams::FilterType::NOTCH
  };
  const char* typeNames[] = {"LPF24", "LPF12", "HPF12", "BPF12", "NOTCH"};
  for (int i = 0; i < 5; ++i) {
      std::cout << "Testing Filter Type: " << typeNames[i] << std::endl;
      setupBasicSynthSound(synth); 
      synth.setFilterType(typesToTest[i]);
      synth.setWaveform(Waveform::Saw);
      synth.setOsc1Level(0.5f); // Lowered level a bit for filter tests
      synth.setVCFBaseCutoff(typesToTest[i] == SynthParams::FilterType::HPF12 ? 100.0f : 8000.0f); 
      synth.setVCFResonance(0.4f); 
      synth.setVCFEnvelopeAmount(typesToTest[i] == SynthParams::FilterType::HPF12 ? -0.7f : 0.7f); 
      EnvelopeParams filterEnvSweep = {0.1f, 1.0f, 0.0f, 0.5f};
      synth.setFilterEnvelope(filterEnvSweep);
      synth.noteOn(48, 110); 
      Pa_Sleep(2500);
      synth.noteOff(48);
      Pa_Sleep(1000);
  }
  
  // --- Reverb Tests ---
  std::cout << "\n--- Reverb Tests (Short Sawtooth Note C4) ---" << std::endl;
  setupBasicSynthSound(synth); // Reverb is off, basic sound
  synth.setWaveform(Waveform::Saw);
  synth.setOsc1Level(0.6f);
  EnvelopeParams shortAmpEnv = {0.01f, 0.1f, 0.0f, 0.2f}; // Short staccato note
  synth.setAmpEnvelope(shortAmpEnv);

  std::cout << "Reverb OFF" << std::endl;
  if(mainReverb) mainReverb->setEnabled(false);
  synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(1000);

  if(mainReverb) { // Only proceed if reverb exists
    std::cout << "Reverb ON (Default values)" << std::endl;
    mainReverb->setEnabled(true);
    mainReverb->setDryWetMix(0.3f); mainReverb->setRoomSize(0.5f); mainReverb->setDamping(0.5f);
    mainReverb->setWetGain(1.0f); mainReverb->setRT60(1.2f);
    synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(2000);

    std::cout << "Reverb - Short RT60 (0.3s)" << std::endl;
    mainReverb->setRT60(0.3f);
    synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(1500);

    std::cout << "Reverb - Long RT60 (3.5s)" << std::endl;
    mainReverb->setRT60(3.5f);
    synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(4000);
    mainReverb->setRT60(1.2f); // Reset

    std::cout << "Reverb - Small RoomSize (0.1)" << std::endl;
    mainReverb->setRoomSize(0.1f);
    synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(2000);

    std::cout << "Reverb - Large RoomSize (0.9)" << std::endl;
    mainReverb->setRoomSize(0.9f);
    synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(2000);
    mainReverb->setRoomSize(0.5f); // Reset

    std::cout << "Reverb - Low Damping (0.1 - bright)" << std::endl;
    mainReverb->setDamping(0.1f);
    synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(2000);

    std::cout << "Reverb - High Damping (0.9 - dark)" << std::endl;
    mainReverb->setDamping(0.9f);
    synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(2000);
    mainReverb->setDamping(0.5f); // Reset

    std::cout << "Reverb - High Dry/Wet (0.7)" << std::endl;
    mainReverb->setDryWetMix(0.7f);
    synth.noteOn(60, 127); Pa_Sleep(500); synth.noteOff(60); Pa_Sleep(2000);
    mainReverb->setDryWetMix(0.3f); // Reset

    mainReverb->setEnabled(false); // Turn off reverb for subsequent tests
  }
  // --- End Reverb Tests ---


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