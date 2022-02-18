// not native imgui! demonstrates how to create a widget "from scratch"
// lightly modded from https://github.com/ocornut/imgui/issues/942#issuecomment-268369298

#include "imgui_internal.h"

// Implementing a simple custom widget using the public API.
// You may also use the <imgui_internal.h> API to get raw access to more data/helpers, however the internal API isn't guaranteed to be forward compatible.
// FIXME: Need at least proper label centering + clipping (internal functions RenderTextClipped provides both but api is flaky/temporary)
bool ImGui_Knob(const char* label, float* p_value, float v_min, float v_max, const char *fmt)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    float radius_outer = 20.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float line_height = ImGui::GetTextLineHeight();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float ANGLE_MIN = 3.141592f * 0.75f;
    float ANGLE_MAX = 3.141592f * 2.25f;

    ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer + line_height + style.ItemInnerSpacing.y);
    ImGui::InvisibleButton(label, ImVec2(radius_outer*2, radius_outer*2 + line_height*2 + style.ItemInnerSpacing.y*2));
    bool value_changed = false;
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemActive();
    if (is_active && io.MouseDelta.y != 0.0f)
    {
        float step = (v_max - v_min) / 200.0f;
        *p_value -= io.MouseDelta.y * step;
        if (*p_value < v_min) *p_value = v_min;
        if (*p_value > v_max) *p_value = v_max;
        value_changed = true;
    }

    float t = (*p_value - v_min) / (v_max - v_min);
    float angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * t;
    float angle_cos = cosf(angle), angle_sin = sinf(angle);
    float radius_inner = radius_outer*0.40f;
    draw_list->AddCircleFilled(center, radius_outer, ImGui::GetColorU32(ImGuiCol_FrameBg), 16);
    draw_list->AddLine(ImVec2(center.x + angle_cos*radius_inner, center.y + angle_sin*radius_inner), ImVec2(center.x + angle_cos*(radius_outer-2), center.y + angle_sin*(radius_outer-2)), ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);
    draw_list->AddCircleFilled(center, radius_inner, ImGui::GetColorU32(is_active ? ImGuiCol_FrameBgActive : is_hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);
 
    char buf[256];
    sprintf(buf, fmt, *p_value);
    ImGui::RenderTextClipped(ImVec2(center.x - radius_outer*2, center.y - radius_outer - line_height - style.ItemInnerSpacing.y),
      ImVec2(center.x + radius_outer*2, center.y - radius_outer - style.ItemInnerSpacing.y),
      buf, buf + strlen(buf), NULL, ImVec2(0.5, 0.0), NULL);

    ImGui::RenderTextClipped(ImVec2(center.x - radius_outer*2, center.y + radius_outer + style.ItemInnerSpacing.y),
      ImVec2(center.x + radius_outer*2, center.y + radius_outer + line_height + style.ItemInnerSpacing.y),
      label, label + strlen(label), NULL, ImVec2(0.5, 0.0), NULL);

    return value_changed;
}