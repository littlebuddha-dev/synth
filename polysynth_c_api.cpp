#include "polysynth_c_api.h"
#include "poly_synth.h" 
#include "waveform.h"   
#include "envelope.h"   
#include "synth_parameters.h" // For SynthParams::ParamID
#include "lfo.h" // For LfoWaveform

// Helper to convert C enum to C++ enum 
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

PS_Waveform map_cpp_waveform_to_ps(Waveform wf_cpp) {
    switch (wf_cpp) {
        case Waveform::Sine: return PS_WAVEFORM_SINE;
        case Waveform::Saw: return PS_WAVEFORM_SAW;
        case Waveform::Square: return PS_WAVEFORM_SQUARE;
        case Waveform::Triangle: return PS_WAVEFORM_TRIANGLE;
        case Waveform::Pulse: return PS_WAVEFORM_PULSE;
        case Waveform::Additive: return PS_WAVEFORM_ADDITIVE;
        default: return PS_WAVEFORM_SAW;
    }
}

LfoWaveform map_ps_lfo_waveform_to_cpp(PS_LfoWaveform wf_c) {
    switch (wf_c) {
        case PS_LFO_WAVEFORM_TRIANGLE: return LfoWaveform::Triangle;
        case PS_LFO_WAVEFORM_SAW_UP: return LfoWaveform::SawUp;
        case PS_LFO_WAVEFORM_SQUARE: return LfoWaveform::Square;
        case PS_LFO_WAVEFORM_SINE: return LfoWaveform::Sine;
        case PS_LFO_WAVEFORM_RANDOM_STEP: return LfoWaveform::RandomStep;
        default: return LfoWaveform::Triangle;
    }
}

PS_LfoWaveform map_cpp_lfo_waveform_to_ps(LfoWaveform wf_cpp) {
    switch (wf_cpp) {
        case LfoWaveform::Triangle: return PS_LFO_WAVEFORM_TRIANGLE;
        case LfoWaveform::SawUp: return PS_LFO_WAVEFORM_SAW_UP;
        case LfoWaveform::Square: return PS_LFO_WAVEFORM_SQUARE;
        case LfoWaveform::Sine: return PS_LFO_WAVEFORM_SINE;
        case LfoWaveform::RandomStep: return PS_LFO_WAVEFORM_RANDOM_STEP;
        default: return PS_LFO_WAVEFORM_TRIANGLE;
    }
}


// Helper to map C_ParamID to SynthParams::ParamID
// This needs to be comprehensive or use a direct cast if enums are kept in sync.
SynthParams::ParamID map_c_param_id_to_cpp(SynthParams::C_ParamID c_id) {
    // For now, direct cast assuming values are identical.
    // A robust solution would use a switch statement or a map.
    if (static_cast<int>(c_id) >= 0 && static_cast<int>(c_id) < static_cast<int>(SynthParams::ParamID::NumParameters)) {
         return static_cast<SynthParams::ParamID>(c_id);
    }
    // Handle error or return a default. For now, let's assume valid input for simplicity.
    // This could cause issues if C_ParamID and ParamID diverge.
    return SynthParams::ParamID::MasterTuneCents; // Fallback, should log error
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

    // This is where you'd have a large switch statement or a map
    // to call the correct PolySynth setter based on param_id_cpp.
    // Example for a few parameters:
    switch (param_id_cpp) {
        case SynthParams::ParamID::MasterTuneCents:    synth->setMasterTuneCents(value); break;
        case SynthParams::ParamID::Osc1Level:          synth->setOsc1Level(value); break;
        case SynthParams::ParamID::Osc2Level:          synth->setOsc2Level(value); break;
        case SynthParams::ParamID::NoiseLevel:         synth->setNoiseLevel(value); break;
        case SynthParams::ParamID::RingModLevel:       synth->setRingModLevel(value); break;
        case SynthParams::ParamID::VCOBDetuneCents:    synth->setVCOBDetuneCents(value); break;
        case SynthParams::ParamID::VCOBFreqKnob:       synth->setVCOBFreqKnob(value); break;
        // ... many more parameters
        case SynthParams::ParamID::UnisonDetuneCents:  synth->setUnisonDetuneCents(value); break;
        case SynthParams::ParamID::UnisonStereoSpread: synth->setUnisonStereoSpread(value); break;
        case SynthParams::ParamID::VCFBaseCutoff:      synth->setVCFBaseCutoff(value); break;
        case SynthParams::ParamID::VCFResonance:       synth->setVCFResonance(value); break;
        case SynthParams::ParamID::VCFEnvelopeAmount:  synth->setVCFEnvelopeAmount(value); break;
        case SynthParams::ParamID::LfoRate:            synth->setLfoRate(value); break;
        // ... etc.
        default:
            // Handle unknown or type-mismatched parameter
            // std::cerr << "ps_set_float_param: Unhandled or type-mismatched C_ParamID: " << param_id_c << std::endl;
            break;
    }
}

void ps_set_int_param(PolySynthHandle handle, SynthParams::C_ParamID param_id_c, int value) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    SynthParams::ParamID param_id_cpp = map_c_param_id_to_cpp(param_id_c);

    switch (param_id_cpp) {
        case SynthParams::ParamID::Waveform:       synth->setWaveform(map_ps_waveform_to_cpp(static_cast<PS_Waveform>(value))); break;
        case SynthParams::ParamID::SyncEnabled:    synth->setSyncEnabled(static_cast<bool>(value)); break;
        case SynthParams::ParamID::UnisonEnabled:  synth->setUnisonEnabled(static_cast<bool>(value)); break;
        case SynthParams::ParamID::LfoWaveform:    synth->setLfoWaveform(map_ps_lfo_waveform_to_cpp(static_cast<PS_LfoWaveform>(value))); break;
        // ... other int/bool/enum parameters
        default:
            // Handle unknown or type-mismatched parameter
            // std::cerr << "ps_set_int_param: Unhandled or type-mismatched C_ParamID: " << param_id_c << std::endl;
            break;
    }
}


void ps_set_waveform_c(PolySynthHandle handle, PS_Waveform wf_c) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setWaveform(map_ps_waveform_to_cpp(wf_c));
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

void ps_set_vcf_base_cutoff(PolySynthHandle handle, float hz) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setVCFBaseCutoff(hz);
}
void ps_set_vcf_resonance(PolySynthHandle handle, float q) {
    if (!handle) return;
    static_cast<PolySynth*>(handle)->setVCFResonance(q);
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


} // extern "C"