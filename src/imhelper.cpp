#include <imgui/imgui_internal.h>
#include "imhelper.hpp"

void ImGui::TextFmtV (fmt::string_view format, fmt::format_args args)
{
	if (!GetCurrentWindow()->SkipItems) {
		const auto& buffer = GImGui->TempBuffer;
		const auto result = fmt::vformat_to_n(buffer.Data, buffer.Size, format, args);
		TextEx(buffer.Data, result.out, ImGuiTextFlags_NoWidthForLargeClippedText);
	}
}