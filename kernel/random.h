#pragma once

#include <kernel/common.h>

namespace kernel {

class RandomSource {
public:
    virtual ~RandomSource() = default;

    virtual u32 next() = 0;
    virtual void seed(u32 seed) = 0;
};

class RDRandSource : public RandomSource {
public:
    u32 next() override;
    void seed(u32 seed) override;
};

class RDSeedSource : public RandomSource {
public:
    u32 next() override;
    void seed(u32 seed) override;
};

class Random {
public:
    static u32 next();
    static void seed(u32 seed);

};
    
}