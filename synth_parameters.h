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
enum C_ParamID {
    C_PARAM_MASTER_TUNE_CENTS = 0,
    C_PARAM_WAVEFORM,
    // ...
};


} // namespace SynthParams