// synth/harmonic_osc.h
#pragma once
#include "envelope.h" // For Envelope
#include "lfo.h"      // For LFO full definition
#include "waveform.h" // For Waveform enum
#include <vector>
#include <random>
#include <cmath>
#include <algorithm> // For std::clamp

class HarmonicOscillator { // ← クラスの宣言が最初に来る
public:
    HarmonicOscillator(int sampleRate, int numHarmonics); // メソッドの宣言のみ
    void setFrequency(float freq);                         // メソッドの宣言のみ
    float getBaseFrequency() const;                        // メソッドの宣言のみ
    void noteOn();                                         // メソッドの宣言のみ
    void noteOff();                                        // メソッドの宣言のみ
    float process();                                       // メソッドの宣言のみ
    bool isRunning() const;                                // メソッドの宣言のみ
    bool isGateOpen() const;                               // メソッドの宣言のみ

    void setWaveform(Waveform wf);                         // メソッドの宣言のみ
    Waveform getWaveform() const;                          // メソッドの宣言のみ
    void setNoiseLevel(float level);                       // メソッドの宣言のみ
    void resetPhase();                                     // メソッドの宣言のみ
    float getPhase() const;                                // メソッドの宣言のみ
    void setPulseWidth(float width);                       // メソッドの宣言のみ
    void setPWMDepth(float depth);                         // メソッドの宣言のみ
    void setPWMSource(float value);                        // メソッドの宣言のみ
    void setPolyModPWValue(float value);                   // メソッドの宣言のみ
    void setWheelModPWValue(float value);                  // メソッドの宣言のみ
    void setDriftPWValue(float value);                     // メソッドの宣言のみ
    void sync();                                           // メソッドの宣言のみ

private:
    int sampleRate;
    int numHarmonics;
    float baseFreq;
    float phase;
    bool gateOpen;
    Waveform waveform;

    std::vector<float> amplitudes;
    std::vector<LFO> lfos;
    std::vector<Envelope> envelopes;

    float noiseLevel; // このメンバ変数が .cpp ファイルの setNoiseLevel で使われることを想定
    std::mt19937 rng;
    std::uniform_real_distribution<float> noiseDist;

    float pulseWidth;
    float pwmDepth;
    float currentPWMSourceValue;
    float polyModPWValue;
    float wheelModPWValue;
    float driftPWValue;
};