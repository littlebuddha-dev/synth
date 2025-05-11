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
#include <vector>   // For std::vector in playNoteSequence
#include <utility>  // For std::pair in playNoteSequence

PolySynth synth(44100.0f, 8); // グローバル synth オブジェクト
constexpr float sampleRate = 44100.0f;
ReverbEffect *mainReverb = nullptr; // グローバルポインタ

// --- PortAudio Callback ---
int audioCallback(const void *inputBuffer, void *outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo *timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void *userData) {
    float *out = static_cast<float *>(outputBuffer);
    (void)inputBuffer; // Unused

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        out[i] = synth.process();
    }
    return paContinue;
}

// --- Synth Reset Utilities ---
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

void setupBasicSynthSound(PolySynth &s, bool resetMods = true) {
    s.setWaveform(Waveform::Saw);
    s.setOsc1Level(1.0f);
    s.setOsc2Level(0.0f);
    s.setNoiseLevel(0.0f);
    s.setVCOBDetuneCents(0.0f);
    s.setSyncEnabled(false);
    s.setPulseWidth(0.5f);
    s.setVCOBLowFreqEnabled(false);
    s.setVCOBFreqKnob(0.5f);

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

    s.setLfoRate(0.0f); // LFO off by default for basic sound
    s.setLfoWaveform(LfoWaveform::Triangle);
    s.setLfoAmountToVco1Freq(0.0f);
    s.setLfoAmountToVco2Freq(0.0f);
    s.setLfoAmountToVco1Pw(0.0f);
    s.setLfoAmountToVco2Pw(0.0f);
    s.setLfoAmountToVcfCutoff(0.0f);

    if (resetMods) {
        resetPolyModAmounts(s);
        resetWheelModAmounts(s);
    }

    s.setUnisonEnabled(false);
    s.setUnisonDetuneCents(7.0f);
    s.setGlideEnabled(false);
    s.setGlideTime(0.05f);
    s.setMasterTuneCents(0.0f);
    s.setAnalogPitchDriftDepth(0.0f);
    s.setAnalogPWDriftDepth(0.0f);

    if (mainReverb) {
        mainReverb->setEnabled(false);
    }
}

// --- Test Helper Functions ---
const int C4 = 60;
const int D4 = 62;
const int E4 = 64;
const int F4 = 65;
const int G4 = 67;
const int A4 = 69;
const int C5 = 72;
const int C3 = 48;


// Plays a single note for a duration, then releases
void playNote(PolySynth& s, int midiNote, float velocity, int durationMs, int releaseMs = 500) {
    s.noteOn(midiNote, velocity);
    Pa_Sleep(durationMs);
    s.noteOff(midiNote);
    Pa_Sleep(releaseMs);
}

// Structure to define a note event for sequences
struct NoteEvent {
    int midiNote;
    float velocity;
    int actionDelayMs; // Delay *before* this action
    bool isNoteOn;     // true for noteOn, false for noteOff
    int holdDurationMs; // Only for noteOn: how long to hold before auto-releasing (if next event is far)
                        // Not perfectly implemented here, simplified to just noteOn/noteOff events
};


// --- Individual Test Functions ---

void testBasicSound(PolySynth& s) {
    std::cout << "\n--- Test: Basic Sound (All Mod Off, Glide Off, Master Tune 0) ---" << std::endl;
    setupBasicSynthSound(s);
    playNote(s, C4, 100.0f, 1500);
}

void testMasterTune(PolySynth& s) {
    std::cout << "\n--- Test Section: Master Tune ---" << std::endl;
    setupBasicSynthSound(s);

    std::cout << "  Master Tune: +50 Cents" << std::endl;
    s.setMasterTuneCents(50.0f);
    s.noteOn(C4, 100.0f); Pa_Sleep(700);
    s.noteOn(G4, 100.0f); Pa_Sleep(1500);
    s.noteOff(C4); s.noteOff(G4); Pa_Sleep(1000);

    std::cout << "  Master Tune: -30 Cents" << std::endl;
    s.setMasterTuneCents(-30.0f);
    playNote(s, C4, 100.0f, 1500);

    std::cout << "  Master Tune: 0 Cents (Back to standard)" << std::endl;
    s.setMasterTuneCents(0.0f);
    playNote(s, C4, 100.0f, 1500);

    std::cout << "  Master Tune: Unison with Tune +20 Cents" << std::endl;
    s.setMasterTuneCents(20.0f);
    s.setUnisonEnabled(true);
    s.setUnisonDetuneCents(10.0f);
    playNote(s, C4, 100.0f, 2000);
    s.setUnisonEnabled(false); // Reset unison
    s.setMasterTuneCents(0.0f);  // Reset tune
}

void testGlide(PolySynth& s) {
    std::cout << "\n--- Test Section: Glide ---" << std::endl;
    setupBasicSynthSound(s);
    s.setGlideEnabled(true);

    std::cout << "  Glide: Slow (0.5s) Upwards (C3 -> C4)" << std::endl;
    s.setGlideTime(0.5f);
    s.noteOn(C3, 100.0f); Pa_Sleep(700);
    s.noteOn(C4, 100.0f); Pa_Sleep(2000);
    s.noteOff(C4); Pa_Sleep(1000); // Release last playing note

    std::cout << "  Glide: Fast (0.05s) Downwards (C5 -> C4 -> G4)" << std::endl;
    s.setGlideTime(0.05f);
    s.noteOn(C5, 100.0f); Pa_Sleep(700);
    s.noteOn(C4, 100.0f); Pa_Sleep(700);
    s.noteOn(G4, 100.0f); Pa_Sleep(1500);
    s.noteOff(G4); Pa_Sleep(1000);

    std::cout << "  Glide: Quick note changes (0.15s)" << std::endl;
    s.setGlideTime(0.15f);
    s.noteOn(C4, 100.0f); Pa_Sleep(300);
    s.noteOn(D4, 100.0f); Pa_Sleep(300);
    s.noteOn(E4, 100.0f); Pa_Sleep(300);
    s.noteOn(F4, 100.0f); Pa_Sleep(300);
    s.noteOn(G4, 100.0f); Pa_Sleep(1500);
    s.noteOff(G4); Pa_Sleep(1000);
    
    std::cout << "  Glide: Off (check no glide C4 -> C5)" << std::endl;
    s.setGlideEnabled(false);
    s.noteOn(C4, 100.0f); Pa_Sleep(700);
    s.noteOn(C5, 100.0f); Pa_Sleep(1500);
    s.noteOff(C5); Pa_Sleep(1000);
}

void testPolyMod(PolySynth& s) {
    std::cout << "\n--- Test Section: PolyMod ---" << std::endl;
    
    std::cout << "  PolyMod: Filter Env -> Freq A" << std::endl;
    setupBasicSynthSound(s);
    s.setPMFilterEnvToFreqAAmount(0.3f);
    playNote(s, C4, 100.0f, 2000);

    std::cout << "  PolyMod: OSC B -> Freq A (Cross Mod)" << std::endl;
    setupBasicSynthSound(s);
    s.setOsc1Level(1.0f);
    s.setOsc2Level(0.0f); // OSC B produces modulation, not direct sound
    s.setVCOBLowFreqEnabled(false);
    s.setVCOBFreqKnob(0.52f); // OSC B slightly detuned to hear effect
    s.setPMOscBToFreqAAmount(0.2f);
    playNote(s, C3, 100.0f, 2000);

    std::cout << "  PolyMod: Filter Env -> PW A" << std::endl;
    setupBasicSynthSound(s);
    s.setWaveform(Waveform::Pulse);
    s.setPMFilterEnvToPWAAmount(0.9f);
    playNote(s, C4, 100.0f, 2000);

    std::cout << "  PolyMod: OSC B -> PW A" << std::endl;
    setupBasicSynthSound(s);
    s.setWaveform(Waveform::Pulse);
    s.setOsc2Level(0.0f);
    s.setVCOBLowFreqEnabled(false);
    s.setVCOBFreqKnob(0.3f); // OSC B as LFO-ish rate
    s.setPMOscBToPWAAmount(0.8f);
    playNote(s, C4, 100.0f, 2000);

    std::cout << "  PolyMod: Filter Env -> Filter Cutoff" << std::endl;
    setupBasicSynthSound(s);
    s.setVCFBaseCutoff(500.0f);
    s.setVCFEnvelopeAmount(0.0f); // Turn off standard filter env mod
    s.setPMFilterEnvToFilterCutoffAmount(0.9f);
    playNote(s, C4, 100.0f, 2000);

    std::cout << "  PolyMod: OSC B -> Filter Cutoff" << std::endl;
    setupBasicSynthSound(s);
    s.setVCFBaseCutoff(1000.0f);
    s.setVCFEnvelopeAmount(0.0f);
    s.setOsc2Level(0.0f);
    s.setVCOBLowFreqEnabled(true); // OSC B as LFO
    s.setVCOBFreqKnob(0.2f);      // Low LFO rate
    s.setPMOscBToFilterCutoffAmount(0.7f);
    playNote(s, C4, 100.0f, 3000);
}

void testWheelModulation(PolySynth& s) {
    std::cout << "\n--- Test Section: Wheel Modulation ---" << std::endl;

    std::cout << "  WheelMod: LFO -> Freq A (Vibrato)" << std::endl;
    setupBasicSynthSound(s);
    s.setWheelModSource(WheelModSource::LFO);
    s.setLfoRate(6.0f);
    s.setLfoWaveform(LfoWaveform::Sine);
    s.setWheelModAmountToFreqA(0.7f); // Wheel controls LFO depth to pitch

    s.setModulationWheelValue(0.0f); std::cout << "    Wheel @ 0.0" << std::endl;
    s.noteOn(C4, 100.0f); Pa_Sleep(1500);
    s.setModulationWheelValue(0.5f); std::cout << "    Wheel @ 0.5" << std::endl;
    Pa_Sleep(1500);
    s.setModulationWheelValue(1.0f); std::cout << "    Wheel @ 1.0" << std::endl;
    Pa_Sleep(2000);
    s.noteOff(C4); Pa_Sleep(1000);

    std::cout << "  WheelMod: LFO -> Filter Cutoff" << std::endl;
    setupBasicSynthSound(s);
    s.setLfoRate(1.0f); // Slow LFO for filter sweep
    s.setWheelModSource(WheelModSource::LFO);
    s.setWheelModAmountToFilter(0.8f);
    s.setVCFBaseCutoff(800.0f);

    s.setModulationWheelValue(0.0f); std::cout << "    Wheel @ 0.0" << std::endl;
    s.noteOn(C4, 100.0f); Pa_Sleep(1500);
    s.setModulationWheelValue(0.7f); std::cout << "    Wheel @ 0.7" << std::endl;
    Pa_Sleep(2000);
    s.setModulationWheelValue(0.0f); std::cout << "    Wheel @ 0.0 (down)" << std::endl;
    Pa_Sleep(1500);
    s.noteOff(C4); Pa_Sleep(1000);

    std::cout << "  WheelMod: NOISE -> Filter Cutoff" << std::endl;
    setupBasicSynthSound(s);
    s.setWheelModSource(WheelModSource::NOISE);
    s.setWheelModAmountToFilter(1.0f); // Full depth noise to filter
    s.setVCFBaseCutoff(300.0f);
    s.setVCFResonance(0.5f);

    s.setModulationWheelValue(0.0f); std::cout << "    Wheel @ 0.0" << std::endl;
    s.noteOn(C4, 100.0f); Pa_Sleep(1500);
    s.setModulationWheelValue(1.0f); std::cout << "    Wheel @ 1.0" << std::endl;
    Pa_Sleep(2000);
    s.noteOff(C4); Pa_Sleep(1000);

    std::cout << "  WheelMod: LFO -> PW A" << std::endl;
    setupBasicSynthSound(s);
    s.setWaveform(Waveform::Pulse);
    s.setLfoRate(3.0f);
    s.setWheelModSource(WheelModSource::LFO);
    s.setWheelModAmountToPWA(1.0f); // Full depth LFO to PW via wheel

    s.setModulationWheelValue(0.0f); std::cout << "    Wheel @ 0.0" << std::endl;
    s.noteOn(C4, 100.0f); Pa_Sleep(1500);
    s.setModulationWheelValue(1.0f); std::cout << "    Wheel @ 1.0" << std::endl;
    Pa_Sleep(2000);
    s.noteOff(C4); Pa_Sleep(1000);
}

void testUnison(PolySynth& s) {
    std::cout << "\n--- Test Section: Unison Mode ---" << std::endl;
    setupBasicSynthSound(s);
    s.setOsc1Level(1.0f);
    s.setOsc2Level(0.0f); // Focus on unison effect
    s.setUnisonEnabled(true);

    std::cout << "  Unison: Detune 0 cents" << std::endl;
    s.setUnisonDetuneCents(0.0f);
    playNote(s, C4, 100.0f, 2000);

    std::cout << "  Unison: Detune 25 cents" << std::endl;
    s.setUnisonDetuneCents(25.0f);
    playNote(s, G4, 100.0f, 2000);

    std::cout << "  Unison: Legato (A4 -> C5), Detune 10 cents" << std::endl;
    s.setUnisonDetuneCents(10.0f);
    s.noteOn(A4, 100.0f); Pa_Sleep(1500);
    s.noteOn(C5, 100.0f); Pa_Sleep(2000); // Should retrigger all voices for C5
    s.noteOff(C5); Pa_Sleep(1000);

    std::cout << "  Unison: Glide On (0.2s), Detune 10 cents" << std::endl;
    s.setGlideEnabled(true);
    s.setGlideTime(0.2f);
    s.setUnisonDetuneCents(10.0f);
    s.noteOn(C4, 100.0f); Pa_Sleep(500);
    s.noteOn(F4, 100.0f); Pa_Sleep(2000);
    s.noteOff(F4); Pa_Sleep(1000);
    
    s.setGlideEnabled(false);
    s.setUnisonEnabled(false); // Turn off unison for next tests
}

void testPolyphony(PolySynth& s) {
    std::cout << "\n--- Test Section: Polyphony (Unison Off) ---" << std::endl;
    setupBasicSynthSound(s);
    s.setOsc2Level(0.7f); // Add OSC2 for richer chord
    s.setAmpVelocitySensitivity(0.8f);

    std::cout << "  Polyphony: Playing a C Major chord (C4, E4, G4)" << std::endl;
    s.noteOn(C4, 100.0f); Pa_Sleep(100); // Stagger for distinct attacks
    s.noteOn(E4, 90.0f);  Pa_Sleep(100);
    s.noteOn(G4, 80.0f);  Pa_Sleep(2000);

    std::cout << "  Polyphony: Releasing notes one by one" << std::endl;
    s.noteOff(C4); Pa_Sleep(500);
    s.noteOff(E4); Pa_Sleep(500);
    s.noteOff(G4); Pa_Sleep(1000);
}

void testReverb(PolySynth& s, ReverbEffect* reverb) {
    std::cout << "\n--- Test Section: Reverb Effect ---" << std::endl;
    setupBasicSynthSound(s); // Reverb will be off initially

    if (!reverb) {
        std::cout << "  Reverb effect not available for testing." << std::endl;
        return;
    }

    std::cout << "  Reverb: DryWet 0.9, RoomSize 0.9, Damping 0.8" << std::endl;
    reverb->setEnabled(true);
    reverb->setDryWetMix(0.9f);
    reverb->setRoomSize(0.9f);
    reverb->setDamping(0.8f);
    reverb->setRT60(2.5f); // Set a specific RT60
    playNote(s, C4, 100.0f, 1500, 3000); // Longer release to hear tail

    std::cout << "  Reverb: DryWet 0.5, RoomSize 0.5, Damping 0.2 (shorter)" << std::endl;
    reverb->setDryWetMix(0.5f);
    reverb->setRoomSize(0.5f);
    reverb->setDamping(0.2f);
    reverb->setRT60(1.0f);
    playNote(s, G4, 100.0f, 1500, 2000);

    reverb->setEnabled(false); // Turn off reverb after test
}

void testAnalogDrift(PolySynth& s) {
    std::cout << "\n--- Test Section: Analog Drift ---" << std::endl;
    setupBasicSynthSound(s);
    s.setWaveform(Waveform::Pulse);
    s.setOsc1Level(1.0);
    s.setOsc2Level(0.0);

    std::cout << "  Analog Drift: Pitch +/-2.5c, PW +/-0.15" << std::endl;
    s.setAnalogPitchDriftDepth(2.5f);
    s.setAnalogPWDriftDepth(0.15f);
    playNote(s, C4, 100.0f, 5000, 1000);

    std::cout << "  Analog Drift: Stronger - Pitch +/-7.0c, PW +/-0.35" << std::endl;
    s.setAnalogPitchDriftDepth(7.0f);
    s.setAnalogPWDriftDepth(0.35f);
    playNote(s, C4, 100.0f, 5000, 1000);

    // Reset drift after test
    s.setAnalogPitchDriftDepth(0.0f);
    s.setAnalogPWDriftDepth(0.0f);
}


// --- Main Application ---
int main() {
    if (Pa_Initialize() != paNoError) {
        std::cerr << "PortAudio initialization error!" << std::endl;
        return 1;
    }

    // Initialize Reverb
    auto reverbInstance = std::make_unique<ReverbEffect>(sampleRate);
    mainReverb = reverbInstance.get();
    synth.addEffect(std::move(reverbInstance));

    PaStream *stream;
    PaError err = Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, sampleRate,
                                     256, audioCallback, nullptr);
    if (err != paNoError) {
        std::cerr << "PortAudio stream open error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream start error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    std::cout << "Synth Test Suite Running..." << std::endl;
    std::cout << "Press Enter to stop if interactive, or wait for tests to complete." << std::endl;

    // --- Run Test Suites ---
    testBasicSound(synth);
    testMasterTune(synth);
    testGlide(synth);
    testPolyMod(synth);
    testWheelModulation(synth);
    testUnison(synth);
    testPolyphony(synth);
    testReverb(synth, mainReverb); // Pass the global reverb pointer
    testAnalogDrift(synth);
    
    std::cout << "\nAll tests completed." << std::endl;

    // --- Cleanup ---
    std::cout << "Stopping synth..." << std::endl;
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream stop error: " << Pa_GetErrorText(err) << std::endl;
    }
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream close error: " << Pa_GetErrorText(err) << std::endl;
    }
    Pa_Terminate();
    std::cout << "Synth stopped." << std::endl;
    return 0;
}