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
    calculateCoefficients(baseCutoffHz, resonance); // Initial calculation
}

void VCF::setType(SynthParams::FilterType type) {
    if (currentFilterType_ != type) {
        currentFilterType_ = type;
        // Reset filter states when type changes to avoid instability/artifacts
        std::fill(std::begin(z_ladder_), std::end(z_ladder_), 0.0f);
        s1_svf_ = 0.0f;
        s2_svf_ = 0.0f;
        // Recalculate coefficients for the new type with current settings
        calculateCoefficients(currentEffectiveCutoffHz_, resonance); 
    }
}

SynthParams::FilterType VCF::getType() const {
    return currentFilterType_;
}

void VCF::setBaseCutoff(float hz) {
    baseCutoffHz = std::clamp(hz, 20.0f, sampleRate * 0.49f); // Clamp to prevent aliasing issues
    // No direct recalculation here, process() will do it based on effective cutoff
}

void VCF::setResonance(float r) {
    // User resonance 0-1. Internally, SVF Q needs to be mapped carefully.
    // For ladder, resonance 0-0.98 was used.
    // Let's keep resonance 0-1 for user, and map it.
    resonance = std::clamp(r, 0.0f, 1.0f);
    // No direct recalculation here
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
    // SVF Coefficients
    // cutoffHz is the final modulated cutoff
    // resonanceValue is user 0-1
    
    // Using tan pre-warping for SVF 'f' coefficient for better Nyquist behavior
    float f_temp = static_cast<float>(std::tan(M_PI * cutoffHz / sampleRate));
    svf_f_ = std::clamp(f_temp, 0.0001f, 1.0f); // Clamp f to avoid issues at extremes

    // Map user resonance (0-1) to SVF Q factor.
    // Q = 0.5 (no resonance) to high values (e.g., 20-50 for strong resonance)
    // svf_q_coeff_ is typically 1.0 / (2.0 * Q) or similar for Chamberlin.
    // So, high resonance = low svf_q_coeff_
    // Let's map resonance [0, 1] to Q [0.5, 25]. So svf_q_coeff_ [1.0, 0.02]
    float min_q_svf = 0.5f; // Minimum Q (no resonance)
    float max_q_svf = 25.0f; // Maximum Q (high resonance, adjust for self-oscillation behavior)
    float q_factor = min_q_svf + resonanceValue * (max_q_svf - min_q_svf);
    
    svf_q_coeff_ = 1.0f / (2.0f * q_factor);
    svf_q_coeff_ = std::clamp(svf_q_coeff_, 0.01f, 1.0f); // Clamp to prevent instability (1/ (2*0.5) = 1, 1/(2*50) = 0.01)

    // Ladder filter coefficients are calculated inside its process block for now due to 'f' calculation method
}


float VCF::process(float input, float directModHz) {
    // 1. Calculate effective cutoff frequency
    float effectiveCutoff = baseCutoffHz;
    effectiveCutoff *= std::pow(2.0f, keyFollow * (std::log2(noteBaseFreq / 440.0f)) );
    float envSweepOctaves = 5.0f; 
    effectiveCutoff *= std::pow(2.0f, envModAmount * (envelopeValue - 0.5f) * 2.0f * envSweepOctaves); 
    effectiveCutoff += directModHz;
    currentEffectiveCutoffHz_ = std::clamp(effectiveCutoff, 20.0f, sampleRate * 0.49f);

    // 2. Recalculate filter coefficients based on new effective cutoff and resonance
    calculateCoefficients(currentEffectiveCutoffHz_, resonance);

    // 3. Process based on filter type
    float output = 0.0f;

    switch (currentFilterType_) {
        case SynthParams::FilterType::LPF24: {
            // Ladder Filter (existing algorithm)
            float f_ladder = 2.0f * std::sinf(static_cast<float>(M_PI) * currentEffectiveCutoffHz_ / sampleRate);
            f_ladder = std::clamp(f_ladder, 0.0f, 1.0f); 
            float fb_ladder = resonance * 3.95f; // Map resonance 0-1 to 0-3.95
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
            // State Variable Filter (Chamberlin)
            // Input with soft clipping to prevent blow-ups with high resonance
            float svf_input = std::tanh(input);

            // Chamberlin SVF calculation (integrators s1_svf, s2_svf)
            // v0 = input - s2 (or input - feedback_path)
            // For this form, resonance is controlled by damping on the BP feedback to LP input
            // A common Chamberlin form:
            float v0 = svf_input; // Input to the filter
            float v_lp = s2_svf_; // Lowpass output from previous sample
            float v_bp = s1_svf_; // Bandpass output from previous sample

            // Note: svf_q_coeff_ is like 1/(2Q). Smaller value means higher Q.
            float v_hp = v0 - v_lp - svf_q_coeff_ * v_bp; // Highpass output

            float v_bp_new = svf_f_ * v_hp + v_bp;      // New bandpass state component
            float v_lp_new = svf_f_ * v_bp_new + v_lp;    // New lowpass state component

            s1_svf_ = v_bp_new; // Update bandpass state (integrator 1)
            s2_svf_ = v_lp_new; // Update lowpass state (integrator 2)
            
            // Denormal protection (very small random numbers if state becomes zero)
            // This is a common trick if filter states get stuck at pure zero.
            // Not always necessary with float precision but can help.
            // const float denormal_offset = 1e-20f;
            // if (std::abs(s1_svf_) < denormal_offset) s1_svf_ += (rand() % 2 ? denormal_offset : -denormal_offset);
            // if (std::abs(s2_svf_) < denormal_offset) s2_svf_ += (rand() % 2 ? denormal_offset : -denormal_offset);


            if (currentFilterType_ == SynthParams::FilterType::LPF12) {
                output = s2_svf_; // Lowpass output
            } else if (currentFilterType_ == SynthParams::FilterType::HPF12) {
                output = v_hp; // Highpass output
            } else if (currentFilterType_ == SynthParams::FilterType::BPF12) {
                output = s1_svf_; // Bandpass output
            } else if (currentFilterType_ == SynthParams::FilterType::NOTCH) {
                output = v_hp + s2_svf_; // Notch = HP + LP
            }
            break;
        }
    }
    return output;
}