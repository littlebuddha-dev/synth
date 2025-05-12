#pragma once

namespace SynthParams {

// New: FilterType enum
enum class FilterType {
    LPF24, // Ladder Low Pass 24dB/oct (existing)
    LPF12, // SVF Low Pass 12dB/oct
    HPF12, // SVF High Pass 12dB/oct
    BPF12, // SVF Band Pass 12dB/oct
    NOTCH  // SVF Notch (Band Reject)
};

enum class ParamID {
    // Global Voice Parameters
    MasterTuneCents,
    Waveform, 
    Osc1Level,
    Osc2Level,
    NoiseLevel,
    RingModLevel, 
    VCOBDetuneCents,
    SyncEnabled,
    VCOBLowFreqEnabled,
    VCOBFreqKnob,
    FilterEnvVelocitySensitivity,
    AmpVelocitySensitivity,
    PulseWidth, 
    PWMDepth,   

    XModOsc2ToOsc1FMAmount,
    XModOsc1ToOsc2FMAmount,

    PMFilterEnvToFreqAAmount,
    PMFilterEnvToPWAAmount,
    PMFilterEnvToFilterCutoffAmount,
    PMOscBToPWAAmount,
    PMOscBToFilterCutoffAmount,

    // Filter Parameters
    FilterType, // New: To select filter type
    VCFBaseCutoff,
    VCFResonance,
    VCFKeyFollow,
    VCFEnvelopeAmount,

    AmpEnvAttack,
    AmpEnvDecay,
    AmpEnvSustain,
    AmpEnvRelease,
    FilterEnvAttack,
    FilterEnvDecay,
    FilterEnvSustain,
    FilterEnvRelease,

    LfoRate,
    LfoWaveform,
    LfoAmountToVco1Freq,
    LfoAmountToVco2Freq,
    LfoAmountToVco1Pw,
    LfoAmountToVco2Pw,
    LfoAmountToVcfCutoff,

    ModulationWheelValue, 
    WheelModSource,
    WheelModAmountToFreqA,
    WheelModAmountToFreqB,
    WheelModAmountToPWA,
    WheelModAmountToPWB,
    WheelModAmountToFilter,

    UnisonEnabled,
    UnisonDetuneCents,
    UnisonStereoSpread, 

    GlideEnabled,
    GlideTime,

    AnalogPitchDriftDepth,
    AnalogPWDriftDepth,

    ReverbEnabled,
    ReverbDryWetMix,
    ReverbRoomSize,
    ReverbDamping,
    ReverbWetGain,
    ReverbRT60,

    NumParameters 
};


enum C_ParamID {
    C_PARAM_MASTER_TUNE_CENTS = 0,
    C_PARAM_WAVEFORM,
    C_PARAM_OSC1_LEVEL,
    C_PARAM_OSC2_LEVEL,
    C_PARAM_NOISE_LEVEL,
    C_PARAM_RING_MOD_LEVEL, 
    C_PARAM_VCOB_DETUNE_CENTS,
    C_PARAM_SYNC_ENABLED,
    C_PARAM_VCOB_LOW_FREQ_ENABLED,
    C_PARAM_VCOB_FREQ_KNOB,
    C_PARAM_FILTER_ENV_VELOCITY_SENSITIVITY,
    C_PARAM_AMP_VELOCITY_SENSITIVITY,
    C_PARAM_PULSE_WIDTH,
    C_PARAM_PWM_DEPTH,

    C_PARAM_XMOD_OSC2_TO_OSC1_FM_AMOUNT,
    C_PARAM_XMOD_OSC1_TO_OSC2_FM_AMOUNT,

    C_PARAM_PM_FILTER_ENV_TO_FREQ_A_AMOUNT,
    C_PARAM_PM_FILTER_ENV_TO_PW_A_AMOUNT,
    C_PARAM_PM_FILTER_ENV_TO_FILTER_CUTOFF_AMOUNT,
    C_PARAM_PM_OSCB_TO_PW_A_AMOUNT,
    C_PARAM_PM_OSCB_TO_FILTER_CUTOFF_AMOUNT,

    // Filter Parameters
    C_PARAM_FILTER_TYPE, // New
    C_PARAM_VCF_BASE_CUTOFF,
    C_PARAM_VCF_RESONANCE,
    C_PARAM_VCF_KEY_FOLLOW,
    C_PARAM_VCF_ENVELOPE_AMOUNT,

    C_PARAM_AMP_ENV_ATTACK,
    C_PARAM_AMP_ENV_DECAY,
    C_PARAM_AMP_ENV_SUSTAIN,
    C_PARAM_AMP_ENV_RELEASE,
    C_PARAM_FILTER_ENV_ATTACK,
    C_PARAM_FILTER_ENV_DECAY,
    C_PARAM_FILTER_ENV_SUSTAIN,
    C_PARAM_FILTER_ENV_RELEASE,

    C_PARAM_LFO_RATE,
    C_PARAM_LFO_WAVEFORM,
    C_PARAM_LFO_AMOUNT_TO_VCO1_FREQ,
    C_PARAM_LFO_AMOUNT_TO_VCO2_FREQ,
    C_PARAM_LFO_AMOUNT_TO_VCO1_PW,
    C_PARAM_LFO_AMOUNT_TO_VCO2_PW,
    C_PARAM_LFO_AMOUNT_TO_VCF_CUTOFF,

    C_PARAM_MODULATION_WHEEL_VALUE,
    C_PARAM_WHEEL_MOD_SOURCE,
    C_PARAM_WHEEL_MOD_AMOUNT_TO_FREQ_A,
    C_PARAM_WHEEL_MOD_AMOUNT_TO_FREQ_B,
    C_PARAM_WHEEL_MOD_AMOUNT_TO_PW_A,
    C_PARAM_WHEEL_MOD_AMOUNT_TO_PW_B,
    C_PARAM_WHEEL_MOD_AMOUNT_TO_FILTER,

    C_PARAM_UNISON_ENABLED,
    C_PARAM_UNISON_DETUNE_CENTS,
    C_PARAM_UNISON_STEREO_SPREAD, 

    C_PARAM_GLIDE_ENABLED,
    C_PARAM_GLIDE_TIME,

    C_PARAM_ANALOG_PITCH_DRIFT_DEPTH,
    C_PARAM_ANALOG_PW_DRIFT_DEPTH,

    C_PARAM_REVERB_ENABLED,
    C_PARAM_REVERB_DRY_WET_MIX,
    C_PARAM_REVERB_ROOM_SIZE,
    C_PARAM_REVERB_DAMPING,
    C_PARAM_REVERB_WET_GAIN,
    C_PARAM_REVERB_RT60,
    
    C_PARAM_NUM_PARAMETERS
};

// C API用 FilterType enum
typedef enum {
    PS_FILTER_TYPE_LPF24 = 0,
    PS_FILTER_TYPE_LPF12,
    PS_FILTER_TYPE_HPF12,
    PS_FILTER_TYPE_BPF12,
    PS_FILTER_TYPE_NOTCH
} PS_FilterType;


} // namespace SynthParams