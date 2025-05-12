// synth/harmonic_osc.h
#pragma once
#include "envelope.h" 
#include "lfo.h"      
#include "waveform.h" 
#include <vector>
#include <random>
#include <cmath>
#include <algorithm> 

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif


class HarmonicOscillator { 
public:
    HarmonicOscillator(int sampleRate, int numHarmonics); 
    void setFrequency(float freq);                         
    float getBaseFrequency() const;                        
    void noteOn();                                         
    void noteOff();                                        
    float process();                                       
    bool isRunning() const;                                
    bool isGateOpen() const;                               

    void setWaveform(Waveform wf);                         
    Waveform getWaveform() const;                          
    void resetPhase();                                     
    float getPhase() const;                                
    void setPulseWidth(float width);                       
    void setPWMDepth(float depth);                         
    void setPWMSource(float value);                        
    void setPolyModPWValue(float value);                   
    void setWheelModPWValue(float value);                  
    void setHarmonicAmplitude(int harmonicIndex, float amplitude); 
    float getHarmonicAmplitude(int harmonicIndex) const;           
    void setDriftPWValue(float value);                     
    void sync();                                           

private:
    int sampleRate;
    int numHarmonics; 
    float baseFreq; 
    float phase;
    bool gateOpen;
    Waveform waveform;

    std::vector<LFO> lfos; // These seem unused currently for direct osc modulation
    std::vector<Envelope> envelopes; // These also seem unused currently for direct osc modulation
    std::vector<float> harmonicAmplitudes_; 

    std::mt19937 rng; // Unused currently
    std::uniform_real_distribution<float> noiseDist; // Unused currently

    float pulseWidth;
    float pwmDepth;
    float currentPWMSourceValue;
    float polyModPWValue;
    float wheelModPWValue;
    float driftPWValue;

};