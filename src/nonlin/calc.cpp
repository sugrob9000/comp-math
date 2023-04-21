#include "nonlin/calc.hpp"
#include "util/util.hpp"
#include <cassert>

namespace math {

std::vector<Chord> build_chords (double (*f) (double), double low, double high, double precision)
{
	assert(high > low);
	assert(precision > 0);

	double f_low = f(low);
	double f_high = f(high);

	if ((f_low >= 0) == (f_high >= 0)) return {};
	std::vector<Chord> result;

	const auto push_chord = [&] { result.push_back({{ low, f_low }, { high, f_high }}); };
	constexpr size_t max_iterations = 10000;

	push_chord();
	for (size_t i = 0; i < max_iterations && high - low >= precision; i++) {
		const double mid = low + f_low * (high - low) / (f_low - f_high);
		const double f_mid = f(mid);
		if ((f_mid >= 0) == (f_high >= 0)) {
			high = mid;
			f_high = f_mid;
		} else {
			low = mid;
			f_low = f_mid;
		}
		push_chord();
	}

	return result;
}

} // namespace math