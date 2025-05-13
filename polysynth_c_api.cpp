#include "polysynth_c_api.h"
#include "poly_synth.h" 
#include "waveform.h"   
#include "envelope.h"   
#include "synth_parameters.h" 
#include "lfo.h" 
#include "effects/reverb_effect.h" // For casting to ReverbEffect


LfoWaveform map_ps_lfo_waveform_to_cpp(PS_LfoWaveform wf_c) {
    switch (wf_c) {
        case PS_LFO_WAVEFORM_TRIANGLE:   return LfoWaveform::Triangle;
        case PS_LFO_WAVEFORM_SAW_UP:     return LfoWaveform::SawUp;
        case PS_LFO_WAVEFORM_SQUARE:     return LfoWaveform::Square;
        case PS_LFO_WAVEFORM_SINE:       return LfoWaveform::Sine;
        case PS_LFO_WAVEFORM_RANDOM_STEP:return LfoWaveform::RandomStep;
        default:                         return LfoWaveform::Triangle;
    }
}


Waveform map_ps_waveform_to_cpp(PS_Waveform wf_c) {
    switch (wf_c) {
        case PS_WAVEFORM_SINE: return Waveform::Sine;
        case PS_WAVEFORM_SAW: return Waveform::Saw;
        case PS_WAVEFORM_SQUARE: return Waveform::Square;
        case PS_WAVEFORM_TRIANGLE: return Waveform::Triangle;
        case PS_WAVEFORM_PULSE: return Waveform::Pulse;
        case PS_WAVEFORM_ADDITIVE: return Waveform::Additive;
        default: return Waveform::Saw; 
    }
}


SynthParams::FilterType map_ps_filter_type_to_cpp(SynthParams::PS_FilterType type_c) {
    switch (type_c) {
        case SynthParams::PS_FILTER_TYPE_LPF24: return SynthParams::FilterType::LPF24;
        case SynthParams::PS_FILTER_TYPE_LPF12: return SynthParams::FilterType::LPF12;
        case SynthParams::PS_FILTER_TYPE_HPF12: return SynthParams::FilterType::HPF12;
        case SynthParams::PS_FILTER_TYPE_BPF12: return SynthParams::FilterType::BPF12;
        case SynthParams::PS_FILTER_TYPE_NOTCH: return SynthParams::FilterType::NOTCH;
        default: return SynthParams::FilterType::LPF24;
    }
}


SynthParams::ParamID map_c_param_id_to_cpp(SynthParams::C_ParamID c_id) {
    if (static_cast<int>(c_id) >= 0 && static_cast<int>(c_id) < static_cast<int>(SynthParams::ParamID::NumParameters)) {
         return static_cast<SynthParams::ParamID>(c_id);
    }
    return SynthParams::ParamID::MasterTuneCents; 
}


extern "C" {

PolySynthHandle ps_create_synth(int sample_rate, int max_voices) {
    return new PolySynth(sample_rate, max_voices);
}

void ps_destroy_synth(PolySynthHandle handle) {
    delete static_cast<PolySynth*>(handle);
}

void ps_process_audio(PolySynthHandle handle, float* output_buffer, int num_frames) {
    if (!handle || !output_buffer) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    for (int i = 0; i < num_frames; ++i) {
        StereoSample sample = synth->process();
        output_buffer[2 * i] = sample.L;
        output_buffer[2 * i + 1] = sample.R;
    }
}

void ps_note_on(PolySynthHandle handle, int midi_note, float velocity) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->noteOn(midi_note, velocity);
}

void ps_note_off(PolySynthHandle handle, int midi_note) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->noteOff(midi_note);
}

void ps_set_float_param(PolySynthHandle handle, SynthParams::C_ParamID param_id_c, float value) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    SynthParams::ParamID param_id_cpp = map_c_param_id_to_cpp(param_id_c);

    switch (param_id_cpp) {
        case SynthParams::ParamID::MasterTuneCents:    synth->setMasterTuneCents(value); break;
        case SynthParams::ParamID::Osc1Level:          synth->setOsc1Level(value); break;
        case SynthParams::ParamID::Osc2Level:          synth->setOsc2Level(value); break;
        case SynthParams::ParamID::NoiseLevel:         synth->setNoiseLevel(value); break;
        case SynthParams::ParamID::RingModLevel:       synth->setRingModLevel(value); break;
        case SynthParams::ParamID::VCOBDetuneCents:    synth->setVCOBDetuneCents(value); break;
        case SynthParams::ParamID::VCOBFreqKnob:       synth->setVCOBFreqKnob(value); break;
        case SynthParams::ParamID::FilterEnvVelocitySensitivity: synth->setFilterEnvVelocitySensitivity(value); break;
        case SynthParams::ParamID::AmpVelocitySensitivity: synth->setAmpVelocitySensitivity(value); break;
        case SynthParams::ParamID::PulseWidth:         synth->setPulseWidth(value); break;
        case SynthParams::ParamID::PWMDepth:           synth->setPWMDepth(value); break;
        case SynthParams::ParamID::XModOsc2ToOsc1FMAmount: synth->setXModOsc2ToOsc1FMAmount(value); break;
        case SynthParams::ParamID::XModOsc1ToOsc2FMAmount: synth->setXModOsc1ToOsc2FMAmount(value); break;
        case SynthParams::ParamID::PMFilterEnvToFreqAAmount: synth->setPMFilterEnvToFreqAAmount(value); break;
        case SynthParams::ParamID::PMFilterEnvToPWAAmount: synth->setPMFilterEnvToPWAAmount(value); break;
        case SynthParams::ParamID::PMFilterEnvToFilterCutoffAmount: synth->setPMFilterEnvToFilterCutoffAmount(value); break;
        case SynthParams::ParamID::PMOscBToPWAAmount:  synth->setPMOscBToPWAAmount(value); break;
        case SynthParams::ParamID::PMOscBToFilterCutoffAmount: synth->setPMOscBToFilterCutoffAmount(value); break;
        case SynthParams::ParamID::VCFBaseCutoff:      synth->setVCFBaseCutoff(value); break;
        case SynthParams::ParamID::VCFResonance:       synth->setVCFResonance(value); break;
        case SynthParams::ParamID::VCFKeyFollow:       synth->setVCFKeyFollow(value); break;
        case SynthParams::ParamID::VCFEnvelopeAmount:  synth->setVCFEnvelopeAmount(value); break;
        case SynthParams::ParamID::MixerDrive:         synth->setMixerDrive(value); break;         
        case SynthParams::ParamID::MixerPostGain:      synth->setMixerPostGain(value); break;      
        case SynthParams::ParamID::AmpEnvAttack:       
        case SynthParams::ParamID::AmpEnvDecay:        
        case SynthParams::ParamID::AmpEnvSustain:
        case SynthParams::ParamID::AmpEnvRelease:
        case SynthParams::ParamID::FilterEnvAttack:    
        case SynthParams::ParamID::FilterEnvDecay:     
        case SynthParams::ParamID::FilterEnvSustain:
        case SynthParams::ParamID::FilterEnvRelease:
            break; 
        case SynthParams::ParamID::LfoRate:            synth->setLfoRate(value); break;
        case SynthParams::ParamID::LfoAmountToVco1Freq: synth->setLfoAmountToVco1Freq(value); break;
        case SynthParams::ParamID::LfoAmountToVco2Freq: synth->setLfoAmountToVco2Freq(value); break;
        case SynthParams::ParamID::LfoAmountToVco1Pw:  synth->setLfoAmountToVco1Pw(value); break;
        case SynthParams::ParamID::LfoAmountToVco2Pw:  synth->setLfoAmountToVco2Pw(value); break;
        case SynthParams::ParamID::LfoAmountToVcfCutoff: synth->setLfoAmountToVcfCutoff(value); break;
        case SynthParams::ParamID::ModulationWheelValue: synth->setModulationWheelValue(value); break;
        case SynthParams::ParamID::WheelModAmountToFreqA: synth->setWheelModAmountToFreqA(value); break;
        case SynthParams::ParamID::WheelModAmountToFreqB: synth->setWheelModAmountToFreqB(value); break;
        case SynthParams::ParamID::WheelModAmountToPWA: synth->setWheelModAmountToPWA(value); break;
        case SynthParams::ParamID::WheelModAmountToPWB: synth->setWheelModAmountToPWB(value); break;
        case SynthParams::ParamID::WheelModAmountToFilter: synth->setWheelModAmountToFilter(value); break;
        case SynthParams::ParamID::UnisonDetuneCents:  synth->setUnisonDetuneCents(value); break;
        case SynthParams::ParamID::UnisonStereoSpread: synth->setUnisonStereoSpread(value); break;
        case SynthParams::ParamID::GlideTime:          synth->setGlideTime(value); break;
        case SynthParams::ParamID::AnalogPitchDriftDepth: synth->setAnalogPitchDriftDepth(value); break;
        case SynthParams::ParamID::AnalogPWDriftDepth: synth->setAnalogPWDriftDepth(value); break;
        
        // Reverb params (generic setter)
        case SynthParams::ParamID::ReverbDryWetMix: {
            AudioEffect* effect = synth->getEffect(0); // Assuming reverb is the first effect
            if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
                reverb->setDryWetMix(value);
            }
            break;
        }
        case SynthParams::ParamID::ReverbRoomSize: {
            AudioEffect* effect = synth->getEffect(0);
            if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
                reverb->setRoomSize(value);
            }
            break;
        }
        case SynthParams::ParamID::ReverbDamping: {
            AudioEffect* effect = synth->getEffect(0);
            if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
                reverb->setDamping(value);
            }
            break;
        }
        case SynthParams::ParamID::ReverbWetGain: {
            AudioEffect* effect = synth->getEffect(0);
            if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
                reverb->setWetGain(value);
            }
            break;
        }
        case SynthParams::ParamID::ReverbRT60: {
            AudioEffect* effect = synth->getEffect(0);
            if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
                reverb->setRT60(value);
            }
            break;
        }
        default:
            break;
    }
}

void ps_set_int_param(PolySynthHandle handle, SynthParams::C_ParamID param_id_c, int value) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    SynthParams::ParamID param_id_cpp = map_c_param_id_to_cpp(param_id_c);

    switch (param_id_cpp) {
        case SynthParams::ParamID::Osc1Waveform:
            synth->setOsc1Waveform(map_ps_waveform_to_cpp(static_cast<PS_Waveform>(value)));
            break;
        case SynthParams::ParamID::Osc2Waveform:
            synth->setOsc2Waveform(map_ps_waveform_to_cpp(static_cast<PS_Waveform>(value)));
            break;
        case SynthParams::ParamID::SyncEnabled:        
            synth->setSyncEnabled(static_cast<bool>(value)); 
            break;
        case SynthParams::ParamID::VCOBLowFreqEnabled: 
            synth->setVCOBLowFreqEnabled(static_cast<bool>(value)); 
            break;
        case SynthParams::ParamID::FilterType:     
            synth->setFilterType(map_ps_filter_type_to_cpp(static_cast<SynthParams::PS_FilterType>(value))); 
            break; 
        case SynthParams::ParamID::LfoWaveform:
            synth->setLfoWaveform(map_ps_lfo_waveform_to_cpp(static_cast<PS_LfoWaveform>(value)));
            break;
        case SynthParams::ParamID::WheelModSource: 
            synth->setWheelModSource(static_cast<WheelModSource>(value));
            break;
        case SynthParams::ParamID::UnisonEnabled:
            synth->setUnisonEnabled(static_cast<bool>(value));
            break;
        case SynthParams::ParamID::GlideEnabled:
            synth->setGlideEnabled(static_cast<bool>(value));
            break;
        case SynthParams::ParamID::ReverbEnabled: {
             AudioEffect* effect = synth->getEffect(0); // Assuming reverb is the first effect
            if (effect) { // Check if effect exists
                effect->setEnabled(static_cast<bool>(value));
            }
            break;
        }
        default:
            break;
    }
}


void ps_set_osc1_waveform_c(PolySynthHandle handle, PS_Waveform wf_c) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setOsc1Waveform(map_ps_waveform_to_cpp(wf_c));
}

void ps_set_osc2_waveform_c(PolySynthHandle handle, PS_Waveform wf_c) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setOsc2Waveform(map_ps_waveform_to_cpp(wf_c));
}

void ps_set_osc1_level(PolySynthHandle handle, float level) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setOsc1Level(level);
}
void ps_set_osc2_level(PolySynthHandle handle, float level) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setOsc2Level(level);
}
void ps_set_noise_level(PolySynthHandle handle, float level) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setNoiseLevel(level);
}
void ps_set_ring_mod_level(PolySynthHandle handle, float level) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setRingModLevel(level);
}


void ps_set_filter_type(PolySynthHandle handle, SynthParams::PS_FilterType type_c) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setFilterType(map_ps_filter_type_to_cpp(type_c));
}


void ps_set_vcf_base_cutoff(PolySynthHandle handle, float hz) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setVCFBaseCutoff(hz);
}
void ps_set_vcf_resonance(PolySynthHandle handle, float q) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setVCFResonance(q);
}

void ps_set_xmod_osc2_to_osc1_fm_amount(PolySynthHandle handle, float amount) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setXModOsc2ToOsc1FMAmount(amount);
}
void ps_set_xmod_osc1_to_osc2_fm_amount(PolySynthHandle handle, float amount) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setXModOsc1ToOsc2FMAmount(amount);
}

void ps_set_mixer_drive(PolySynthHandle handle, float drive) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setMixerDrive(drive);
}
void ps_set_mixer_post_gain(PolySynthHandle handle, float gain) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setMixerPostGain(gain);
}


void ps_set_amp_envelope(PolySynthHandle handle, PS_EnvelopeParams params_c) {
    if (!handle) return;
    EnvelopeParams params_cpp;
    params_cpp.attack = params_c.attack;
    params_cpp.decay = params_c.decay;
    params_cpp.sustain = params_c.sustain;
    params_cpp.release = params_c.release;
    static_cast<PolySynth*>(handle)->setAmpEnvelope(params_cpp);
}

void ps_set_filter_envelope(PolySynthHandle handle, PS_EnvelopeParams params_c) {
    if (!handle) return;
    EnvelopeParams params_cpp;
    params_cpp.attack = params_c.attack;
    params_cpp.decay = params_c.decay;
    params_cpp.sustain = params_c.sustain;
    params_cpp.release = params_c.release;
    static_cast<PolySynth*>(handle)->setFilterEnvelope(params_cpp);
}

void ps_set_unison_enabled(PolySynthHandle handle, int enabled) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setUnisonEnabled(static_cast<bool>(enabled));
}
void ps_set_unison_detune_cents(PolySynthHandle handle, float cents) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setUnisonDetuneCents(cents);
}
void ps_set_unison_stereo_spread(PolySynthHandle handle, float spread) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setUnisonStereoSpread(spread);
}

void ps_set_osc_harmonic_amplitude(PolySynthHandle handle, int osc_num, int harmonic_index, float amplitude) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setOscHarmonicAmplitude(osc_num, harmonic_index, amplitude);
}

// Reverb specific C API
void ps_reverb_set_enabled(PolySynthHandle handle, int effect_index, int enabled) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    AudioEffect* effect = synth->getEffect(static_cast<size_t>(effect_index));
    if (effect) {
        effect->setEnabled(static_cast<bool>(enabled));
    }
}

void ps_reverb_set_dry_wet_mix(PolySynthHandle handle, int effect_index, float mix) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    AudioEffect* effect = synth->getEffect(static_cast<size_t>(effect_index));
    if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
        reverb->setDryWetMix(mix);
    }
}

void ps_reverb_set_room_size(PolySynthHandle handle, int effect_index, float size) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    AudioEffect* effect = synth->getEffect(static_cast<size_t>(effect_index));
    if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
        reverb->setRoomSize(size);
    }
}

void ps_reverb_set_damping(PolySynthHandle handle, int effect_index, float damping) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    AudioEffect* effect = synth->getEffect(static_cast<size_t>(effect_index));
    if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
        reverb->setDamping(damping);
    }
}

void ps_reverb_set_wet_gain(PolySynthHandle handle, int effect_index, float gain) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    AudioEffect* effect = synth->getEffect(static_cast<size_t>(effect_index));
    if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
        reverb->setWetGain(gain);
    }
}

void ps_reverb_set_rt60(PolySynthHandle handle, int effect_index, float rt60) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    AudioEffect* effect = synth->getEffect(static_cast<size_t>(effect_index));
    if (ReverbEffect* reverb = dynamic_cast<ReverbEffect*>(effect)) {
        reverb->setRT60(rt60);
    }
}


} // extern "C"