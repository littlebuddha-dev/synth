#ifndef POLYSYNTH_C_API_H
#define POLYSYNTH_C_API_H

#include "synth_parameters.h" // For C_ParamID (assuming it's C-compatible)

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
    PS_WAVEFORM_PULSE,
    PS_WAVEFORM_ADDITIVE // Added
} PS_Waveform;

typedef enum {
    PS_LFO_WAVEFORM_TRIANGLE = 0,
    PS_LFO_WAVEFORM_SAW_UP,
    PS_LFO_WAVEFORM_SQUARE,
    PS_LFO_WAVEFORM_SINE,
    PS_LFO_WAVEFORM_RANDOM_STEP
} PS_LfoWaveform;


// Lifecycle
PolySynthHandle ps_create_synth(int sample_rate, int max_voices);
void ps_destroy_synth(PolySynthHandle handle);

// Audio Processing
// output_buffer is interleaved stereo (L, R, L, R, ...)
void ps_process_audio(PolySynthHandle handle, float* output_buffer, int num_frames);

// Note Control
void ps_note_on(PolySynthHandle handle, int midi_note, float velocity);
void ps_note_off(PolySynthHandle handle, int midi_note);

// Parameter Setting
// This is a generic setter. You might want more specific ones for type safety or clarity.
void ps_set_float_param(PolySynthHandle handle, SynthParams::C_ParamID param_id, float value);
void ps_set_int_param(PolySynthHandle handle, SynthParams::C_ParamID param_id, int value); // For bools or enum indices
// float ps_get_float_param(PolySynthHandle handle, SynthParams::C_ParamID param_id);
// int ps_get_int_param(PolySynthHandle handle, SynthParams::C_ParamID param_id);


// Specific parameter setters (examples, more would be needed for full coverage)
void ps_set_waveform_c(PolySynthHandle handle, PS_Waveform wf); // Renamed to avoid conflict
void ps_set_osc1_level(PolySynthHandle handle, float level); 
void ps_set_osc2_level(PolySynthHandle handle, float level); 
void ps_set_noise_level(PolySynthHandle handle, float level);
void ps_set_ring_mod_level(PolySynthHandle handle, float level); 
void ps_set_vcf_base_cutoff(PolySynthHandle handle, float hz); 
void ps_set_vcf_resonance(PolySynthHandle handle, float q);   

typedef struct {
    float attack;
    float decay;
    float sustain;
    float release;
} PS_EnvelopeParams;
void ps_set_amp_envelope(PolySynthHandle handle, PS_EnvelopeParams params);
void ps_set_filter_envelope(PolySynthHandle handle, PS_EnvelopeParams params);

void ps_set_unison_enabled(PolySynthHandle handle, int enabled); // 0 or 1
void ps_set_unison_detune_cents(PolySynthHandle handle, float cents);
void ps_set_unison_stereo_spread(PolySynthHandle handle, float spread);

void ps_set_osc_harmonic_amplitude(PolySynthHandle handle, int osc_num, int harmonic_index, float amplitude);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // POLYSYNTH_C_API_H