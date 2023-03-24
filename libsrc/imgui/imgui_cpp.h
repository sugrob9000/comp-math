#pragma once
#include "imgui.h"
#include <fmt/core.h>
#include <utility>

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
template<typename> constexpr ImGuiDataType DataTypeEnum;
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

} // namespace ImGui
