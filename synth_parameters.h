#pragma once

// このファイルは、C APIからもインクルードされることを想定し、
// C++特有の機能に依存しないように注意するか、#ifdef __cplusplus で囲む。
// ここではシンプルにするため、C++のenum classを使用し、
// C API側では通常のenumにマッピングすることを想定します。

namespace SynthParams {

enum class ParamID {
    // Global Voice Parameters
    MasterTuneCents,
    Waveform, // For both OSCs
    Osc1Level,
    Osc2Level,
    NoiseLevel,
    RingModLevel, // New
    VCOBDetuneCents,
    SyncEnabled,
    VCOBLowFreqEnabled,
    VCOBFreqKnob,
    FilterEnvVelocitySensitivity,
    AmpVelocitySensitivity,
    PulseWidth, // Base PW for both OSCs
    PWMDepth,   // General PWM depth for both OSCs

    // PolyMod Parameters
    PMFilterEnvToFreqAAmount,
    PMFilterEnvToPWAAmount,
    PMFilterEnvToFilterCutoffAmount,
    PMOscBToFreqAAmount,
    PMOscBToPWAAmount,
    PMOscBToFilterCutoffAmount,

    // Filter Parameters
    VCFBaseCutoff,
    VCFResonance,
    VCFKeyFollow,
    VCFEnvelopeAmount,

    // Envelope Parameters (Attack, Decay, Sustain, Release per envelope)
    AmpEnvAttack,
    AmpEnvDecay,
    AmpEnvSustain,
    AmpEnvRelease,
    FilterEnvAttack,
    FilterEnvDecay,
    FilterEnvSustain,
    FilterEnvRelease,

    // LFO Parameters
    LfoRate,
    LfoWaveform,
    LfoAmountToVco1Freq,
    LfoAmountToVco2Freq,
    LfoAmountToVco1Pw,
    LfoAmountToVco2Pw,
    LfoAmountToVcfCutoff,

    // Wheel Modulation Settings
    ModulationWheelValue, // This is an input, but can be represented
    WheelModSource,
    WheelModAmountToFreqA,
    WheelModAmountToFreqB,
    WheelModAmountToPWA,
    WheelModAmountToPWB,
    WheelModAmountToFilter,

    // Unison Settings
    UnisonEnabled,
    UnisonDetuneCents,
    UnisonStereoSpread, // New

    // Glide/Portamento Settings
    GlideEnabled,
    GlideTime,

    // Analog Drift
    AnalogPitchDriftDepth,
    AnalogPWDriftDepth,

    // Effects (Example: Reverb - assuming one main reverb instance)
    ReverbEnabled,
    ReverbDryWetMix,
    ReverbRoomSize,
    ReverbDamping,
    ReverbWetGain,
    ReverbRT60,

    // --- Add more parameters as needed ---
    NumParameters // Keep last for counting if necessary
};

// パラメータの型に関する情報や範囲などもここに定義できると便利
// struct ParameterInfo {
//     ParamID id;
//     const char* name;
//     enum class Type { Float, Int, Bool, Enum } type;
//     float minValue, maxValue, defaultValue;
//     // std::vector<const char*> enumNames; // for Enum type
// };
// extern const std::vector<ParameterInfo> PARAMETER_INFO_LIST;


// C API 用のプレーンな enum (例)
// C API を作る際に、上記 ParamID とマッピングする
// C_ParamIDはSynthParams名前空間の外にある方が良いかもしれないが、今回はこのまま
enum C_ParamID {
    C_PARAM_MASTER_TUNE_CENTS = 0,
    C_PARAM_WAVEFORM,
    C_PARAM_OSC1_LEVEL,
    C_PARAM_OSC2_LEVEL,
    C_PARAM_NOISE_LEVEL,
    C_PARAM_RING_MOD_LEVEL, // New
    C_PARAM_VCOB_DETUNE_CENTS,
    C_PARAM_SYNC_ENABLED,
    C_PARAM_VCOB_LOW_FREQ_ENABLED,
    C_PARAM_VCOB_FREQ_KNOB,
    C_PARAM_FILTER_ENV_VELOCITY_SENSITIVITY,
    C_PARAM_AMP_VELOCITY_SENSITIVITY,
    C_PARAM_PULSE_WIDTH,
    C_PARAM_PWM_DEPTH,

    C_PARAM_PM_FILTER_ENV_TO_FREQ_A_AMOUNT,
    C_PARAM_PM_FILTER_ENV_TO_PW_A_AMOUNT,
    C_PARAM_PM_FILTER_ENV_TO_FILTER_CUTOFF_AMOUNT,
    C_PARAM_PM_OSCB_TO_FREQ_A_AMOUNT,
    C_PARAM_PM_OSCB_TO_PW_A_AMOUNT,
    C_PARAM_PM_OSCB_TO_FILTER_CUTOFF_AMOUNT,

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
    C_PARAM_UNISON_STEREO_SPREAD, // New

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


} // namespace SynthParams