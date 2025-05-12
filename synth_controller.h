#pragma once
#include "poly_synth.h"
#include <memory>
#include <thread>
#include <atomic>

namespace osc { class OscPacketListener; class UdpListeningReceiveSocket; }

class SynthController {
public:
    SynthController(int sample_rate, int max_voices);
    ~SynthController();

    bool startOSCServer(int port);
    void stopOSCServer();

    bool startMIDIInput(int port_number = 0); 
    void stopMIDIInput();

    PolySynth* getSynthInstance(); 

private:
    void oscMessageReceived(const char* address, const std::vector<char>& data); 
    void midiMessageReceived(double deltatime, std::vector<unsigned char> *message, void *userData);


    std::unique_ptr<PolySynth> synth_;
    
    std::unique_ptr<std::thread> osc_thread_;
    std::atomic<bool> osc_running_;
};