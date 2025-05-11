#ifndef POLYSYNTH_C_API_H
#define POLYSYNTH_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque pointer type for the PolySynth instance
typedef void* PolySynthHandle;

// Enum for waveforms, LFO waves, etc. (C-compatible)
// These should mirror your C++ enums but be plain C enums.
typedef enum {
    PS_WAVEFORM_SINE = 0,
    PS_WAVEFORM_SAW,
    PS_WAVEFORM_SQUARE,
    PS_WAVEFORM_TRIANGLE,
    PS_WAVEFORM_PULSE
} PS_Waveform;

// Lifecycle
PolySynthHandle ps_create_synth(int sample_rate, int max_voices);
void ps_destroy_synth(PolySynthHandle handle);

// Audio Processing
// (Note: VST/plugin integration might directly call C++ process methods for performance)
// This C API version is more for if you want to run the synth in a separate process/thread
// and get audio blocks via C.
void ps_process_audio(PolySynthHandle handle, float* output_buffer, int num_frames);

// Note Control
void ps_note_on(PolySyn`thHandle handle, int midi_note, float velocity);
void ps_note_off(PolySynthHandle handle, int midi_note);

// Global Parameters
void ps_set_waveform(PolySynthHandle handle, PS_Waveform wf);
void ps_set_osc1_level(PolySynthHandle handle, float level); // 0.0 to 1.0
void ps_set_osc2_level(PolySynthHandle handle, float level); // 0.0 to 1.0
void ps_set_noise_level(PolySynthHandle handle, float level); // 0.0 to 1.0
// ... (多くのパラメータセッターが続く) ...
void ps_set_vcf_base_cutoff(PolySynthHandle handle, float hz); // 20.0 to 20000.0
void ps_set_vcf_resonance(PolySynthHandle handle, float q);   // 0.0 to 1.0

// Example for setting an envelope parameter
typedef struct {
    float attack;
    float decay;
    float sustain;
    float release;
} PS_EnvelopeParams;
void ps_set_amp_envelope(PolySynthHandle handle, PS_EnvelopeParams params);


// SynthParams::C_ParamID は synth_parameters.h で定義したC互換enum
void ps_set_parameter_float(PolySynthHandle handle, SynthParams::C_ParamID param_id_c, float value);
void ps_set_parameter_int(PolySynthHandle handle, SynthParams::C_ParamID param_id_c, int value);
// float ps_get_parameter_float(PolySynthHandle handle, SynthParams::C_ParamID param_id_c);
// int ps_get_parameter_int(PolySynthHandle handle, SynthParams::C_ParamID param_id_c);

void ps_set_envelope_params_c(PolySynthHandle handle, SynthParams::C_ParamID base_param_id_c, PS_EnvelopeParams params_c);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // POLYSYNTH_C_API_H