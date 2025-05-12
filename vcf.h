// synth/vcf.h
#pragma once
#include <algorithm> 
#include <cmath>     
#include "synth_parameters.h" 

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

class VCF {
public:
    VCF(float sampleRate = 44100.0f);

    void setType(SynthParams::FilterType type); 
    SynthParams::FilterType getType() const;    

    void setBaseCutoff(float hz); 
    float getBaseCutoff() const { return baseCutoffHz; }
    void setResonance(float q); 
    void setKeyFollow(float factor);
    void setEnvelopeMod(float amount);
    void setNote(int midiNote); 
    void setEnvelopeValue(float env); 
    
    float process(float input, float directModHz);

private:
    void calculateCoefficients(float cutoffHz, float resonanceValue); 

    SynthParams::FilterType currentFilterType_; 
    float baseCutoffHz; 
    float resonance;    
    float keyFollow;    
    float envModAmount; 
    float envelopeValue = 0.0f; 
    float noteBaseFreq = 440.0f; 
    float sampleRate;

    float z_ladder_[4]; 

    float s1_svf_, s2_svf_; 
    
    float svf_f_, svf_q_coeff_; 

    float currentEffectiveCutoffHz_;
};