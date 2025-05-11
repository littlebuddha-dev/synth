// synth/voice.cpp
#include "voice.h"
#include <cmath>
#include "envelope.h" 
#include <random>
#include <iostream> // For std::cout in debug, remove in production

std::default_random_engine generator; // Should this be per-voice or global? Global for now.
std::uniform_real_distribution<float> distribution(-1.0f, 1.0f); 

Voice::Voice(int sampleRate_, int numHarmonics)
    : sampleRate(sampleRate_), active(false),
      velocityValue(1.0f),
      osc1(sampleRate_, numHarmonics),
      osc2(sampleRate_, numHarmonics),
      envelopes{
          Envelope(0.01f, 0.1f, 0.7f, 0.3f, sampleRate_), 
          Envelope(0.01f, 0.1f, 0.9f, 0.2f, sampleRate_)  
      },
      filter(sampleRate_),
      noteOnTimestamp(0),
      vcoBDetuneCents(0.0f),
      vcoBLowFreqEnabled(false),
      vcoBFreqKnob(0.5f),
      vcoBKeyFollowEnabled_(true),      
      vcoBFixedBaseFreq_(-1.0f),        
      filterEnvVelocitySensitivity(0.0f),
      ampVelocitySensitivity(0.0f),
      pm_filterEnv_to_freqA_amt(0.0f),
      pm_filterEnv_to_pwA_amt(0.0f),
      pm_filterEnv_to_filterCutoff_amt(0.0f),
      pm_oscB_to_freqA_amt(0.0f),
      pm_oscB_to_pwA_amt(0.0f),
      pm_oscB_to_filterCutoff_amt(0.0f),
      currentOutputFreq(0.0f), // This will store the glided, unbent frequency
      targetKeyFreq(0.0f),
      glideStartFreqForCurrentSegment(0.0f),
      glideLogStep(0.0f),
      glideTimeSamples(0),
      glideSamplesElapsed(0),
      isGliding(false),
      firstNoteForThisVoiceInstance(true)
      , pitchDriftDepthCents(0.0f) 
      , pwDriftDepth(0.0f)
{ 
}

void Voice::noteOn(float freq, float velocity, int midiNoteNum, bool globalGlideEnabled, float globalGlideTimeSeconds) {
    float normalizedVelocity = std::clamp(velocity / 127.0f, 0.0f, 1.0f);
    noteOnDetailed(freq, normalizedVelocity, midiNoteNum, globalGlideEnabled, globalGlideTimeSeconds);
}


void Voice::noteOnDetailed(float newTargetFrequency, float normVelocity, int midiNoteNum, bool useGlide, float glideTimeSec) {
    this->targetKeyFreq = newTargetFrequency; // This is the unbent frequency of the MIDI key
    this->velocityValue = normVelocity; 
    this->noteNumber = midiNoteNum; 
    this->active = true;

    if (vcoBKeyFollowEnabled_ || vcoBFixedBaseFreq_ < 0.0f) {
        vcoBFixedBaseFreq_ = this->targetKeyFreq; // Store unbent frequency
    }

    osc1.noteOn();
    osc2.noteOn();
    envelopes[0].noteOn(); 
    envelopes[1].noteOn(); 
    filter.setNote(noteNumber); 

    float freqToGlideFrom = this->currentOutputFreq; 

    if (useGlide &&
        !this->firstNoteForThisVoiceInstance &&
        freqToGlideFrom != 0.0f && 
        freqToGlideFrom != this->targetKeyFreq && 
        glideTimeSec > 0.0001f) { 

        this->isGliding = true;
        this->glideStartFreqForCurrentSegment = freqToGlideFrom;
        this->currentOutputFreq = freqToGlideFrom; 

        this->glideTimeSamples = static_cast<unsigned int>(glideTimeSec * static_cast<float>(sampleRate));
        if (this->glideTimeSamples == 0) { 
             this->glideTimeSamples = 1;
        }
        
        if (this->glideStartFreqForCurrentSegment <= 0.0f || this->targetKeyFreq <= 0.0f) {
            this->isGliding = false; 
            this->currentOutputFreq = this->targetKeyFreq;
        } else {
            double logRatioTotal = std::log(static_cast<double>(this->targetKeyFreq) / this->glideStartFreqForCurrentSegment);
            this->glideLogStep = static_cast<float>(logRatioTotal / static_cast<double>(this->glideTimeSamples));
        }
        this->glideSamplesElapsed = 0;

    } else { 
        this->currentOutputFreq = this->targetKeyFreq; // Start at target, unbent
        this->isGliding = false;
    }
    this->firstNoteForThisVoiceInstance = false;
}


void Voice::noteOff() {
    active = false; 
    envelopes[0].noteOff(); 
    envelopes[1].noteOff(); 
}

float Voice::process(const LfoModulationValues& lfoMod, float currentPitchBendValue, float pitchBendRangeInSemitones) {
    if (isGliding) {
        glideSamplesElapsed++;
        if (glideSamplesElapsed >= glideTimeSamples) {
            currentOutputFreq = targetKeyFreq; // Target is unbent
            isGliding = false;
        } else {
            currentOutputFreq = glideStartFreqForCurrentSegment * std::exp(glideLogStep * static_cast<float>(glideSamplesElapsed));
        }
    } else if (active) { // Ensure currentOutputFreq is up-to-date if not gliding but active
        currentOutputFreq = targetKeyFreq;
    }
    
    if (!active && !envelopes[0].isActive() && !envelopes[1].isActive()) {
        return 0.0f;
    }
    
    float filterEnvOutput_raw = envelopes[0].step();
    float ampEnvOutput_raw = envelopes[1].step();

    float filter_velocity_scaler = (1.0f - filterEnvVelocitySensitivity) + (velocityValue * filterEnvVelocitySensitivity);
    float filterEnvOutput = filterEnvOutput_raw * filter_velocity_scaler; 

    float drift1_pitch_val = analogDriftPitch1.process(); 
    float drift2_pitch_val = analogDriftPitch2.process(); 
    float drift1_pw_val = analogDriftPW1.process();       
    float drift2_pw_val = analogDriftPW2.process();       

    float osc1_pitch_drift_cents = drift1_pitch_val * pitchDriftDepthCents;
    float osc2_pitch_drift_cents = drift2_pitch_val * pitchDriftDepthCents; 
    float osc1_pw_drift_offset = drift1_pw_val * pwDriftDepth;
    float osc2_pw_drift_offset = drift2_pw_val * pwDriftDepth;

    osc1.setPWMSource(lfoMod.osc1PwMod);
    osc1.setWheelModPWValue(lfoMod.wheelOsc1PwOffset); 
    osc2.setPWMSource(lfoMod.osc2PwMod);
    osc2.setWheelModPWValue(lfoMod.wheelOsc2PwOffset); 
    
    // --- VCO B (osc2) Frequency Calculation ---
    float osc2_final_fundamental_freq;
    if (vcoBLowFreqEnabled) {
        const float minLfoRate = 0.05f;
        const float maxLfoRate = 20.0f;
        float osc2_lfo_base_rate;
        if (minLfoRate > 0.0f && maxLfoRate > minLfoRate) {
             osc2_lfo_base_rate = minLfoRate * std::pow(maxLfoRate / minLfoRate, vcoBFreqKnob);
        } else {
             osc2_lfo_base_rate = minLfoRate + vcoBFreqKnob * (maxLfoRate - minLfoRate);
        }
        // lfoMod.osc2FreqMod contains LFO+Wheel for VCO2 Freq. Global pitch bend does not apply to LFO rate.
        osc2_final_fundamental_freq = osc2_lfo_base_rate * std::pow(2.0f, lfoMod.osc2FreqMod / 12.0f);
    } else { // Audio rate for VCO B
        float baseFreqVCOB_unbent = (vcoBKeyFollowEnabled_ ? this->currentOutputFreq 
                                                            : ((vcoBFixedBaseFreq_ < 0.0f) ? 261.63f : vcoBFixedBaseFreq_));
        
        float driftedBaseFreqVCOB = baseFreqVCOB_unbent * std::pow(2.0f, osc2_pitch_drift_cents / 1200.0f);
        
        float totalPitchModSemitonesVCOB = lfoMod.osc2FreqMod + (currentPitchBendValue * pitchBendRangeInSemitones);
        float freqAfterPitchModsVCOB = driftedBaseFreqVCOB * std::pow(2.0f, totalPitchModSemitonesVCOB / 12.0f);
        
        float semitone_offset_from_knob = (vcoBFreqKnob - 0.5f) * 2.0f * 30.0f;
        float freqAfterKnobVCOB = freqAfterPitchModsVCOB * std::pow(2.0f, semitone_offset_from_knob / 12.0f);
        
        osc2_final_fundamental_freq = freqAfterKnobVCOB * std::pow(2.0f, vcoBDetuneCents / 1200.0f);
    }
    osc2.setFrequency(std::max(0.0f, osc2_final_fundamental_freq));
    osc2.setDriftPWValue(osc2_pw_drift_offset); 
    float s2_output = osc2.process(); 
    
    // --- VCO A (osc1) PW PolyMod ---
    float filterEnv_pwm_mod_scaled = (filterEnvOutput - 0.5f) * 2.0f; // Bipolar envelope
    float vco1_pm_env_pw_effect = filterEnv_pwm_mod_scaled * pm_filterEnv_to_pwA_amt * 0.5f;
    float vco1_pm_oscB_pw_effect = s2_output * pm_oscB_to_pwA_amt * 0.5f;
    osc1.setPolyModPWValue(vco1_pm_env_pw_effect + vco1_pm_oscB_pw_effect); 

    if (syncEnabled && s2_output > 0.0f && lastOsc2 <= 0.0f) {
        osc1.sync();
    }
    lastOsc2 = s2_output;

    // --- VCO A (osc1) Frequency Calculation ---
    float baseFreqVCOA_unbent_glided = this->currentOutputFreq; // Glided, unbent frequency
    
    float driftedBaseFreqVCOA = baseFreqVCOA_unbent_glided * std::pow(2.0f, osc1_pitch_drift_cents / 1200.0f);
    
    float totalPitchModSemitonesVCOA = lfoMod.osc1FreqMod + (currentPitchBendValue * pitchBendRangeInSemitones);
    float freqAfterPitchModsVCOA = driftedBaseFreqVCOA * std::pow(2.0f, totalPitchModSemitonesVCOA / 12.0f);
    
    // PolyMod Freq offsets are scaled by the unbent, glided base frequency for consistency
    float pm_env_to_freqA_hz_offset = ((filterEnvOutput - 0.5f) * 2.0f) * pm_filterEnv_to_freqA_amt * (baseFreqVCOA_unbent_glided * 2.0f);
    float pm_oscB_to_freqA_hz_offset = s2_output * pm_oscB_to_freqA_amt * (baseFreqVCOA_unbent_glided * 5.0f);
    
    float finalOsc1Freq = freqAfterPitchModsVCOA + pm_env_to_freqA_hz_offset + pm_oscB_to_freqA_hz_offset;
    osc1.setFrequency(std::max(0.0f, finalOsc1Freq)); 
    osc1.setDriftPWValue(osc1_pw_drift_offset); 

    float s1_output = osc1.process();

    // --- Mixer & Filter ---
    float noise = distribution(generator);
    float mixed = (osc1Level * s1_output + osc2Level * s2_output + noiseLevel * noise);

    float vcfLfoModOffset = lfoMod.vcfCutoffMod; // Already includes LFO + Wheel for VCF in Hz
    // PolyMod to Filter Cutoff (Hz offsets)
    float pm_env_to_vcf_hz_offset = ((filterEnvOutput - 0.5f) * 2.0f) * pm_filterEnv_to_filterCutoff_amt * 2000.0f;
    float pm_oscB_to_vcf_hz_offset = s2_output * pm_oscB_to_filterCutoff_amt * 2000.0f;
    
    filter.setEnvelopeValue(filterEnvOutput); 
    float directVcfModHz = vcfLfoModOffset + pm_env_to_vcf_hz_offset + pm_oscB_to_vcf_hz_offset;
    float filtered = filter.process(mixed, directVcfModHz);

    // --- Amplitude Envelope & Velocity ---
    float amp_velocity_scaler = (1.0f - ampVelocitySensitivity) + (velocityValue * ampVelocitySensitivity);
    float ampEnvOutput = ampEnvOutput_raw * amp_velocity_scaler;
    float finalOutput = filtered * ampEnvOutput;
    
    return finalOutput;
}

bool Voice::isActive() const {
    return active || envelopes[0].isActive() || envelopes[1].isActive();
}

void Voice::setWaveform(Waveform wf) {
    osc1.setWaveform(wf);
    osc2.setWaveform(wf);
}

void Voice::setOsc1Level(float level) { osc1Level = level; }
void Voice::setOsc2Level(float level) { osc2Level = level; }
void Voice::setNoiseLevel(float level) { noiseLevel = level; }
void Voice::setMixLevels(float level1, float level2, float noise) {
    osc1Level = level1;
    osc2Level = level2;
    noiseLevel = noise;
}

void Voice::setVCOBDetuneCents(float cents) { vcoBDetuneCents = cents; }
void Voice::setSyncEnabled(bool enabled) { syncEnabled = enabled; }
void Voice::setPulseWidth(float width) {
    osc1.setPulseWidth(width);
    osc2.setPulseWidth(width);
}
void Voice::setPWMDepth(float depth) {
    osc1.setPWMDepth(depth);
    osc2.setPWMDepth(depth);
}
void Voice::setVCOBLowFreqEnabled(bool enabled) {
    vcoBLowFreqEnabled = enabled;
}
void Voice::setVCOBFreqKnob(float knobValue) {
    vcoBFreqKnob = std::clamp(knobValue, 0.0f, 1.0f);
}

void Voice::setVCOBKeyFollowEnabled(bool enabled) {
    vcoBKeyFollowEnabled_ = enabled;
    if (enabled && active) { // If re-enabled while a note is held, update fixed base immediately
        vcoBFixedBaseFreq_ = this->targetKeyFreq;
    }
}

void Voice::setPMFilterEnvToFreqAAmount(float amount) {
    pm_filterEnv_to_freqA_amt = std::clamp(amount, 0.0f, 1.0f);
}
void Voice::setPMFilterEnvToPWAAmount(float amount) {
    pm_filterEnv_to_pwA_amt = std::clamp(amount, 0.0f, 1.0f);
}
void Voice::setPMFilterEnvToFilterCutoffAmount(float amount) {
    pm_filterEnv_to_filterCutoff_amt = std::clamp(amount, 0.0f, 1.0f);
}
void Voice::setPMOscBToFreqAAmount(float amount) {
    pm_oscB_to_freqA_amt = std::clamp(amount, 0.0f, 1.0f);
}
void Voice::setPMOscBToPWAAmount(float amount) {
    pm_oscB_to_pwA_amt = std::clamp(amount, 0.0f, 1.0f);
}
void Voice::setPMOscBToFilterCutoffAmount(float amount) {
    pm_oscB_to_filterCutoff_amt = std::clamp(amount, 0.0f, 1.0f);
}
void Voice::setVCFResonance(float q)         { filter.setResonance(q); }
void Voice::setVCFKeyFollow(float f)         { filter.setKeyFollow(f); }
void Voice::setVCFEnvelopeAmount(float amt)  { filter.setEnvelopeMod(amt); }
void Voice::setVCFBaseCutoff(float hz)       { filter.setBaseCutoff(hz); }

void Voice::setAmpEnvelope(const EnvelopeParams& p) {
    envelopes[1] = Envelope(p.attack, p.decay, p.sustain, p.release, sampleRate);
}

void Voice::setFilterEnvelope(const EnvelopeParams& p) {
    envelopes[0] = Envelope(p.attack, p.decay, p.sustain, p.release, sampleRate);
}

void Voice::setFilterEnvVelocitySensitivity(float amount) {
    filterEnvVelocitySensitivity = std::clamp(amount, 0.0f, 1.0f);
}

void Voice::setAmpVelocitySensitivity(float amount) {
    ampVelocitySensitivity = std::clamp(amount, 0.0f, 1.0f);
}

void Voice::setPitchDriftDepth(float cents) {
    pitchDriftDepthCents = cents;
}

void Voice::setPWDriftDepth(float depth) {
    pwDriftDepth = depth;
}

void Voice::setOsc1HarmonicAmplitude(int harmonicIndex, float amplitude) {
    osc1.setHarmonicAmplitude(harmonicIndex, amplitude);
}

void Voice::setOsc2HarmonicAmplitude(int harmonicIndex, float amplitude) {
    osc2.setHarmonicAmplitude(harmonicIndex, amplitude);
}