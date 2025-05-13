// synth/main.cpp
#include "poly_synth.h"
#include "effects/reverb_effect.h"
#include "waveform.h"
#include "synth_parameters.h"
#include "envelope.h" // For EnvelopeParams in JSON loading
#include "lfo.h"      // For LfoWaveform in JSON loading

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm> // For std::min, std::max
#include <cmath>     // For std::pow, std::log, std::exp

#include <portaudio.h>

// External library headers (ensure these are in your include path)
#include "nlohmann/json.hpp" // For JSON parsing
#include "rtmidi/RtMidi.h"          // For MIDI input
#include "MidiFile.h"        // For MIDI file playback

// Global synth instance
PolySynth synth(44100, 16); // Default sample rate and max voices
ReverbEffect* mainReverbPtr = nullptr; // Pointer to the reverb effect

// PortAudio callback function (remains largely the same)
int audioCallback(const void* /*inputBuffer*/, void* outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo* /*timeInfo*/,
                  PaStreamCallbackFlags /*statusFlags*/,
                  void* /*userData*/) {
    float* out = static_cast<float*>(outputBuffer);
    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        StereoSample sample = synth.process();
        *out++ = sample.L;
        *out++ = sample.R;
    }
    return paContinue;
}

// --- Helper functions from original main, slightly adapted ---
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


// --- Default Sound Configuration ---
void loadDefaultStringsSound(PolySynth& s, ReverbEffect* reverb) {
    std::cout << "Loading default strings sound..." << std::endl;
    s.setOsc1Waveform(Waveform::Saw);
    s.setOsc2Waveform(Waveform::Saw);
    s.setOsc1Level(0.6f);
    s.setOsc2Level(0.4f);
    s.setVCOBDetuneCents(7.0f);
    s.setNoiseLevel(0.0f);
    s.setRingModLevel(0.0f);
    s.setSyncEnabled(false);
    s.setPulseWidth(0.5f);
    s.setPWMDepth(0.0f);

    s.setVCOBLowFreqEnabled(false);
    s.setVCOBFreqKnob(0.5f);
    s.setVCOBKeyFollowEnabled(true);

    s.setFilterType(SynthParams::FilterType::LPF24);
    s.setVCFBaseCutoff(3000.0f);
    s.setVCFResonance(0.2f);
    s.setVCFKeyFollow(0.3f);
    s.setVCFEnvelopeAmount(0.6f);

    EnvelopeParams ampEnv = {0.8f, 1.5f, 0.7f, 2.0f};
    s.setAmpEnvelope(ampEnv);
    EnvelopeParams filterEnv = {1.2f, 1.0f, 0.4f, 1.5f};
    s.setFilterEnvelope(filterEnv);

    s.setFilterEnvVelocitySensitivity(0.3f);
    s.setAmpVelocitySensitivity(0.8f);

    s.setLfoRate(5.0f); 
    s.setLfoWaveform(LfoWaveform::Sine);
    s.setLfoAmountToVco1Freq(0.15f); 
    s.setLfoAmountToVco2Freq(0.18f);
    s.setLfoAmountToVco1Pw(0.0f);
    s.setLfoAmountToVco2Pw(0.0f);
    s.setLfoAmountToVcfCutoff(0.0f);

    resetPolyModAmounts(s);
    resetWheelModAmounts(s);
    s.setWheelModSource(WheelModSource::LFO); // Default to LFO for modwheel
    s.setWheelModAmountToFreqA(0.3f); 
    s.setWheelModAmountToFreqB(0.3f);

    s.setXModOsc1ToOsc2FMAmount(0.0f);
    s.setXModOsc2ToOsc1FMAmount(0.0f);

    s.setUnisonEnabled(true);
    s.setUnisonDetuneCents(10.0f);
    s.setUnisonStereoSpread(0.8f);

    s.setGlideEnabled(false);
    s.setGlideTime(0.1f);

    s.setMasterTuneCents(0.0f);
    s.setPitchBend(0.0f);
    s.setPitchBendRange(2.0f);

    s.setAnalogPitchDriftDepth(0.5f); 
    s.setAnalogPWDriftDepth(0.0f);    

    s.setMixerDrive(0.0f);
    s.setMixerPostGain(0.7f); // Adjusted for unison

    if (reverb) {
        reverb->setEnabled(true);
        reverb->setDryWetMix(0.35f);
        reverb->setRoomSize(0.7f);
        reverb->setDamping(0.4f);
        reverb->setRT60(2.5f);
        reverb->setWetGain(1.0f);
    }
    
    for (int oscNum = 1; oscNum <= 2; ++oscNum) {
      for (int i = 0; i < 16; ++i) { 
          s.setOscHarmonicAmplitude(oscNum, i, (i == 0) ? 1.0f : 0.0f);
      }
    }
}

// --- JSON Parameter Loading ---
namespace { // Anonymous namespace for helpers

// Helper to get value from json or default, with error logging
template <typename T>
T get_json_value_safe(const nlohmann::json& j, const std::string& key, T default_val, const std::string& path_prefix = "") {
    if (j.contains(key)) {
        try {
            return j.at(key).get<T>();
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "Warning: JSON type mismatch or error for key '" << path_prefix << key << "': " << e.what() << ". Using default." << std::endl;
            return default_val;
        }
    }
    return default_val;
}

Waveform stringToWaveform(const std::string& s, Waveform default_wf = Waveform::Saw) {
    if (s == "Sine") return Waveform::Sine;
    if (s == "Saw") return Waveform::Saw;
    if (s == "Square") return Waveform::Square;
    if (s == "Triangle") return Waveform::Triangle;
    if (s == "Pulse") return Waveform::Pulse;
    if (s == "Additive") return Waveform::Additive;
    std::cerr << "Warning: Unknown waveform string '" << s << "'. Using default." << std::endl;
    return default_wf;
}

LfoWaveform stringToLfoWaveform(const std::string& s, LfoWaveform default_wf = LfoWaveform::Triangle) {
    if (s == "Triangle") return LfoWaveform::Triangle;
    if (s == "SawUp") return LfoWaveform::SawUp;
    if (s == "Square") return LfoWaveform::Square;
    if (s == "Sine") return LfoWaveform::Sine;
    if (s == "RandomStep") return LfoWaveform::RandomStep;
    std::cerr << "Warning: Unknown LFO waveform string '" << s << "'. Using default." << std::endl;
    return default_wf;
}

SynthParams::FilterType stringToFilterType(const std::string& s, SynthParams::FilterType default_ft = SynthParams::FilterType::LPF24) {
    if (s == "LPF24") return SynthParams::FilterType::LPF24;
    if (s == "LPF12") return SynthParams::FilterType::LPF12;
    if (s == "HPF12") return SynthParams::FilterType::HPF12;
    if (s == "BPF12") return SynthParams::FilterType::BPF12;
    if (s == "NOTCH") return SynthParams::FilterType::NOTCH;
    std::cerr << "Warning: Unknown filter type string '" << s << "'. Using default." << std::endl;
    return default_ft;
}

WheelModSource stringToWheelModSource(const std::string& s, WheelModSource default_src = WheelModSource::LFO) {
    if (s == "LFO") return WheelModSource::LFO;
    if (s == "NOISE") return WheelModSource::NOISE;
    std::cerr << "Warning: Unknown wheel mod source string '" << s << "'. Using default." << std::endl;
    return default_src;
}

} // anonymous namespace

void loadParametersFromJson(PolySynth& s, ReverbEffect* reverb, const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cerr << "Warning: Could not open JSON parameter file: " << filename << std::endl;
        loadDefaultStringsSound(s, reverb);
        return;
    }

    nlohmann::json j;
    try {
        f >> j;
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Warning: Could not parse JSON file: " << filename << ". Error: " << e.what() << std::endl;
        loadDefaultStringsSound(s, reverb);
        return;
    }

    std::cout << "Loading parameters from " << filename << "..." << std::endl;

    // Synth-wide parameters
    s.setMasterTuneCents(get_json_value_safe(j, "masterTuneCents", 0.0f));
    if (j.contains("osc1Waveform")) {
        s.setOsc1Waveform(stringToWaveform(j.at("osc1Waveform").get<std::string>()));
    } else if (j.contains("waveform")) { // Fallback for old "waveform" key for OSC1
        std::cout << "Warning: 'waveform' key is deprecated for osc1, use 'osc1Waveform'. Using 'waveform' for OSC1." << std::endl;
        s.setOsc1Waveform(stringToWaveform(j.at("waveform").get<std::string>()));
    }
    if (j.contains("osc2Waveform")) {
        s.setOsc2Waveform(stringToWaveform(j.at("osc2Waveform").get<std::string>()));
    } else if (j.contains("waveform")) { // Fallback for old "waveform" key for OSC2
         std::cout << "Warning: 'waveform' key is deprecated for osc2, use 'osc2Waveform'. Using 'waveform' for OSC2." << std::endl;
        s.setOsc2Waveform(stringToWaveform(j.at("waveform").get<std::string>()));
    }
    s.setOsc1Level(get_json_value_safe(j, "osc1Level", 1.0f));
    s.setOsc2Level(get_json_value_safe(j, "osc2Level", 0.0f));
    s.setNoiseLevel(get_json_value_safe(j, "noiseLevel", 0.0f));
    s.setRingModLevel(get_json_value_safe(j, "ringModLevel", 0.0f));
    s.setVCOBDetuneCents(get_json_value_safe(j, "vcoBDetuneCents", 0.0f));
    s.setSyncEnabled(get_json_value_safe(j, "syncEnabled", false));
    s.setPulseWidth(get_json_value_safe(j, "pulseWidth", 0.5f));
    s.setPWMDepth(get_json_value_safe(j, "pwmDepth", 0.0f));
    s.setVCOBLowFreqEnabled(get_json_value_safe(j, "vcoBLowFreqEnabled", false));
    s.setVCOBFreqKnob(get_json_value_safe(j, "vcoBFreqKnob", 0.5f));
    s.setVCOBKeyFollowEnabled(get_json_value_safe(j, "vcoBKeyFollowEnabled", true));
    s.setFilterEnvVelocitySensitivity(get_json_value_safe(j, "filterEnvVelocitySensitivity", 0.0f));
    s.setAmpVelocitySensitivity(get_json_value_safe(j, "ampVelocitySensitivity", 0.7f));
    s.setXModOsc2ToOsc1FMAmount(get_json_value_safe(j, "xmodOsc2ToOsc1FMAmount", 0.0f));
    s.setXModOsc1ToOsc2FMAmount(get_json_value_safe(j, "xmodOsc1ToOsc2FMAmount", 0.0f));
    s.setPMFilterEnvToFreqAAmount(get_json_value_safe(j, "pmFilterEnvToFreqAAmount", 0.0f));
    s.setPMFilterEnvToPWAAmount(get_json_value_safe(j, "pmFilterEnvToPWAAmount", 0.0f));
    s.setPMFilterEnvToFilterCutoffAmount(get_json_value_safe(j, "pmFilterEnvToFilterCutoffAmount", 0.0f));
    s.setPMOscBToPWAAmount(get_json_value_safe(j, "pmOscBToPWAAmount", 0.0f));
    s.setPMOscBToFilterCutoffAmount(get_json_value_safe(j, "pmOscBToFilterCutoffAmount", 0.0f));
    
    if (j.contains("filterType")) s.setFilterType(stringToFilterType(j.at("filterType").get<std::string>()));
    s.setVCFBaseCutoff(get_json_value_safe(j, "vcfBaseCutoff", 5000.0f));
    s.setVCFResonance(get_json_value_safe(j, "vcfResonance", 0.1f));
    s.setVCFKeyFollow(get_json_value_safe(j, "vcfKeyFollow", 0.0f));
    s.setVCFEnvelopeAmount(get_json_value_safe(j, "vcfEnvelopeAmount", 0.5f));

    s.setMixerDrive(get_json_value_safe(j, "mixerDrive", 0.0f));
    s.setMixerPostGain(get_json_value_safe(j, "mixerPostGain", 1.0f));

    // Envelopes
    if (j.contains("ampEnv")) {
        const auto& env_j = j.at("ampEnv");
        EnvelopeParams p = {
            get_json_value_safe(env_j, "attack", 0.01f, "ampEnv."),
            get_json_value_safe(env_j, "decay", 0.1f, "ampEnv."),
            get_json_value_safe(env_j, "sustain", 0.7f, "ampEnv."),
            get_json_value_safe(env_j, "release", 0.2f, "ampEnv.")
        };
        s.setAmpEnvelope(p);
    }
    if (j.contains("filterEnv")) {
        const auto& env_j = j.at("filterEnv");
        EnvelopeParams p = {
            get_json_value_safe(env_j, "attack", 0.05f, "filterEnv."),
            get_json_value_safe(env_j, "decay", 0.2f, "filterEnv."),
            get_json_value_safe(env_j, "sustain", 0.5f, "filterEnv."),
            get_json_value_safe(env_j, "release", 0.3f, "filterEnv.")
        };
        s.setFilterEnvelope(p);
    }

    // LFO
    s.setLfoRate(get_json_value_safe(j, "lfoRate", 1.0f));
    if (j.contains("lfoWaveform")) s.setLfoWaveform(stringToLfoWaveform(j.at("lfoWaveform").get<std::string>()));
    s.setLfoAmountToVco1Freq(get_json_value_safe(j, "lfoAmountToVco1Freq", 0.0f));
    s.setLfoAmountToVco2Freq(get_json_value_safe(j, "lfoAmountToVco2Freq", 0.0f));
    s.setLfoAmountToVco1Pw(get_json_value_safe(j, "lfoAmountToVco1Pw", 0.0f));
    s.setLfoAmountToVco2Pw(get_json_value_safe(j, "lfoAmountToVco2Pw", 0.0f));
    s.setLfoAmountToVcfCutoff(get_json_value_safe(j, "lfoAmountToVcfCutoff", 0.0f));
    
    // Modulation Wheel
    s.setModulationWheelValue(get_json_value_safe(j, "modulationWheelValue", 0.0f)); // Usually set by MIDI, but can be preset
    if (j.contains("wheelModSource")) s.setWheelModSource(stringToWheelModSource(j.at("wheelModSource").get<std::string>()));
    s.setWheelModAmountToFreqA(get_json_value_safe(j, "wheelModAmountToFreqA", 0.0f));
    s.setWheelModAmountToFreqB(get_json_value_safe(j, "wheelModAmountToFreqB", 0.0f));
    s.setWheelModAmountToPWA(get_json_value_safe(j, "wheelModAmountToPWA", 0.0f));
    s.setWheelModAmountToPWB(get_json_value_safe(j, "wheelModAmountToPWB", 0.0f));
    s.setWheelModAmountToFilter(get_json_value_safe(j, "wheelModAmountToFilter", 0.0f));

    // Unison
    s.setUnisonEnabled(get_json_value_safe(j, "unisonEnabled", false));
    s.setUnisonDetuneCents(get_json_value_safe(j, "unisonDetuneCents", 7.0f));
    s.setUnisonStereoSpread(get_json_value_safe(j, "unisonStereoSpread", 0.7f));

    // Glide
    s.setGlideEnabled(get_json_value_safe(j, "glideEnabled", false));
    s.setGlideTime(get_json_value_safe(j, "glideTime", 0.05f));

    // Analog Drift
    s.setAnalogPitchDriftDepth(get_json_value_safe(j, "analogPitchDriftDepth", 0.0f));
    s.setAnalogPWDriftDepth(get_json_value_safe(j, "analogPWDriftDepth", 0.0f));

    // Pitch Bend Range
    s.setPitchBendRange(get_json_value_safe(j, "pitchBendRangeSemitones", 2.0f));


    // Harmonics (for Additive waveform primarily)
    if (j.contains("osc1Harmonics") && j.at("osc1Harmonics").is_array()) {
        int harmonicIdx = 0;
        for (const auto& amp_json : j.at("osc1Harmonics")) {
            if (harmonicIdx < 16) {
                s.setOscHarmonicAmplitude(1, harmonicIdx++, get_json_value_safe(amp_json, "", 0.0f));
            } else break;
        }
    }
    if (j.contains("osc2Harmonics") && j.at("osc2Harmonics").is_array()) {
        int harmonicIdx = 0;
        for (const auto& amp_json : j.at("osc2Harmonics")) {
            if (harmonicIdx < 16) {
                s.setOscHarmonicAmplitude(2, harmonicIdx++, get_json_value_safe(amp_json, "", 0.0f));
            } else break;
        }
    }

    // Reverb
    if (j.contains("reverb") && reverb) {
        const auto& rev_j = j.at("reverb");
        reverb->setEnabled(get_json_value_safe(rev_j, "enabled", false, "reverb."));
        reverb->setDryWetMix(get_json_value_safe(rev_j, "dryWetMix", 0.3f, "reverb."));
        reverb->setRoomSize(get_json_value_safe(rev_j, "roomSize", 0.5f, "reverb."));
        reverb->setDamping(get_json_value_safe(rev_j, "damping", 0.5f, "reverb."));
        reverb->setWetGain(get_json_value_safe(rev_j, "wetGain", 1.0f, "reverb."));
        reverb->setRT60(get_json_value_safe(rev_j, "rt60", 1.2f, "reverb."));
    }
    std::cout << "Parameters loaded successfully from " << filename << std::endl;
}


// --- MIDI Input Handling ---
RtMidiIn* midiIn = nullptr;
std::atomic<bool> midiInputActive(false);

void midiInputCallback(double /*deltatime*/, std::vector<unsigned char>* message, void* /*userData*/) {
    if (!message || message->empty()) return;

    unsigned char status = message->at(0);
    unsigned char type = status & 0xF0;
    // unsigned char channel = status & 0x0F; // Channel info if needed

    if (type == 0x90 && message->size() >= 3) { // Note On
        int note = message->at(1);
        int velocity = message->at(2);
        if (velocity > 0) {
            synth.noteOn(note, static_cast<float>(velocity));
        } else { // Note On with velocity 0 is often Note Off
            synth.noteOff(note);
        }
    } else if (type == 0x80 && message->size() >= 3) { // Note Off
        int note = message->at(1);
        synth.noteOff(note);
    } else if (type == 0xE0 && message->size() >= 3) { // Pitch Bend
        int lsb = message->at(1);
        int msb = message->at(2);
        int bendValue = (msb << 7) | lsb;
        float normalizedBend = (static_cast<float>(bendValue - 8192) / 8192.0f);
        synth.setPitchBend(normalizedBend);
    } else if (type == 0xB0 && message->size() >= 3) { // Control Change
        int controller = message->at(1);
        int value = message->at(2);
        if (controller == 1) { // Modulation Wheel
            synth.setModulationWheelValue(static_cast<float>(value) / 127.0f);
        }
        // Add more CC handling here if needed (e.g., sustain pedal CC 64)
    }
}

bool initializeMidiInput(int portNumber = -1) {
    midiIn = new RtMidiIn();
    unsigned int nPorts = midiIn->getPortCount();
    if (nPorts == 0) {
        std::cout << "No MIDI input ports available!" << std::endl;
        delete midiIn;
        midiIn = nullptr;
        return false;
    }

    if (portNumber < 0 || portNumber >= static_cast<int>(nPorts)) {
        std::cout << "Available MIDI Input Ports:" << std::endl;
        for (unsigned int i = 0; i < nPorts; i++) {
            std::cout << "  Port " << i << ": " << midiIn->getPortName(i) << std::endl;
        }
        std::cout << "Please select a port number (or run with -p <port_num>): ";
        std::cin >> portNumber;
        if (portNumber < 0 || portNumber >= static_cast<int>(nPorts)) {
             std::cerr << "Invalid port selection." << std::endl;
             delete midiIn;
             midiIn = nullptr;
             return false;
        }
    }
    
    try {
        midiIn->openPort(portNumber);
        midiIn->setCallback(&midiInputCallback);
        midiIn->ignoreTypes(false, false, false); // Don't ignore sysex, timing, or active sensing
        std::cout << "MIDI input opened on port " << portNumber << ": " << midiIn->getPortName(portNumber) << std::endl;
        midiInputActive.store(true);
        return true;
    } catch (RtMidiError &error) {
        error.printMessage();
        delete midiIn;
        midiIn = nullptr;
        return false;
    }
}

void cleanupMidiInput() {
    if (midiIn) {
        midiIn->closePort();
        delete midiIn;
        midiIn = nullptr;
        midiInputActive.store(false);
    }
}

// --- MIDI File Playback ---
void playMidiFile(const std::string& filePath, PaStream* audioStream) {
    smf::MidiFile midifile;
    if (!midifile.read(filePath)) {
        std::cerr << "Error: Could not read MIDI file: " << filePath << std::endl;
        return;
    }

    midifile.linkNotePairs(); // Important for proper note off handling if not explicit
    midifile.doTimeAnalysis(); // Convert ticks to seconds based on tempo map
    midifile.joinTracks();    // Merge all tracks into track 0
    // midifile.sortTracks(); // Sort events in merged track (usually done by joinTracks)

    std::cout << "Playing MIDI file: " << filePath << " (" << midifile.getTrackCount() << " tracks, "
              << midifile.getNumEvents(0) << " events after join)" << std::endl;
    std::cout << "TPQN: " << midifile.getTicksPerQuarterNote() << std::endl;


    double currentTimeSeconds = 0.0;
    PaTime paStreamStartTime = Pa_GetStreamTime(audioStream); // Get stream start time for more accurate timing
    if (paStreamStartTime < 0) { // Error getting stream time
        std::cerr << "Warning: Could not get PortAudio stream time. MIDI playback timing might be less accurate." << std::endl;
        paStreamStartTime = 0; // Fallback, timing relative to playback start
    }


    for (int i = 0; i < midifile.getNumEvents(0); ++i) {
        if (midiInputActive.load()) { // If live MIDI input starts, stop file playback
            std::cout << "MIDI input detected, stopping MIDI file playback." << std::endl;
            synth.noteOff(-1); // All notes off as a precaution
            return;
        }

        smf::MidiEvent& event = midifile.getEvent(0, i);
        double eventTimeSeconds = event.seconds;

        // Calculate delay until this event
        // More robust timing:
        double targetPaTime = paStreamStartTime + eventTimeSeconds;
        double currentPaTime = Pa_GetStreamTime(audioStream);
        if (currentPaTime < 0 && paStreamStartTime == 0) { // If stream time errored and using relative
             currentPaTime = currentTimeSeconds; // Use our own tracked time
        } else if (currentPaTime < 0) { // Error, try to recover or sleep based on simpler diff
             currentPaTime = targetPaTime - (eventTimeSeconds - currentTimeSeconds); // Estimate
        }


        double delaySeconds = targetPaTime - currentPaTime;
        
        if (delaySeconds > 0.001) { // Only sleep if delay is significant
            Pa_Sleep(static_cast<long>(delaySeconds * 1000.0));
        }
        currentTimeSeconds = eventTimeSeconds; // Update our tracked time regardless

        // Process MIDI event
        if (event.isNoteOn()) {
            synth.noteOn(event.getKeyNumber(), static_cast<float>(event.getVelocity()));
        } else if (event.isNoteOff()) {
            synth.noteOff(event.getKeyNumber());
        } else if (event.isPitchbend()) {
            // SMF pitch bend is 0-16383, center 8192.
            // Convert to -1.0 to 1.0 range.
            float bend = (static_cast<float>(event.getP2()) - 8192.0f) / 8192.0f;
            synth.setPitchBend(bend);
        } else if (event.isController()) {
            if (event.getP1() == 1) { // Mod Wheel (CC1)
                synth.setModulationWheelValue(static_cast<float>(event.getP2()) / 127.0f);
            }
            // Add other CCs if needed
        }
        // Could handle tempo changes, meta events, etc. here
    }
    std::cout << "MIDI file playback finished." << std::endl;
    synth.noteOff(-1); // All notes off
}


// --- Main Application ---
int main(int argc, char* argv[]) {
    std::string jsonPath = "synth_params.json"; // Default JSON path
    std::string midiFilePath = "";
    int midiInputPort = -1; // Default to no MIDI input, will prompt if not specified

    // Basic command line argument parsing
    // Usage: ./synth [json_config_path] [midi_file_path] [midi_input_port_num]
    if (argc > 1) jsonPath = argv[1];
    if (argc > 2) midiFilePath = argv[2];
    if (argc > 3) {
        try {
            midiInputPort = std::stoi(argv[3]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid MIDI input port number: " << argv[3] << ". Will prompt if MIDI input is chosen." << std::endl;
            midiInputPort = -1;
        }
    }
    
    // Initialize PortAudio
    if (Pa_Initialize() != paNoError) {
        std::cerr << "PortAudio initialization error!" << std::endl;
        return 1;
    }

    // Setup Reverb
    auto reverbInstance = std::make_unique<ReverbEffect>(synth.getSampleRate()); // Use synth's sample rate
    mainReverbPtr = reverbInstance.get();
    synth.addEffect(std::move(reverbInstance));

    // Load parameters (from JSON or default strings)
    loadParametersFromJson(synth, mainReverbPtr, jsonPath);

    // Open PortAudio stream
    PaStream* audioStream;
    PaError err = Pa_OpenDefaultStream(&audioStream, 0, 2, paFloat32,
                                     synth.getSampleRate(), 256, audioCallback, nullptr);
    if (err != paNoError) {
        std::cerr << "PortAudio stream open error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(audioStream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream start error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(audioStream);
        Pa_Terminate();
        return 1;
    }
    std::cout << "Audio stream started. Outputting sound." << std::endl;

    // Determine mode: MIDI input, MIDI file playback, or idle
    bool midiFilePlayed = false;
    if (initializeMidiInput(midiInputPort)) {
        std::cout << "MIDI input active. Press Ctrl+C to quit." << std::endl;
        // If MIDI file also specified, MIDI input takes precedence.
        // User can modify this logic if MIDI file should play THEN listen.
        if (!midiFilePath.empty()) {
            std::cout << "MIDI file specified (" << midiFilePath << ") but MIDI input is active and takes precedence." << std::endl;
        }
    } else if (!midiFilePath.empty()) {
        playMidiFile(midiFilePath, audioStream);
        midiFilePlayed = true;
    } else {
        std::cout << "No MIDI input or MIDI file specified. Idling." << std::endl;
        std::cout << "Usage: " << argv[0] << " [params.json] [song.mid] [midi_port_num]" << std::endl;
        std::cout << "Example (strings preset, play midifile): " << argv[0] << " \"\" my_song.mid" << std::endl;
        std::cout << "Example (load 'custom.json', listen to MIDI port 0): " << argv[0] << " custom.json \"\" 0" << std::endl;
        std::cout << "If params.json is empty string or non-existent, default strings are used." << std::endl;
    }

    // Main loop (keep audio running)
    if (midiInputActive.load() || !midiFilePlayed) { // Keep alive if MIDI input or if nothing played
        std::cout << "Synth running. Press Ctrl+C to quit." << std::endl;
        while (Pa_IsStreamActive(audioStream)) {
            Pa_Sleep(100); // Sleep a bit
            if (midiInputActive.load() && midiIn && !midiIn->isPortOpen()) {
                 std::cerr << "MIDI port seems to have closed unexpectedly." << std::endl;
                 break; // Or try to reopen?
            }
        }
    } else { // MIDI file played and finished, no live input
        std::cout << "Playback finished. Exiting." << std::endl;
    }


    // Cleanup
    std::cout << "Stopping synth..." << std::endl;
    cleanupMidiInput();

    err = Pa_StopStream(audioStream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream stop error: " << Pa_GetErrorText(err) << std::endl;
    }
    err = Pa_CloseStream(audioStream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream close error: " << Pa_GetErrorText(err) << std::endl;
    }
    Pa_Terminate();
    std::cout << "Synth stopped." << std::endl;

    return 0;
}