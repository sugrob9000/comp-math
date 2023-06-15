#pragma once

#include <imgui/imgui.h>
#include <fmt/core.h>
#include <utility>
#include <concepts>
#include <limits>
#include <cstdint>

namespace ImScoped {
// ================= Scoped wrappers for ImGui's Begin*/End* functions =================
// Usage example:
//
//   if (auto w = ImGui::ScopedWindow(...anything that goes in ImGui::Begin()...)) {
//       ... Window contents, as usual ...
//   }
//
// The pairing End* function is called automatically if needed.
// Note the `auto w = ...` Naming the object is necessary for it to live long enough

// Widget types resort to a lambda that wraps the function to properly forward arguments into
// functions with default arguments (most ImGui functions) and/or overloaded functions (some),
// and detail::Widget takes that lambda as a NTTP, meaning that none of the ImScoped:: types
// can ever cross any ABI.
// That is fine, given the usage; these types should only ever be local variables.
// (Wrapping with the lambda inside detail::Widget::Widget() rather than outside it fixes
//  the ABI issue, but only GCC compiles this, and it doesn't resolve overloaded functions.)

#define IM_SCOPED_LAMBDA_WRAP(F) \
	[] (auto&&... args) { return F(std::forward<decltype(args)>(args)...); }

namespace detail {
template <auto BeginF, auto EndF, bool AlwaysCallEndF = false> class Widget {
	bool Shown;
public:
	explicit Widget (auto&&... a): Shown{ BeginF(std::forward<decltype(a)>(a)...) } {}
	~Widget () { if (AlwaysCallEndF || Shown) EndF(); }

	explicit operator bool () const& { return Shown; }

	// Stop "if (ImScoped::Window(...))" etc. from compiling, this is wrong as it calls the destructor immediately
	// Correct usage is "if (auto w = ImScoped::Window(...))"
	explicit operator bool () && = delete;

	// Most other usage is disabled, this should only ever be a local variable in a function
	Widget (const Widget&) = delete;
	Widget (Widget&&) = delete;
	Widget& operator= (const Widget&) = delete;
	Widget& operator= (Widget&&) = delete;
};
} // namespace detail

#define IM_SCOPED_WIDGET(BEGIN, ...) \
	detail::Widget<IM_SCOPED_LAMBDA_WRAP(BEGIN), __VA_ARGS__>

using Window = IM_SCOPED_WIDGET(ImGui::Begin, ImGui::End, true);
using ChildWindow = IM_SCOPED_WIDGET(ImGui::BeginChild, ImGui::EndChild, true);
using ChildFrame = IM_SCOPED_WIDGET(ImGui::BeginChildFrame, ImGui::EndChildFrame);

using Group = IM_SCOPED_WIDGET(ImGui::BeginGroup, ImGui::EndGroup);

using Table = IM_SCOPED_WIDGET(ImGui::BeginTable, ImGui::EndTable);

using TabBar = IM_SCOPED_WIDGET(ImGui::BeginTabBar, ImGui::EndTabBar);
using TabItem = IM_SCOPED_WIDGET(ImGui::BeginTabItem, ImGui::EndTabItem);

using MainMenuBar = IM_SCOPED_WIDGET(ImGui::BeginMainMenuBar, ImGui::EndMainMenuBar);
using MenuBar = IM_SCOPED_WIDGET(ImGui::BeginMenuBar, ImGui::EndMenuBar);
using Menu = IM_SCOPED_WIDGET(ImGui::BeginMenu, ImGui::EndMenu);

using DropdownCombo = IM_SCOPED_WIDGET(ImGui::BeginCombo, ImGui::EndCombo);
using ListBox = IM_SCOPED_WIDGET(ImGui::BeginListBox, ImGui::EndListBox);
using Tooltip = IM_SCOPED_WIDGET(ImGui::BeginTooltip, ImGui::EndTooltip);

using Popup = IM_SCOPED_WIDGET(ImGui::BeginPopup, ImGui::EndPopup);
using PopupModal = IM_SCOPED_WIDGET(ImGui::BeginPopupModal, ImGui::EndPopup);

using DragDropSource = IM_SCOPED_WIDGET(ImGui::BeginDragDropSource, ImGui::EndDragDropSource);
using DragDropTarget = IM_SCOPED_WIDGET(ImGui::BeginDragDropTarget, ImGui::EndDragDropTarget);

using TreeNode = IM_SCOPED_WIDGET(ImGui::TreeNode, ImGui::TreePop);

#undef IM_SCOPED_WIDGET

namespace detail {
template <auto PushF, auto PopF> struct Push {
	explicit Push (auto&&... a) { PushF(std::forward<decltype(a)>(a)...); }
	~Push () { PopF(); }
};
} // namespace detail

using ID = detail::Push<IM_SCOPED_LAMBDA_WRAP(ImGui::PushID), ImGui::PopID>;
using Font = detail::Push<ImGui::PushFont, ImGui::PopFont>;
using TabStop = detail::Push<ImGui::PushTabStop, ImGui::PopTabStop>;
using ButtonRepeat = detail::Push<ImGui::PushButtonRepeat, ImGui::PopButtonRepeat>;
using ItemWidth = detail::Push<ImGui::PushItemWidth, ImGui::PopItemWidth>;
using TextWrapPos = detail::Push<IM_SCOPED_LAMBDA_WRAP(ImGui::PushTextWrapPos), ImGui::PopTextWrapPos>;

// TODO maybe there are more pairs where the closing function also accepts data
class Indent {
	float value;
public:
	Indent (float v = 0.0f): value(v) { ImGui::Indent(value); }
	~Indent () { ImGui::Unindent(value); }
};

// TODO generalize for PushStyleVar, get rid of duplication
class StyleColor {
	int n;
	using Key = ImGuiCol;
	using Value2 = ImU32;
	using Value1 = ImVec4;
public:
	explicit StyleColor (std::initializer_list<std::pair<Key, Value1>> colors): n(colors.size()) {
		for (const auto& [col, value]: colors)
			ImGui::PushStyleColor(col, value);
	}
	explicit StyleColor (std::initializer_list<std::pair<Key, Value2>> colors): n(colors.size()) {
		for (const auto& [col, value]: colors)
			ImGui::PushStyleColor(col, value);
	}
	template <typename Value> StyleColor (Key key, Value value): n(1) {
		ImGui::PushStyleColor(key, value);
	}
	~StyleColor () { ImGui::PopStyleColor(n); }
};

#undef IM_SCOPED_LAMBDA_WRAP
} // namespace ImScoped

namespace ImGui {
// =================================== ImGui x fmtlib ===================================
// Incomplete, but the "real" ImGui API is very redundant anyway
IMGUI_API void TextFmtV (fmt::string_view format, fmt::format_args);

template <typename Fmt, typename... Args>
void TextFmt (const Fmt& fmt, Args&&... args)
{
	TextFmtV(fmt, fmt::make_format_args(std::forward<Args&&>(args)...));
}

template <typename Fmt, typename... Args>
void TextFmtWrapped (const Fmt& fmt, Args&&... args)
{
	ImScoped::TextWrapPos wrap_pos;
	TextFmtV(fmt, fmt::make_format_args(std::forward<Args&&>(args)...));
}

template <typename Fmt, typename... Args>
void TextFmtColored (const ImVec4& col, const Fmt& fmt, Args&&... args)
{
	ImScoped::StyleColor color(ImGuiCol_Text, col);
	TextFmtV(fmt, fmt::make_format_args(std::forward<Args&&>(args)...));
}

// ===================== Get ImGuiDataType enum from an actual type =====================
template <typename> constexpr ImGuiDataType DataTypeEnum = ImGuiDataType_COUNT;
template<> constexpr inline ImGuiDataType DataTypeEnum<int8_t>   = ImGuiDataType_S8;
template<> constexpr inline ImGuiDataType DataTypeEnum<uint8_t>  = ImGuiDataType_U8;
template<> constexpr inline ImGuiDataType DataTypeEnum<int16_t>  = ImGuiDataType_S16;
template<> constexpr inline ImGuiDataType DataTypeEnum<uint16_t> = ImGuiDataType_U16;
template<> constexpr inline ImGuiDataType DataTypeEnum<int32_t>  = ImGuiDataType_S32;
template<> constexpr inline ImGuiDataType DataTypeEnum<uint32_t> = ImGuiDataType_U32;
template<> constexpr inline ImGuiDataType DataTypeEnum<int64_t>  = ImGuiDataType_S64;
template<> constexpr inline ImGuiDataType DataTypeEnum<uint64_t> = ImGuiDataType_U64;
template<> constexpr inline ImGuiDataType DataTypeEnum<float>    = ImGuiDataType_Float;
template<> constexpr inline ImGuiDataType DataTypeEnum<double>   = ImGuiDataType_Double;

template <typename T> concept ScalarType = (DataTypeEnum<T> != ImGuiDataType_COUNT);

// ==================== Wrap type-specific functions into templates ====================

template <ScalarType T>
bool Drag (const char* l, T* p, float speed, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags flags = 0)
{
	return DragScalar(l, DataTypeEnum<T>, p, speed, &min, &max, fmt, flags);
}

template <ScalarType T>
bool DragN (const char* l, T* p, int n, float speed, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags flags = 0)
{
	return DragScalarN(l, DataTypeEnum<T>, p, n, speed, &min, &max, fmt, flags);
}

template <ScalarType T>
bool Slider (const char* l, T* p, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags flags = 0)
{
	return SliderScalar(l, DataTypeEnum<T>, p, &min, &max, fmt, flags);
}

template <ScalarType T>
bool SliderN (const char* l, T* p, int n, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags flags = 0)
{
	return SliderScalar(l, DataTypeEnum<T>, p, n, &min, &max, fmt, flags);
}

template <ScalarType T>
bool InputNumber (const char* l, T* p, T step = 0, T step_fast = 0, const char* fmt = nullptr, ImGuiInputTextFlags flags = 0)
{
	return InputScalar(l, DataTypeEnum<T>, p, (step == 0 ? nullptr : &step), (step_fast == 0 ? nullptr : &step_fast), fmt, flags);
}

template <ScalarType T>
bool InputNumberN (const char* l, T* p, int n, T step = 0, T step_fast = 0, const char* fmt = nullptr, ImGuiInputTextFlags flags = 0)
{
	return InputScalarN(l, DataTypeEnum<T>, p, n, (step == 0 ? nullptr : &step), (step_fast == 0 ? nullptr : &step_fast), fmt, flags);
}


// Drag two values such that `low <= high - min_diff`
template <ScalarType T>
bool DragMinMax (const char* id, T* low, T* high, float speed, T min_diff = 0, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags = 0)
{
	bool result = false;
	ImScoped::ID id_guard(id);
	ImScoped::ItemWidth width_guard(CalcItemWidth() * 0.5);
	result |= Drag("##l", low, speed, min, *high - min_diff, fmt);
	SameLine();
	result |= Drag("##h", high, speed, *low + min_diff, max, fmt);
	return result;
}

// ======================================================================================
// Generating a unique ImGui ID string from some (small) integers. Example:
//
//  for (int i = 0; i < 10; i++) {                       for (int i = 0; i < 10; i++) {
//    const char id[] = { '#', '#', char(1+i) };   ==>     InputText(GenerateId(i), ...);
//    InputText(id, ...);                                }
//  }

template <size_t Length> class GenerateId {
	char s[2+Length+1]; // "##[...]\0"
public:
	template <std::integral... Ts> constexpr explicit GenerateId (Ts... ts) {
		static_assert(Length == sizeof...(Ts));
		s[0] = s[1] = '#';
		int i = 2;
		((s[i++] = char(1 + size_t(ts) % std::numeric_limits<char>::max())), ...);
		s[i] = '\0';
	}
	constexpr operator const char* () const { return s; }
};
template <std::integral... Ts> GenerateId(Ts...) -> GenerateId<sizeof...(Ts)>;

} // namespace ImGui