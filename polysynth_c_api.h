#ifndef POLYSYNTH_C_API_H
#define POLYSYNTH_C_API_H

#include "synth_parameters.h" 

#ifdef __cplusplus
extern "C" {
#endif

typedef void* PolySynthHandle;

typedef enum {
    PS_WAVEFORM_SINE = 0,
    PS_WAVEFORM_SAW,
    PS_WAVEFORM_SQUARE,
    PS_WAVEFORM_TRIANGLE,
    PS_WAVEFORM_PULSE,
    PS_WAVEFORM_ADDITIVE 
} PS_Waveform;

typedef enum {
    PS_LFO_WAVEFORM_TRIANGLE = 0,
    PS_LFO_WAVEFORM_SAW_UP,
    PS_LFO_WAVEFORM_SQUARE,
    PS_LFO_WAVEFORM_SINE,
    PS_LFO_WAVEFORM_RANDOM_STEP
} PS_LfoWaveform;


PolySynthHandle ps_create_synth(int sample_rate, int max_voices);
void ps_destroy_synth(PolySynthHandle handle);

void ps_process_audio(PolySynthHandle handle, float* output_buffer, int num_frames);

void ps_note_on(PolySynthHandle handle, int midi_note, float velocity);
void ps_note_off(PolySynthHandle handle, int midi_note);

void ps_set_float_param(PolySynthHandle handle, SynthParams::C_ParamID param_id, float value);
void ps_set_int_param(PolySynthHandle handle, SynthParams::C_ParamID param_id, int value); 


void ps_set_waveform_c(PolySynthHandle handle, PS_Waveform wf); 
void ps_set_osc1_level(PolySynthHandle handle, float level); 
void ps_set_osc2_level(PolySynthHandle handle, float level); 
void ps_set_noise_level(PolySynthHandle handle, float level);
void ps_set_ring_mod_level(PolySynthHandle handle, float level); 

void ps_set_filter_type(PolySynthHandle handle, SynthParams::PS_FilterType type_c); 
void ps_set_vcf_base_cutoff(PolySynthHandle handle, float hz); 
void ps_set_vcf_resonance(PolySynthHandle handle, float q);   

void ps_set_xmod_osc2_to_osc1_fm_amount(PolySynthHandle handle, float amount);
void ps_set_xmod_osc1_to_osc2_fm_amount(PolySynthHandle handle, float amount);

void ps_set_mixer_drive(PolySynthHandle handle, float drive);
void ps_set_mixer_post_gain(PolySynthHandle handle, float gain);


typedef struct {
    float attack;
    float decay;
    float sustain;
    float release;
} PS_EnvelopeParams;
void ps_set_amp_envelope(PolySynthHandle handle, PS_EnvelopeParams params);
void ps_set_filter_envelope(PolySynthHandle handle, PS_EnvelopeParams params);

void ps_set_unison_enabled(PolySynthHandle handle, int enabled); 
void ps_set_unison_detune_cents(PolySynthHandle handle, float cents);
void ps_set_unison_stereo_spread(PolySynthHandle handle, float spread);

void ps_set_osc_harmonic_amplitude(PolySynthHandle handle, int osc_num, int harmonic_index, float amplitude);

// Reverb specific C API (could also be part of ps_set_float_param with C_PARAM_REVERB_... IDs)
void ps_reverb_set_enabled(PolySynthHandle handle, int effect_index, int enabled);
void ps_reverb_set_dry_wet_mix(PolySynthHandle handle, int effect_index, float mix);
void ps_reverb_set_room_size(PolySynthHandle handle, int effect_index, float size);
void ps_reverb_set_damping(PolySynthHandle handle, int effect_index, float damping);
void ps_reverb_set_wet_gain(PolySynthHandle handle, int effect_index, float gain);
void ps_reverb_set_rt60(PolySynthHandle handle, int effect_index, float rt60);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // POLYSYNTH_C_API_H