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
		const auto& es = input.evenly_spaced;
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

	size_t size = values.size();
	Drag("Количество точек", &size, drag_speed, size_t{2}, size_t{150});
	TextFmt("Шаг: {:.3}", get_step());

	if (size != values.size()) {
		values.resize(size);
		dirty = true;
	}

	TextUnformatted("Значения");
	double x = low;
	const double step = get_step();
	for (size_t i = 0; i < values.size(); i++) {
		TextFmt("x = {:8.4}", x);
		SameLine();
		dirty |= Drag(ImGui::GenerateId(i), &values[i], drag_speed);
		x += step;
	}

	return dirty;
}