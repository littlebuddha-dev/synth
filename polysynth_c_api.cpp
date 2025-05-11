#include "polysynth_c_api.h"
#include "poly_synth.h" // Your C++ PolySynth header
#include "waveform.h"   // Your C++ Waveform enum header
#include "envelope.h"   // Your C++ EnvelopeParams header
#include "synth_parameters.h"

// Helper to convert C enum to C++ enum (example for waveform)
Waveform map_ps_waveform_to_cpp(PS_Waveform wf_c) {
    switch (wf_c) {
        case PS_WAVEFORM_SINE: return Waveform::Sine;
        case PS_WAVEFORM_SAW: return Waveform::Saw;
        case PS_WAVEFORM_SQUARE: return Waveform::Square;
        case PS_WAVEFORM_TRIANGLE: return Waveform::Triangle;
        case PS_WAVEFORM_PULSE: return Waveform::Pulse;
        default: return Waveform::Saw; // Default or error
    }
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
        output_buffer[i] = synth->process();
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

void ps_set_waveform(PolySynthHandle handle, PS_Waveform wf_c) {
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

// ... (Implement other C API functions similarly) ...

} // extern "C"

// Helper to map C_ParamID to SynthParams::ParamID
SynthParams::ParamID map_c_param_id_to_cpp(SynthParams::C_ParamID c_id) {
    // ここでC_ParamIDとSynthParams::ParamIDの対応を定義する
    // 例: if (c_id == SynthParams::C_PARAM_MASTER_TUNE_CENTS) return SynthParams::ParamID::MasterTuneCents;
    // 実際にはもっと多くのマッピングが必要
    return static_cast<SynthParams::ParamID>(static_cast<int>(c_id)); // 単純なキャスト（enumの値が一致している前提）
}


void ps_set_parameter_float(PolySynthHandle handle, SynthParams::C_ParamID param_id_c, float value) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    synth->setParameter(map_c_param_id_to_cpp(param_id_c), value);
}

void ps_set_parameter_int(PolySynthHandle handle, SynthParams::C_ParamID param_id_c, int value) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    synth->setParameterInt(map_c_param_id_to_cpp(param_id_c), value);
}

void ps_set_envelope_params_c(PolySynthHandle handle, SynthParams::C_ParamID base_param_id_c, PS_EnvelopeParams params_c) {
    if (!handle) return;
    PolySynth* synth = static_cast<PolySynth*>(handle);
    EnvelopeParams params_cpp;
    params_cpp.attack = params_c.attack;
    params_cpp.decay = params_c.decay;
    params_cpp.sustain = params_c.sustain;
    params_cpp.release = params_c.release;
    synth->setEnvelopeParams(map_c_param_id_to_cpp(base_param_id_c), params_cpp);
}