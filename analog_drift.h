#pragma once

#include <random>
#include <vector>
#include <numeric> 
#include <cmath>   

class AnalogDrift {
public:
    AnalogDrift(int numOctaves = 5) 
        : numOctaves_(numOctaves), totalValue_(0.0f) {
        rng_.seed(std::random_device{}()); 
        dist_ = std::uniform_real_distribution<float>(-1.0f, 1.0f);
        values_.resize(numOctaves_, 0.0f);
        counters_.resize(numOctaves_, 0);
    }

    float process() {
        totalValue_ = 0.0f;
        for (int i = 0; i < numOctaves_; ++i) {
            counters_[i]++;
            if (counters_[i] >= (1u << i)) { 
                counters_[i] = 0;
                values_[i] = dist_(rng_);
            }
            totalValue_ += values_[i];
        }
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