// synth/vcf.cpp
#include "vcf.h"
#include <cmath>
#include <algorithm> 

VCF::VCF(float sr)
    : sampleRate(sr), currentFilterType_(SynthParams::FilterType::LPF24), 
      baseCutoffHz(1000.0f), resonance(0.0f), keyFollow(0.0f),
      envModAmount(0.0f), envelopeValue(0.0f), noteBaseFreq(440.0f),
      s1_svf_(0.0f), s2_svf_(0.0f), svf_f_(0.1f), svf_q_coeff_(0.5f),
      currentEffectiveCutoffHz_(1000.0f) {
    std::fill(std::begin(z_ladder_), std::end(z_ladder_), 0.0f);
    calculateCoefficients(baseCutoffHz, resonance); 
}

void VCF::setType(SynthParams::FilterType type) {
    if (currentFilterType_ != type) {
        currentFilterType_ = type;
        std::fill(std::begin(z_ladder_), std::end(z_ladder_), 0.0f);
        s1_svf_ = 0.0f;
        s2_svf_ = 0.0f;
        calculateCoefficients(currentEffectiveCutoffHz_, resonance); 
    }
}

SynthParams::FilterType VCF::getType() const {
    return currentFilterType_;
}

void VCF::setBaseCutoff(float hz) {
    baseCutoffHz = std::clamp(hz, 20.0f, sampleRate * 0.49f); 
}

void VCF::setResonance(float r) {
    resonance = std::clamp(r, 0.0f, 1.0f);
}

void VCF::setKeyFollow(float factor) {
    keyFollow = std::clamp(factor, 0.0f, 1.0f);
}

void VCF::setEnvelopeMod(float amount) {
    envModAmount = std::clamp(amount, -1.0f, 1.0f);
}

void VCF::setNote(int midiNote) {
    noteBaseFreq = 440.0f * std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);
}

void VCF::setEnvelopeValue(float env) {
    envelopeValue = std::clamp(env, 0.0f, 1.0f); 
}

void VCF::calculateCoefficients(float cutoffHz, float resonanceValue) {
    float f_temp = static_cast<float>(std::tan(M_PI * cutoffHz / sampleRate));
    svf_f_ = std::clamp(f_temp, 0.0001f, 1.0f); 

    float min_q_svf = 0.5f; 
    float max_q_svf = 25.0f; 
    float q_factor = min_q_svf + resonanceValue * (max_q_svf - min_q_svf);
    
    svf_q_coeff_ = 1.0f / (2.0f * q_factor);
    svf_q_coeff_ = std::clamp(svf_q_coeff_, 0.01f, 1.0f); 

}


float VCF::process(float input, float directModHz) {
    float effectiveCutoff = baseCutoffHz;
    effectiveCutoff *= std::pow(2.0f, keyFollow * (std::log2(noteBaseFreq / 440.0f)) );
    float envSweepOctaves = 5.0f; 
    effectiveCutoff *= std::pow(2.0f, envModAmount * (envelopeValue - 0.5f) * 2.0f * envSweepOctaves); 
    effectiveCutoff += directModHz;
    currentEffectiveCutoffHz_ = std::clamp(effectiveCutoff, 20.0f, sampleRate * 0.49f);

    calculateCoefficients(currentEffectiveCutoffHz_, resonance);

    float output = 0.0f;

    switch (currentFilterType_) {
        case SynthParams::FilterType::LPF24: {
            float f_ladder = 2.0f * std::sinf(static_cast<float>(M_PI) * currentEffectiveCutoffHz_ / sampleRate);
            f_ladder = std::clamp(f_ladder, 0.0f, 1.0f); 
            float fb_ladder = resonance * 3.95f; 
            fb_ladder = std::clamp(fb_ladder, 0.0f, 3.95f);

            float lowPass = input - z_ladder_[3] * fb_ladder; 
            lowPass = std::clamp(lowPass, -10.0f, 10.0f); 

            z_ladder_[0] = z_ladder_[0] + f_ladder * (lowPass - z_ladder_[0]);
            z_ladder_[1] = z_ladder_[1] + f_ladder * (z_ladder_[0] - z_ladder_[1]);
            z_ladder_[2] = z_ladder_[2] + f_ladder * (z_ladder_[1] - z_ladder_[2]);
            z_ladder_[3] = z_ladder_[3] + f_ladder * (z_ladder_[2] - z_ladder_[3]);
            output = z_ladder_[3];
            break;
        }
        
        case SynthParams::FilterType::LPF12:
        case SynthParams::FilterType::HPF12:
        case SynthParams::FilterType::BPF12:
        case SynthParams::FilterType::NOTCH: {
            float svf_input = std::tanh(input);

            float v0 = svf_input; 
            float v_lp = s2_svf_; 
            float v_bp = s1_svf_; 

            float v_hp = v0 - v_lp - svf_q_coeff_ * v_bp; 

            float v_bp_new = svf_f_ * v_hp + v_bp;      
            float v_lp_new = svf_f_ * v_bp_new + v_lp;    

            s1_svf_ = v_bp_new; 
            s2_svf_ = v_lp_new; 
            
            if (currentFilterType_ == SynthParams::FilterType::LPF12) {
                output = s2_svf_; 
            } else if (currentFilterType_ == SynthParams::FilterType::HPF12) {
                output = v_hp; 
            } else if (currentFilterType_ == SynthParams::FilterType::BPF12) {
                output = s1_svf_; 
            } else if (currentFilterType_ == SynthParams::FilterType::NOTCH) {
                output = v_hp + s2_svf_; 
            }
            break;
        }
    }
    return output;
}