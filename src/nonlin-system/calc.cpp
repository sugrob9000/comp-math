#include "glm/gtx/norm.hpp"
#include "nonlin-system/calc.hpp"
#include "util/util.hpp"

namespace math {
Newton_system_result newtons_method_system
(double (*f) (double), double (*dfdx) (double),
 double (*g) (double), double (*dgdx) (double),
 dvec2 guess, double precision)
{
	Newton_system_result result;

	constexpr int max_iterations = 200;
	const double precision2 = precision * precision;

	for (int i = 0; i < max_iterations; i++) {
		result.guesses.push_back(guess);
		// ( A  -1 ) ( x )    ( B )
		// ( C  -1 ) ( y ) =  ( D )
		// Ax - y = B,  Cx - y = D
		// (A-C)x = B-D,  x = (B-D)/(A-C)
		// y = Ax - B,  y = Cx - D

		double A = dfdx(guess.x);
		double B = guess.y - f(guess.x);
		double C = dgdx(guess.x);
		double D = guess.y - g(guess.x);

		dvec2 delta;
		delta.x = (B-D) / (A-C);
		delta.y = ((A*delta.x - B) + (C*delta.x - D))*0.5;

		guess += delta;

		if (glm::length2(delta) < precision2) {
			result.root = guess;
			break;
		}
	}

	return result;
}
} // namespace math