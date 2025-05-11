// synth/voice.h
#pragma once

#include "harmonic_osc.h"
#include "waveform.h"
#include "vcf.h"
#include "envelope.h"
#include "lfo.h"
#include "analog_drift.h" 

struct LfoModulationValues {
    float osc1FreqMod = 0.0f; // Semitones from LFO & Wheel
    float osc2FreqMod = 0.0f; // Semitones from LFO & Wheel
    float osc1PwMod = 0.0f;
    float osc2PwMod = 0.0f;
    float wheelOsc1PwOffset = 0.0f; 
    float wheelOsc2PwOffset = 0.0f; 
    float vcfCutoffMod = 0.0f;  // Hz offset from LFO & Wheel
};


class Voice {
private: 
    bool active;
    float velocityValue = 1.0f;
    int sampleRate;
    int noteNumber = 60; 

    HarmonicOscillator osc1;
    HarmonicOscillator osc2;

    float osc1Level = 1.0f;
    float osc2Level = 0.0f;
    float noiseLevel = 0.0f;
    float ringModLevel_ = 0.0f; 

    float lastOsc2 = 0.0f;

    float vcoBDetuneCents = 0.0f;
    bool vcoBLowFreqEnabled = false;
    float vcoBFreqKnob = 0.5f;
    bool vcoBKeyFollowEnabled_;       
    float vcoBFixedBaseFreq_;         

    bool syncEnabled = false;

    float filterEnvVelocitySensitivity = 0.0f;
    float ampVelocitySensitivity = 0.0f;

    float pm_filterEnv_to_freqA_amt = 0.0f;
    float pm_filterEnv_to_pwA_amt = 0.0f;
    float pm_filterEnv_to_filterCutoff_amt = 0.0f;
    float pm_oscB_to_freqA_amt = 0.0f;
    float pm_oscB_to_pwA_amt = 0.0f;
    float pm_oscB_to_filterCutoff_amt = 0.0f;


    VCF filter;
    Envelope envelopes[2];

    unsigned long long noteOnTimestamp = 0;

    float currentOutputFreq;             // Current glided, *unbent* frequency of the note
    float targetKeyFreq;                 
    float glideStartFreqForCurrentSegment; 
    float glideLogStep;                    
    unsigned int glideTimeSamples;         
    unsigned int glideSamplesElapsed;      
    bool isGliding;
    bool firstNoteForThisVoiceInstance;    

    // Analog Drift
    AnalogDrift analogDriftPitch1;
    AnalogDrift analogDriftPitch2;
    AnalogDrift analogDriftPW1;
    AnalogDrift analogDriftPW2;
    float pitchDriftDepthCents; 
    float pwDriftDepth;         

    float panning_ = 0.0f; // -1.0 (L) to 1.0 (R)

    void noteOnDetailed(float newTargetFrequency, float normalizedVelocity, int midiNoteNum, bool useGlide, float glideTimeSec);


public:
    Voice(int sampleRate, int numHarmonics);
    
    void noteOn(float freq, float velocity, int midiNoteNum, bool globalGlideEnabled, float globalGlideTimeSeconds);
    
    void noteOff();
    // Modified process method to accept pitch bend parameters
    float process(const LfoModulationValues& lfoMod, float currentPitchBendValue, float pitchBendRangeInSemitones);
    bool isActive() const;
    
    float getTargetKeyFrequency() const { return targetKeyFreq; }; 
    float getCurrentOutputFrequency() const { return currentOutputFreq; }
    int getNoteNumber() const { return noteNumber; }

    bool isTrulyIdle() const { return !active && !envelopes[0].isActive() && !envelopes[1].isActive(); }
    float getAmpEnvLevel() const { return envelopes[1].getCurrentLevel(); }
    unsigned long long getNoteOnTimestamp() const { return noteOnTimestamp; }
    void setNoteOnTimestamp(unsigned long long ts) { noteOnTimestamp = ts; }
    bool isGateOpen() const { return active; }
    bool areEnvelopesActive() const { return envelopes[0].isActive() || envelopes[1].isActive(); }


    void setWaveform(Waveform wf);

    void setOsc1Level(float level);
    void setOsc2Level(float level);
    void setNoiseLevel(float level);
    void setRingModLevel(float level);
    void setMixLevels(float level1, float level2, float noise, float ringMod);

    void setVCOBDetuneCents(float cents);
    void setSyncEnabled(bool enabled);
    void setPulseWidth(float width);
    void setPWMDepth(float depth);
    void setVCOBLowFreqEnabled(bool enabled);
    void setVCOBFreqKnob(float knobValue); 
    void setVCOBKeyFollowEnabled(bool enabled); 
    void setFilterEnvVelocitySensitivity(float amount); 
    void setAmpVelocitySensitivity(float amount);       

    void setPMFilterEnvToFreqAAmount(float amount);
    void setPMFilterEnvToPWAAmount(float amount);
    void setPMFilterEnvToFilterCutoffAmount(float amount);
    void setPMOscBToFreqAAmount(float amount);
    void setPMOscBToPWAAmount(float amount);
    void setPMOscBToFilterCutoffAmount(float amount);

    void setVCFResonance(float q);
    void setVCFKeyFollow(float f);
    void setVCFEnvelopeAmount(float amt);
    void setVCFBaseCutoff(float hz);

    void setAmpEnvelope(const EnvelopeParams& p);
    void setFilterEnvelope(const EnvelopeParams& p);

    void setPitchDriftDepth(float cents);
    void setPWDriftDepth(float depth);

    void setOsc1HarmonicAmplitude(int harmonicIndex, float amplitude); 
    void setOsc2HarmonicAmplitude(int harmonicIndex, float amplitude); 

    void setPanning(float pan); // -1.0 (L) to 1.0 (R)
    float getPanning() const;
};