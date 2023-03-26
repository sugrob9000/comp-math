#ifndef IMCPP20_H
#define IMCPP20_H

#include <imgui/imgui.h>
#include <fmt/core.h>
#include <utility>
#include <concepts>
#include <limits>

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
#define IM_SCOPED_LAMBDA_WRAP(F) [](auto&&... wrap_arg){ return F(std::forward<decltype(wrap_arg)>(wrap_arg)...); }

namespace detail {
template <auto BeginF, auto EndF, bool AlwaysCallEndF = false> class Widget {
	bool Shown;
public:
	explicit Widget (auto&&... a): Shown{ IM_SCOPED_LAMBDA_WRAP(BeginF)(std::forward<decltype(a)>(a)...) } {}
	~Widget () { if (AlwaysCallEndF || Shown) EndF(); }

	explicit operator bool () const& { return Shown; }

	// Stop "if (ImGui::Window(...))" etc. from compiling, this is wrong as it calls the destructor immediately
	explicit operator bool () && = delete;

	// Most other usage should be prohibited, as the resulting program will be wrong and might not even link
	Widget (const Widget&) = delete;
	Widget (Widget&&) = delete;
	Widget& operator= (const Widget&) = delete;
	Widget& operator= (Widget&&) = delete;
};
} // namespace detail

using Window = detail::Widget<ImGui::Begin, ImGui::End, true>;

// Because ImGui::BeginChild is, besides having defaulted arguments, overloaded, we can't pass it as `BeginF`,
// so wrap it in a lambda here too. (Wrapping once is enough, but then we'd need a way to tell the constructor to not do it)
// Using a lambda in a type name means we can never pass the object across any ABI boundary ever, but that is OK given the use
using ChildWindow = detail::Widget<IM_SCOPED_LAMBDA_WRAP(ImGui::BeginChild), ImGui::EndChild, true>;
using ChildFrame = detail::Widget<ImGui::BeginChildFrame, ImGui::EndChildFrame, true>;

using Group = detail::Widget<ImGui::BeginGroup, ImGui::EndGroup>;

using Table = detail::Widget<ImGui::BeginTable, ImGui::EndTable>;

using TabBar = detail::Widget<ImGui::BeginTabBar, ImGui::EndTabBar>;
using TabItem = detail::Widget<ImGui::BeginTabItem, ImGui::EndTabItem>;

using MenuBar = detail::Widget<ImGui::BeginMenuBar, ImGui::EndMenuBar>;
using Menu = detail::Widget<ImGui::BeginMenu, ImGui::EndMenu>;

using Combo = detail::Widget<ImGui::BeginCombo, ImGui::EndCombo>;
using ListBox = detail::Widget<ImGui::BeginListBox, ImGui::EndListBox>;
using Tooltip = detail::Widget<ImGui::BeginTooltip, ImGui::EndTooltip>;

using Popup = detail::Widget<ImGui::BeginPopup, ImGui::EndPopup>;
using PopupModal = detail::Widget<ImGui::BeginPopupModal, ImGui::EndPopup>;

using DragDropSource = detail::Widget<ImGui::BeginDragDropSource, ImGui::EndDragDropSource>;
using DragDropTarget = detail::Widget<ImGui::BeginDragDropTarget, ImGui::EndDragDropTarget>;

#undef IM_SCOPED_LAMBDA_WRAP
} // namespace ImScoped

namespace ImGui {
// =================================== ImGui x fmtlib ===================================
IMGUI_API void TextFmtV (fmt::string_view format, fmt::format_args);

template <typename Fmt, typename... Args>
void TextFmt (const Fmt& fmt, Args&&... args)
{
	TextFmtV(fmt, fmt::make_format_args(std::forward<Args&&>(args)...));
}

template <typename Fmt, typename... Args>
void TextFmtWrapped (const Fmt& fmt, Args&&... args)
{
	PushTextWrapPos();
	TextFmtV(fmt, fmt::make_format_args(std::forward<Args&&>(args)...));
	PopTextWrapPos();
}

template <typename Fmt, typename... Args>
void TextFmtColored (const ImVec4& col, const Fmt& fmt, Args&&... args)
{
	PushStyleColor(ImGuiCol_Text, col);
	TextFmtV(fmt, fmt::make_format_args(std::forward<Args&&>(args)...));
	PopStyleColor();
}

// ===================== Get ImGuiDataType enum from an actual type =====================
template <typename> constexpr ImGuiDataType DataTypeEnum = ImGuiDataType_COUNT;
template<> constexpr ImGuiDataType DataTypeEnum<int8_t>   = ImGuiDataType_S8;
template<> constexpr ImGuiDataType DataTypeEnum<uint8_t>  = ImGuiDataType_U8;
template<> constexpr ImGuiDataType DataTypeEnum<int16_t>  = ImGuiDataType_S16;
template<> constexpr ImGuiDataType DataTypeEnum<uint16_t> = ImGuiDataType_U16;
template<> constexpr ImGuiDataType DataTypeEnum<int32_t>  = ImGuiDataType_S32;
template<> constexpr ImGuiDataType DataTypeEnum<uint32_t> = ImGuiDataType_U32;
template<> constexpr ImGuiDataType DataTypeEnum<int64_t>  = ImGuiDataType_S64;
template<> constexpr ImGuiDataType DataTypeEnum<uint64_t> = ImGuiDataType_U64;
template<> constexpr ImGuiDataType DataTypeEnum<float>    = ImGuiDataType_Float;
template<> constexpr ImGuiDataType DataTypeEnum<double>   = ImGuiDataType_Double;

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
	return InputScalar(l, DataTypeEnum<T>, p, n, (step == 0 ? nullptr : &step), (step_fast == 0 ? nullptr : &step_fast), fmt, flags);
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
		((s[i++] = 1 + ts % std::numeric_limits<char>::max()), ...);
		s[i] = '\0';
	}
	constexpr operator const char* () const { return s; }
};
template <std::integral... Ts> GenerateId(Ts...) -> GenerateId<sizeof...(Ts)>;

} // namespace ImGui
#endif // IMCPP20_H
