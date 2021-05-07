#include <cmath>

namespace bomaqs {

float to_fixed(float value, int places) {
  const float multiplier = std::pow(10.0, places);
  return std::round(value * multiplier) / multiplier;
}

}  // namespace bomaqs
