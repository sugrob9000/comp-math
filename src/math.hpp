#pragma once

#include <glm/vec2.hpp>
#include <fmt/format.h>

namespace math {
using Dvec2 = glm::vec<2, double, glm::packed>;
using Fvec2 = glm::vec<2, float, glm::packed>;
}

template <int N, typename S, glm::qualifier Q>
class fmt::formatter<glm::vec<N,S,Q>>: public formatter<S> {
	constexpr static auto opening = FMT_STRING("(");
	constexpr static auto joiner = FMT_STRING(", ");
	constexpr static auto closing = FMT_STRING(")");
public:
	template <typename Context>
	auto format (const glm::vec<N,S,Q>& v, Context& ctx) const {
		format_to(ctx.out(), opening);
		for (int i = 0; i+1 < N; i++) {
			formatter<S>::format(v[i], ctx);
			format_to(ctx.out(), joiner);
		}
		formatter<S>::format(v[N-1], ctx);
		return format_to(ctx.out(), closing);
	}
};