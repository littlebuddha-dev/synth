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
          releaseTime(release), sampleRate(sampleRate),
          state(State::Idle), level(0.0f), counter(0) {}

    void noteOn() {
        state = State::Attack;
        counter = 0;
        // level = 0.0f; // Reset level on note on if it wasn't already idle
    }

    void noteOff() {
        // If sustain is 0, or very low, release might start from a low level.
        // Prophet-5 behaviour: Release starts from current level at note-off.
        // If sustain level is 0, decay goes to 0 then release starts from 0.
        // If note is released during attack/decay, release starts from current level.
        state = State::Release;
        counter = 0;
        initialReleaseLevel = level; // Capture level at start of release
    }

    float step() {
        // Prevent division by zero if times are 0
        float safeAttackTime = (attackTime <= 0.0f) ? 0.001f : attackTime;
        float safeDecayTime = (decayTime <= 0.0f) ? 0.001f : decayTime;
        float safeReleaseTime = (releaseTime <= 0.0f) ? 0.001f : releaseTime;

        float sr = static_cast<float>(sampleRate);

        switch (state) {
            case State::Idle:
                // level remains 0.0f
                break;
            case State::Attack:
                level += 1.0f / (safeAttackTime * sr);
                if (level >= 1.0f) {
                    level = 1.0f;
                    state = State::Decay;
                }
                break;
            case State::Decay:
                // Decay towards sustainLevel
                if (sustainLevel < 1.0f) { // Only decay if sustain is not full
                    level -= (1.0f - sustainLevel) / (safeDecayTime * sr);
                    if (level <= sustainLevel) {
                        level = sustainLevel;
                        state = State::Sustain;
                    }
                } else { // Sustain is 1.0, so effectively no decay phase
                    level = 1.0f;
                    state = State::Sustain;
                }
                break;
            case State::Sustain:
                // level remains sustainLevel
                // If noteOff happens, state changes to Release
                if (level < sustainLevel && decayTime > 0) { // If retriggered during release below sustain
                    state = State::Decay; // Go back to decay
                } else {
                    level = sustainLevel; // Ensure it stays at sustain
                }
                break;
            case State::Release:
                // Release from current level towards 0
                // The original code released sustainLevel / releaseTime, which is only correct if release starts from sustain.
                // It should release 'level' over 'releaseTime'.
                // However, if sustainLevel is very low, this can be slow.
                // A common way: releaseTime defines time to go from sustainLevel to 0.
                // If current level is different, adjust rate.
                // For simplicity and Prophet-like behavior, release from current level to 0 over releaseTime.
                // The amount to subtract per sample should be based on the level *at the start of release*.
                // This simple model subtracts based on sustainLevel, which isn't quite right if released during attack/decay.
                // Corrected: level decays from its current value to 0 over releaseTime.
                if (level > 0.0f) { // Only release if level is above 0
                     // The rate of decay in release should allow the current level to reach zero in releaseTime.
                     // If releaseTime is the time to go from 1.0 to 0.0, then:
                     // decrement = 1.0f / (safeReleaseTime * sr)
                     // If releaseTime is time to go from sustainLevel to 0.0 (common for some synths):
                     // decrement = sustainLevel / (safeReleaseTime * sr) - This was the old way for this specific var
                     // Let's assume releaseTime is time to go from current level to 0 (or 1.0 to 0, scaled)
                     // For simplicity, if sustainLevel is 0, release very quickly.
                    float releaseDecrement;
                    // initialReleaseLevel is now set in noteOff()
                    if (initialReleaseLevel > 0.0001f) { // Avoid division by zero if initialReleaseLevel is tiny
                        releaseDecrement = initialReleaseLevel / (safeReleaseTime * sr);
                    } else { // Fallback if initialReleaseLevel wasn't set (e.g. noteOff called on idle envelope) or is 0
                         releaseDecrement = level / (safeReleaseTime * sr); 
                         if (safeReleaseTime * sr == 0) releaseDecrement = level; // Instant if time is 0
                    }
                     if (releaseDecrement <= 0 && level > 0) releaseDecrement = level / (0.001f * sr); // Ensure some decay


                    level -= releaseDecrement;
                }

                if (level <= 0.0f) {
                    level = 0.0f;
                    state = State::Idle;
                    initialReleaseLevel = 0.0f; // Reset for next release
                }
                break;
        }
        counter++;
        return level;
    }

    bool isActive() const { return state != State::Idle; }
    float getCurrentLevel() const { return level; } // Added this method

private:
    enum class State { Idle, Attack, Decay, Sustain, Release };
    State state;

    float attackTime, decayTime, sustainLevel, releaseTime;
    int sampleRate;
    float level;
    int counter; // General purpose counter, could track samples in current state
    float initialReleaseLevel = 0.0f; // Stores the level when release phase begins
};