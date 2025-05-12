#include "polysynth_c_api.h"
#include "poly_synth.h" 
#include "waveform.h"   
#include "envelope.h"   
#include "synth_parameters.h" 
#include "lfo.h" 

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

// ... (other mapping functions remain the same) ...

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
        // ... (many other float params)
        case SynthParams::ParamID::XModOsc2ToOsc1FMAmount: synth->setXModOsc2ToOsc1FMAmount(value); break;
        case SynthParams::ParamID::XModOsc1ToOsc2FMAmount: synth->setXModOsc1ToOsc2FMAmount(value); break;
        case SynthParams::ParamID::VCFBaseCutoff:      synth->setVCFBaseCutoff(value); break;
        case SynthParams::ParamID::VCFResonance:       synth->setVCFResonance(value); break;
        default:
            break;
    }
}

void ps_set_int_param(PolySynthHandle handle, SynthParams::C_ParamID param_id_c, int value) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    SynthParams::ParamID param_id_cpp = map_c_param_id_to_cpp(param_id_c);

    switch (param_id_cpp) {
        case SynthParams::ParamID::Waveform:       synth->setWaveform(map_ps_waveform_to_cpp(static_cast<PS_Waveform>(value))); break;
        case SynthParams::ParamID::FilterType:     synth->setFilterType(map_ps_filter_type_to_cpp(static_cast<SynthParams::PS_FilterType>(value))); break; // New
        // ... (many other int params)
        default:
            break;
    }
}


void ps_set_waveform_c(PolySynthHandle handle, PS_Waveform wf_c) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setWaveform(map_ps_waveform_to_cpp(wf_c));
}

// ... (other specific setters like osc levels, etc.)

// New C API for Filter Type
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

// ... (rest of C API implementation)
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


} // extern "C"