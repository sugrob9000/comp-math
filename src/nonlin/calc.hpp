#pragma once

#include "math.hpp"
#include <vector>

namespace math {

struct Chord { Dvec2 a, b; };

std::vector<Chord> build_chords (double (*f) (double), double low, double high, double precision);

}