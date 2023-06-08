#include <glm/gtx/norm.hpp>
#include <nonlin-system/calc.hpp>
#include <util/util.hpp>

math::Newton_system_result math::newtons_method_system
(double (*f) (dvec2), double (*dfdx) (dvec2), double (*dfdy) (dvec2),
 double (*g) (dvec2), double (*dgdx) (dvec2), double (*dgdy) (dvec2),
 dvec2 guess, double precision)
{
	Newton_system_result result;
	constexpr int max_iterations = 200;
	const double precision2 = precision * precision;

	for (int i = 0; i < max_iterations; i++) {
		result.guesses.push_back(guess);

		// ( A  B ) ( x )   ( C )
		// ( D  E ) ( y ) = ( F )
		// ...
		// ( A        B       ) ( x )   ( C )
		// ( D-A*D/A  E-B*D/A ) ( y ) = ( F )
		// ...
		// ( A   B       ) ( x )   ( C         )
		// ( 0   E-B*D/A ) ( y ) = ( F - C*D/A )
		// ...
		// y = (F - C * D / A) / (E - B * D / A)
		// x = (C - B*y)/A
		const double A = dfdx(guess);
		const double B = dfdy(guess);
		const double C = -f(guess);
		const double D = dgdx(guess);
		const double E = dgdy(guess);
		const double F = -g(guess);
		const double DoverA = D / A;

		dvec2 delta;
		delta.y = (F - C * DoverA) / (E - B * DoverA);
		delta.x = (C - B * delta.y) / A;

		guess += delta;

		if (glm::length2(delta) < precision2) {
			result.root = guess;
			break;
		}
	}

	return result;
}