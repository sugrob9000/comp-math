#include <gui.hpp>
#include <imhelper.hpp>
#include <interp/calc.hpp>
#include <interp/it-gui.hpp>
#include <util/util.hpp>

using namespace ImGui;
using namespace ImScoped;

constexpr float drag_speed = 0.03;

void Interp::gui_frame ()
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Параметры", nullptr, gui::floating_window_flags))
		settings_widget();
	output.result_window();

	Graph_draw_context draw(graph);
	draw.background();

	constexpr uint32_t point_color = 0xFF'0000FF;
	constexpr uint32_t function_color = 0xFF'996633;

	const auto& f = output.function;

	switch (method) {
	case Method::lagrange:
		for (const dvec2& p: input.points.view())
			draw.dot(p, point_color);
		draw.function_plot(function_color, f);
		break;
	case Method::newton: {
		const auto& es = input.evenly_spaced;
		double x = es.low;
		const double step = es.get_step();
		for (double y: es.values) {
			draw.dot({ x, y }, point_color);
			x += step;
		}
		draw.function_plot(function_color, f, es.low, es.high);
		break;
	}
	}

	draw.vert_line(approx_x, 0xFF'6666FF, 2.0);
	draw.dot({ approx_x, f(approx_x) }, 0xFF'44CC44);
}

void Interp::settings_widget ()
{
	bool dirty = false;
	if (auto node = ImScoped::TreeNode("Метод")) {
		using enum Method;
		struct Method_spec {
			const char* name;
			Method method;
		};
		constexpr static Method_spec methods[] = {
			{ "Полином Лагранжа", lagrange },
			{ "Полином Ньютона с конечными разностями", newton },
		};
		for (auto [name, m]: methods) {
			if (RadioButton(name, method == m)) {
				method = m;
				dirty = true;
			}
		}
	}

	dirty |= Drag("X", &approx_x, drag_speed);
	if (auto node = ImScoped::TreeNode("Данные")) {
		switch (method) {
		case Method::lagrange: dirty |= input.points.widget(); break;
		case Method::newton: dirty |= input.evenly_spaced.widget(); break;
			break;
		}
	}
	if (auto node = ImScoped::TreeNode("Вид")) graph.settings_widget();

	if (dirty) update_calculation();
}

void Interp::Output::result_window () const
{
	SetNextWindowSizeConstraints({ 300, -1 }, { 1000, -1 });
	if (auto w = Window("Результат", nullptr, gui::floating_window_flags)) {
		TextFmt("Значение F({}) = {}", approx_x, approx_value);
		if (method == Method::newton) {
			TextUnformatted("Конечные разности");
			if (auto table = Table("fd", diff.diff.size())) {
				for (const auto& v: diff.diff) {
					TableNextRow();
					for (const auto& d: v) {
						TableNextColumn();
						TextFmt("{:.3}", d);
					}
				}
			}
		}
	}
}


namespace {
struct Function_spec {
	const char* name;
	double (*compute) (double);
};
constexpr Function_spec functions[] = {
	{ "exp(-x²) - 0.5", [] (double x) { return exp(-x*x) - 0.5; }, },
	{ "x² - 0.9", [] (double x) { return x*x-0.9; }, },
	{ "sin(x) exp(x)", [] (double x) { return sin(x) * exp(x); }, },
	{ "1/x", [] (double x) { return 1.0 / x; }, }
};
} // anon namespace


void Interp::update_calculation ()
{
	using enum Method;

	output.method = method;

	switch (method) {
	case lagrange:
		output.diff = {};
		output.function = math::approx_lagrange(input.points.view());
		break;
	case newton: {
		auto& es = input.evenly_spaced;
		if (es.input_method == Input::Evenly_spaced::Input_method::sample_function) {
			const double step = es.get_step();
			double x = es.low;

			for (double& v: es.values) {
				v = functions[es.sampled_function_id].compute(x);
				x += step;
			}
		}
		output.diff = math::newton_calc_finite_differences(es.values);
		output.function = math::approx_newton(es.low, es.high, output.diff);
		break;
	}
	}

	output.approx_x = approx_x;
	output.approx_value = output.function(approx_x);
}

bool Interp::Input::Evenly_spaced::widget ()
{
	constexpr double min_width = 0.2;
	bool dirty = false;

	TextUnformatted("Интервал значений");
	dirty |= DragMinMax("even", &low, &high, drag_speed, min_width);

	if (size_t size = values.size();
			Drag("Количество точек", &size, drag_speed, size_t{2}, size_t{150})) {
		values.resize(size);
		dirty = true;
	}
	TextFmt("Шаг: {:.3}", get_step());

	const auto input_method_select = [&] (const char* name, Input_method im) {
		if (RadioButton(name, input_method == im)) {
			dirty = true;
			input_method = im;
		}
	};
	input_method_select("Ввести значения вручную", Input_method::values);
	input_method_select("Взять значения функции", Input_method::sample_function);

	switch (input_method) {
	case Input_method::values: {
		double x = low;
		const double step = get_step();
		for (size_t i = 0; i < values.size(); i++) {
			TextFmt("x = {:8.4}", x);
			SameLine();
			dirty |= Drag(ImGui::GenerateId(i), &values[i], drag_speed);
			x += step;
		}
		break;
	}
	case Input_method::sample_function: {
		ImScoped::Indent indent_guard;
		for (unsigned id = 0; id < std::size(functions); id++) {
			if (RadioButton(functions[id].name, sampled_function_id == id)) {
				sampled_function_id = id;
				dirty = true;
			}
		}
		break;
	}
	}

	return dirty;
}