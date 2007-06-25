#include <cmath>

#include "normaliseeffect.hpp"


NormaliseEffect::NormaliseEffect()
  : Effect("Normalise") {
  
}
  

void NormaliseEffect::process(size_t nframes, float* input, float* output) {
  float max = std::abs(input[0]);
  for (size_t i = 1; i < nframes; ++i)
    max = std::abs(input[i]) > max ? std::abs(input[i]) : max;
  float c = 1 / max;
  for (size_t i = 0; i < nframes; ++i)
    output[i] = c * input[i];
}
