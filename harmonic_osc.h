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
    // void setNoiseLevel(float level);                    // メソッドの宣言のみ (コメントアウトまたは削除)
    void resetPhase();                                     // メソッドの宣言のみ
    float getPhase() const;                                // メソッドの宣言のみ
    void setPulseWidth(float width);                       // メソッドの宣言のみ
    void setPWMDepth(float depth);                         // メソッドの宣言のみ
    void setPWMSource(float value);                        // メソッドの宣言のみ
    void setPolyModPWValue(float value);                   // メソッドの宣言のみ
    void setWheelModPWValue(float value);                  // メソッドの宣言のみ
    void setHarmonicAmplitude(int harmonicIndex, float amplitude); // New: 0-based index
    float getHarmonicAmplitude(int harmonicIndex) const;           // New: 0-based index
    void setDriftPWValue(float value);                     // メソッドの宣言のみ
    void sync();                                           // メソッドの宣言のみ

private:
    int sampleRate;
    int numHarmonics; // Will define the max number of controllable harmonics
    float baseFreq; // Frequency of the fundamental
    float phase;
    bool gateOpen;
    Waveform waveform;

    // std::vector<float> amplitudes; // 元のamplitudesは削除またはコメントアウト
    std::vector<LFO> lfos;
    std::vector<Envelope> envelopes;
    std::vector<float> harmonicAmplitudes_; // Stores amplitudes for each harmonic (index 0 = fundamental)

    std::mt19937 rng;
    std::uniform_real_distribution<float> noiseDist;

    float pulseWidth;
    float pwmDepth;
    float currentPWMSourceValue;
    float polyModPWValue;
    float wheelModPWValue;
    float driftPWValue;

    // float noiseLevel; // このメンバ変数が .cpp ファイルの setNoiseLevel で使われることを想定
                        // Voice class has its own noise generator. Removing to avoid confusion.
};