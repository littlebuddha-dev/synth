#pragma once

#include <random>
#include <vector>
#include <numeric> // for std::accumulate (not strictly needed for current version but good for related ops)
#include <cmath>   // for sqrtf (not used in current normalization but often considered)

class AnalogDrift {
public:
    AnalogDrift(int numOctaves = 5) 
        : numOctaves_(numOctaves), totalValue_(0.0f) {
        // Each instance gets its own RNG, seeded by std::random_device
        // Consider passing a seed or a generator if very rapid instantiation occurs,
        // but for per-voice members created at synth init, this is usually fine.
        rng_.seed(std::random_device{}()); 
        dist_ = std::uniform_real_distribution<float>(-1.0f, 1.0f);
        values_.resize(numOctaves_, 0.0f);
        counters_.resize(numOctaves_, 0);
    }

    float process() {
        totalValue_ = 0.0f;
        for (int i = 0; i < numOctaves_; ++i) {
            counters_[i]++;
            if (counters_[i] >= (1u << i)) { // Update rate: 1, 2, 4, 8... samples
                counters_[i] = 0;
                values_[i] = dist_(rng_);
            }
            totalValue_ += values_[i];
        }
        // Normalize sum to be roughly in [-1.0, 1.0] range
        return totalValue_ / static_cast<float>(numOctaves_); 
    }

private:
    int numOctaves_;
    std::vector<float> values_;
    std::vector<unsigned int> counters_;
    std::default_random_engine rng_; 
    std::uniform_real_distribution<float> dist_;
    float totalValue_; 
};