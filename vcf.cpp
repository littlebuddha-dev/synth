// synth/vcf.cpp
#include "vcf.h"
#include <cmath>
#include <algorithm> // For std::clamp

VCF::VCF(float sr)
    : sampleRate(sr), baseCutoffHz(1000.0f), resonance(0.0f), keyFollow(0.0f),
      envModAmount(0.0f), envelopeValue(0.0f), noteBaseFreq(440.0f) {
    std::fill(std::begin(z), std::end(z), 0.0f);
}

void VCF::setBaseCutoff(float hz) {
    baseCutoffHz = std::clamp(hz, 20.0f, 20000.0f);
}

void VCF::setResonance(float q) {
    resonance = std::clamp(q, 0.0f, 0.98f); // Slightly less than 1 to prevent extreme self-oscillation instability in this model
}

void VCF::setKeyFollow(float factor) {
    keyFollow = std::clamp(factor, 0.0f, 1.0f);
}

void VCF::setEnvelopeMod(float amount) {
    envModAmount = std::clamp(amount, -1.0f, 1.0f);
}

void VCF::setNote(int midiNote) {
    // Reference for key follow is typically A4=440Hz (MIDI note 69)
    // Or C4=261.63Hz (MIDI note 60) depending on convention
    // Using A4=69
    noteBaseFreq = 440.0f * std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);
}

void VCF::setEnvelopeValue(float env) {
    envelopeValue = std::clamp(env, 0.0f, 1.0f); // Envelope output is usually 0-1
}

float VCF::process(float input, float directModHz) {
    // 1. Calculate cutoff based on base, key follow, envelope mod, and direct LFO/PolyMod
    float effectiveCutoff = baseCutoffHz;

    // Apply key follow: 1.0 = 100% key follow (cutoff moves with note pitch)
    // Relative to a reference, e.g. A4 (440Hz) or Middle C. Assume reference for baseCutoff is A4.
    // If noteBaseFreq is higher than ref, cutoff increases. If lower, cutoff decreases.
    // A common reference for keytracking might be that at note 69 (A4), keytrack has no effect if baseCutoff is set for A4.
    // Or, simpler: keyfollow directly scales the note frequency relationship.
    // This formula means if keyFollow = 1, cutoff doubles per octave.
    effectiveCutoff *= std::pow(2.0f, keyFollow * (std::log2(noteBaseFreq / 440.0f)) );


    // Apply envelope modulation
    // Envelope value is 0 to 1. Bipolar envModAmount.
    // A common behavior: envMod scales how many octaves the envelope sweeps.
    // E.g. envMod = 1.0, envValue = 1.0 -> sweep up X octaves. envMod = -1.0 -> sweep down.
    // Let's say envMod of 1.0 means full envelope (1.0) sweeps cutoff by 5 octaves (example)
    float envSweepOctaves = 5.0f;
    effectiveCutoff *= std::pow(2.0f, envModAmount * (envelopeValue - 0.5f) * 2.0f * envSweepOctaves); // Centered around 0.5 for bipolar feel


    // Apply direct modulation (LFO, PolyMod)
    effectiveCutoff += directModHz;

    effectiveCutoff = std::clamp(effectiveCutoff, 20.0f, sampleRate * 0.49f); // Clamp to avoid issues

    // Filter algorithm (4-pole ladder filter approximation)
    // Calculation of 'f' (normalized cutoff frequency for the filter algorithm)
    float f = 2.0f * std::sinf(M_PI * effectiveCutoff / sampleRate);
    f = std::clamp(f, 0.0f, 1.0f); // Ensure f is in a safe range for stability

    // Feedback amount 'fb' based on resonance.
    // Resonance here is 0 to ~1. Higher q gives more resonance.
    // Max fb needs to be < 4.0 for stability in this simple model.
    float fb = resonance * (4.0f * 0.95f); // Max resonance just under instability point. Adjust 0.95f as needed.
    fb = std::clamp(fb, 0.0f, 3.95f);


    // Filter processing stages
    float lowPass = input - z[3] * fb; // Input with feedback
    lowPass = std::clamp(lowPass, -10.0f, 10.0f); // Prevent extreme values from blowing up filter

    z[0] = z[0] + f * (lowPass - z[0]);
    z[1] = z[1] + f * (z[0] - z[1]);
    z[2] = z[2] + f * (z[1] - z[2]);
    z[3] = z[3] + f * (z[2] - z[3]);

    return z[3]; // Output of the 4th pole
}