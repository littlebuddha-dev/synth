#pragma once
#include "poly_synth.h"
#include <memory>
#include <thread>
#include <atomic>

// Forward declarations for OSC and MIDI library types if used
namespace osc { class OscPacketListener; class UdpListeningReceiveSocket; }
// class MidiInput; // Example

class SynthController {
public:
    SynthController(int sample_rate, int max_voices);
    ~SynthController();

    bool startOSCServer(int port);
    void stopOSCServer();

    bool startMIDIInput(int port_number = 0); // Or device name
    void stopMIDIInput();

    PolySynth* getSynthInstance(); // For direct access if needed (e.g., audio callback)

private:
    void oscMessageReceived(const char* address, const std::vector<char>& data); // Simplified
    void midiMessageReceived(double deltatime, std::vector<unsigned char> *message, void *userData);


    std::unique_ptr<PolySynth> synth_;
    
    // OSC members
    std::unique_ptr<std::thread> osc_thread_;
    std::atomic<bool> osc_running_;
    // osc::UdpListeningReceiveSocket* osc_socket_; // Example
    // osc::OscPacketListener* osc_listener_;      // Example

    // MIDI members
    // MidiInput* midi_in_; // Example
    // ...
};