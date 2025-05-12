// synth/envelope.h
#pragma once

struct EnvelopeParams {
    float attack;
    float decay;
    float sustain;
    float release;
};

class Envelope {
public:
    Envelope(float attack = 0.01f, float decay = 0.1f,
             float sustain = 0.7f, float release = 0.2f,
             int sampleRate = 44100)
        : attackTime(attack), decayTime(decay), sustainLevel(sustain),
          releaseTime(release), sampleRate_(sampleRate), // Renamed for clarity
          state(State::Idle), level(0.0f), counter(0), initialReleaseLevel(0.0f) {}

    void noteOn() {
        state = State::Attack;
        counter = 0;
    }

    void noteOff() {
        state = State::Release;
        counter = 0;
        initialReleaseLevel = level; 
    }

    float step() {
        float safeAttackTime = (attackTime <= 0.0f) ? 0.0001f : attackTime; // Avoid 0, use smaller epsilon
        float safeDecayTime = (decayTime <= 0.0f) ? 0.0001f : decayTime;
        float safeReleaseTime = (releaseTime <= 0.0f) ? 0.0001f : releaseTime;

        float sr_float = static_cast<float>(sampleRate_);

        switch (state) {
            case State::Idle:
                break;
            case State::Attack:
                level += 1.0f / (safeAttackTime * sr_float);
                if (level >= 1.0f) {
                    level = 1.0f;
                    state = State::Decay;
                }
                break;
            case State::Decay:
                if (sustainLevel < 1.0f) { 
                    level -= (1.0f - sustainLevel) / (safeDecayTime * sr_float);
                    if (level <= sustainLevel) {
                        level = sustainLevel;
                        state = State::Sustain;
                    }
                } else { 
                    level = 1.0f; // Sustain is at max, so no decay phase effectively
                    state = State::Sustain;
                }
                break;
            case State::Sustain:
                // Level remains sustainLevel. If noteOff happens, state changes to Release.
                // If retriggered during release below sustain (not handled here explicitly, but good to note)
                // it might jump back to Attack or Decay. Current logic is simpler.
                level = sustainLevel; // Ensure it stays at sustain
                break;
            case State::Release:
                if (level > 0.0f) { 
                    float releaseDecrement;
                    if (initialReleaseLevel > 0.00001f) { // Use smaller epsilon
                        releaseDecrement = initialReleaseLevel / (safeReleaseTime * sr_float);
                    } else { 
                         releaseDecrement = level / (safeReleaseTime * sr_float); 
                         if (safeReleaseTime * sr_float < 1e-6) releaseDecrement = level; // Instant if time is effectively 0
                    }
                    // Ensure some decay if calculated decrement is zero or negative but level is positive
                    if (releaseDecrement <= 1e-9f && level > 1e-9f) {
                         releaseDecrement = level / (0.001f * sr_float); // Fallback to a small decay
                    }


                    level -= releaseDecrement;
                }

                if (level <= 0.0f) {
                    level = 0.0f;
                    state = State::Idle;
                    initialReleaseLevel = 0.0f; 
                }
                break;
        }
        counter++; // General purpose counter, mostly for debugging or future use
        return level;
    }

    bool isActive() const { return state != State::Idle; }
    float getCurrentLevel() const { return level; } 

private:
    enum class State { Idle, Attack, Decay, Sustain, Release };
    State state;

    float attackTime, decayTime, sustainLevel, releaseTime;
    int sampleRate_; // Renamed to avoid potential conflict
    float level;
    int counter; 
    float initialReleaseLevel; 
};