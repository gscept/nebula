#pragma warning (disable : 4267)
#include "render/stdneb.h"
#include "nanobind/nanobind.h"
#include "nanobind/make_iterator.h"
#include <array>
#include <limits>
#include "imgui.h"
#include "scripting/scriptserver.h"

struct ImGuiContext;
namespace py = nanobind;

template<typename T>
void template_ImVector(py::module_ &immodule, const char* name)
{
    py::class_< ImVector<T> >(immodule, name)
        .def_prop_ro_static("stride", [](py::object)
        {
            return sizeof(T);
        })
        .def_prop_ro("data", [](const ImVector<T>& self)
        {
            return uintptr_t((void*)self.Data);
        })
        .def("__len__", [](const ImVector<T>& self)
        {
            return self.size();
        })
        .def("__iter__", [](const ImVector<T>& self)
        {
            return py::make_iterator(py::type<ImVector<T>>(), "iterator", self.begin(), self.end());
        })
        .def("__getitem__", [](const ImVector<T>& self, size_t i)
        {
            if ((int)i >= self.size()) throw py::index_error();
            return self[i];
        })
        ;
}

NB_MAKE_OPAQUE(ImGuiContext)
NB_MODULE(deargui, deargui)
{
    //py::class_<ImGuiContext>(deargui, "Context");
    template_ImVector<char>(deargui, "Vector_char");
    template_ImVector<float>(deargui, "Vector_float");
    template_ImVector<unsigned char>(deargui, "Vector_unsignedchar");
    template_ImVector<unsigned short>(deargui, "Vector_unsignedshort");
    template_ImVector<ImDrawCmd>(deargui, "Vector_DrawCmd");
    template_ImVector<ImDrawVert>(deargui, "Vector_DrawVert");
    template_ImVector<ImFontGlyph>(deargui, "Vector_FontGlyph");

    py::class_<ImVec2> Vec2(deargui, "Vec2");
    Vec2.def_rw("x", &ImVec2::x);
    Vec2.def_rw("y", &ImVec2::y);
    Vec2.def(py::init<>());
    Vec2.def(py::init<float, float>()
    , py::arg("_x")
    , py::arg("_y")
    );
    py::class_<ImVec4> Vec4(deargui, "Vec4");
    Vec4.def_rw("x", &ImVec4::x);
    Vec4.def_rw("y", &ImVec4::y);
    Vec4.def_rw("z", &ImVec4::z);
    Vec4.def_rw("w", &ImVec4::w);
    Vec4.def(py::init<>());
    Vec4.def(py::init<float, float, float, float>()
    , py::arg("_x")
    , py::arg("_y")
    , py::arg("_z")
    , py::arg("_w")
    );
    deargui.def("create_context", 
    [](ImFontAtlas* shared_atlas)->void*
    {
        return static_cast<void*>(ImGui::CreateContext(shared_atlas));
    }
    , py::arg("shared_font_atlas") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("destroy_context", 
    [](void* ctx)
    {
        ImGui::DestroyContext(static_cast<ImGuiContext*>(ctx));
    }
    , py::arg("ctx") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("get_current_context",
    []() 
    {
        return static_cast<void*>(ImGui::GetCurrentContext());
    }
    , py::rv_policy::automatic_reference);
    deargui.def("set_current_context", 
    [](void* ctx)
    {
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx));
    }
    , py::arg("ctx")
    , py::rv_policy::automatic_reference);
    deargui.def("debug_check_version_and_data_layout", &ImGui::DebugCheckVersionAndDataLayout
    , py::arg("version_str")
    , py::arg("sz_io")
    , py::arg("sz_style")
    , py::arg("sz_vec2")
    , py::arg("sz_vec4")
    , py::arg("sz_vert")
    , py::arg("sz_idx")
    , py::rv_policy::automatic_reference);
    deargui.def("get_io", &ImGui::GetIO
    , py::rv_policy::reference);
    deargui.def("get_style", &ImGui::GetStyle
    , py::rv_policy::reference);
    deargui.def("new_frame", &ImGui::NewFrame
    , py::rv_policy::automatic_reference);
    deargui.def("end_frame", &ImGui::EndFrame
    , py::rv_policy::automatic_reference);
    deargui.def("render", &ImGui::Render
    , py::rv_policy::automatic_reference);
    deargui.def("get_draw_data", &ImGui::GetDrawData
    , py::rv_policy::automatic_reference);
    deargui.def("show_demo_window", [](bool * p_open)
    {
        ImGui::ShowDemoWindow(p_open);
        return p_open;
    }
    , py::arg("p_open") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("show_metrics_window", [](bool * p_open)
    {
        ImGui::ShowMetricsWindow(p_open);
        return p_open;
    }
    , py::arg("p_open") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("show_style_editor", &ImGui::ShowStyleEditor
    , py::arg("ref") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("show_style_selector", &ImGui::ShowStyleSelector
    , py::arg("label")
    , py::rv_policy::automatic_reference);
    deargui.def("show_font_selector", &ImGui::ShowFontSelector
    , py::arg("label")
    , py::rv_policy::automatic_reference);
    deargui.def("show_user_guide", &ImGui::ShowUserGuide
    , py::rv_policy::automatic_reference);
    deargui.def("get_version", &ImGui::GetVersion
    , py::rv_policy::automatic_reference);
    deargui.def("style_colors_dark", &ImGui::StyleColorsDark
    , py::arg("dst") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("style_colors_classic", &ImGui::StyleColorsClassic
    , py::arg("dst") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("style_colors_light", &ImGui::StyleColorsLight
    , py::arg("dst") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("begin", [](const char * name, bool * p_open, ImGuiWindowFlags flags)
    {
        auto ret = ImGui::Begin(name, p_open, flags);
        return py::make_tuple(ret, p_open);
    }
    , py::arg("name")
    , py::arg("p_open") = nullptr
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("end", &ImGui::End
    , py::rv_policy::automatic_reference);
    deargui.def("begin_child", py::overload_cast<const char *, const ImVec2 &, bool, ImGuiWindowFlags>(&ImGui::BeginChild)
    , py::arg("str_id")
    , py::arg("size") = ImVec2(0,0)
    , py::arg("border") = false
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("begin_child", py::overload_cast<ImGuiID, const ImVec2 &, bool, ImGuiWindowFlags>(&ImGui::BeginChild)
    , py::arg("id")
    , py::arg("size") = ImVec2(0,0)
    , py::arg("border") = false
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("end_child", &ImGui::EndChild
    , py::rv_policy::automatic_reference);
    deargui.def("is_window_appearing", &ImGui::IsWindowAppearing
    , py::rv_policy::automatic_reference);
    deargui.def("is_window_collapsed", &ImGui::IsWindowCollapsed
    , py::rv_policy::automatic_reference);
    deargui.def("is_window_focused", &ImGui::IsWindowFocused
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("is_window_hovered", &ImGui::IsWindowHovered
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("get_window_draw_list", &ImGui::GetWindowDrawList
    , py::rv_policy::automatic_reference);
    deargui.def("get_window_pos", &ImGui::GetWindowPos
    , py::rv_policy::automatic_reference);
    deargui.def("get_window_size", &ImGui::GetWindowSize
    , py::rv_policy::automatic_reference);
    deargui.def("get_window_width", &ImGui::GetWindowWidth
    , py::rv_policy::automatic_reference);
    deargui.def("get_window_height", &ImGui::GetWindowHeight
    , py::rv_policy::automatic_reference);
    deargui.def("get_content_region_max", &ImGui::GetContentRegionMax
    , py::rv_policy::automatic_reference);
    deargui.def("get_content_region_avail", &ImGui::GetContentRegionAvail
    , py::rv_policy::automatic_reference);
    deargui.def("get_window_content_region_min", &ImGui::GetWindowContentRegionMin
    , py::rv_policy::automatic_reference);
    deargui.def("get_window_content_region_max", &ImGui::GetWindowContentRegionMax
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_window_pos", &ImGui::SetNextWindowPos
    , py::arg("pos")
    , py::arg("cond") = 0
    , py::arg("pivot") = ImVec2(0,0)
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_window_size", &ImGui::SetNextWindowSize
    , py::arg("size")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_window_content_size", &ImGui::SetNextWindowContentSize
    , py::arg("size")
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_window_collapsed", &ImGui::SetNextWindowCollapsed
    , py::arg("collapsed")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_window_focus", &ImGui::SetNextWindowFocus
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_window_bg_alpha", &ImGui::SetNextWindowBgAlpha
    , py::arg("alpha")
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_pos", py::overload_cast<const ImVec2 &, ImGuiCond>(&ImGui::SetWindowPos)
    , py::arg("pos")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_size", py::overload_cast<const ImVec2 &, ImGuiCond>(&ImGui::SetWindowSize)
    , py::arg("size")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_collapsed", py::overload_cast<bool, ImGuiCond>(&ImGui::SetWindowCollapsed)
    , py::arg("collapsed")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_focus", py::overload_cast<>(&ImGui::SetWindowFocus)
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_font_scale", &ImGui::SetWindowFontScale
    , py::arg("scale")
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_pos", py::overload_cast<const char *, const ImVec2 &, ImGuiCond>(&ImGui::SetWindowPos)
    , py::arg("name")
    , py::arg("pos")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_size", py::overload_cast<const char *, const ImVec2 &, ImGuiCond>(&ImGui::SetWindowSize)
    , py::arg("name")
    , py::arg("size")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_collapsed", py::overload_cast<const char *, bool, ImGuiCond>(&ImGui::SetWindowCollapsed)
    , py::arg("name")
    , py::arg("collapsed")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_window_focus", py::overload_cast<const char *>(&ImGui::SetWindowFocus)
    , py::arg("name")
    , py::rv_policy::automatic_reference);
    deargui.def("get_scroll_x", &ImGui::GetScrollX
    , py::rv_policy::automatic_reference);
    deargui.def("get_scroll_y", &ImGui::GetScrollY
    , py::rv_policy::automatic_reference);
    deargui.def("get_scroll_max_x", &ImGui::GetScrollMaxX
    , py::rv_policy::automatic_reference);
    deargui.def("get_scroll_max_y", &ImGui::GetScrollMaxY
    , py::rv_policy::automatic_reference);
    deargui.def("set_scroll_x", py::overload_cast<float>(&ImGui::SetScrollX)
    , py::arg("scroll_x")
    , py::rv_policy::automatic_reference);
    deargui.def("set_scroll_y", py::overload_cast<float>(&ImGui::SetScrollY)
    , py::arg("scroll_y")
    , py::rv_policy::automatic_reference);
    deargui.def("set_scroll_here_y", &ImGui::SetScrollHereY
    , py::arg("center_y_ratio") = 0.5f
    , py::rv_policy::automatic_reference);
    deargui.def("set_scroll_from_pos_y", py::overload_cast<float,float>(&ImGui::SetScrollFromPosY)
    , py::arg("local_y")
    , py::arg("center_y_ratio") = 0.5f
    , py::rv_policy::automatic_reference);
    deargui.def("push_font", &ImGui::PushFont
    , py::arg("font")
    , py::rv_policy::automatic_reference);
    deargui.def("pop_font", &ImGui::PopFont
    , py::rv_policy::automatic_reference);
    deargui.def("push_style_color", py::overload_cast<ImGuiCol, ImU32>(&ImGui::PushStyleColor)
    , py::arg("idx")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    deargui.def("push_style_color", py::overload_cast<ImGuiCol, const ImVec4 &>(&ImGui::PushStyleColor)
    , py::arg("idx")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    deargui.def("pop_style_color", &ImGui::PopStyleColor
    , py::arg("count") = 1
    , py::rv_policy::automatic_reference);
    deargui.def("push_style_var", py::overload_cast<ImGuiStyleVar, float>(&ImGui::PushStyleVar)
    , py::arg("idx")
    , py::arg("val")
    , py::rv_policy::automatic_reference);
    deargui.def("push_style_var", py::overload_cast<ImGuiStyleVar, const ImVec2 &>(&ImGui::PushStyleVar)
    , py::arg("idx")
    , py::arg("val")
    , py::rv_policy::automatic_reference);
    deargui.def("pop_style_var", &ImGui::PopStyleVar
    , py::arg("count") = 1
    , py::rv_policy::automatic_reference);
    deargui.def("get_style_color_vec4", &ImGui::GetStyleColorVec4
    , py::arg("idx")
    , py::rv_policy::reference);
    deargui.def("get_font", &ImGui::GetFont
    , py::rv_policy::automatic_reference);
    deargui.def("get_font_size", &ImGui::GetFontSize
    , py::rv_policy::automatic_reference);
    deargui.def("get_font_tex_uv_white_pixel", &ImGui::GetFontTexUvWhitePixel
    , py::rv_policy::automatic_reference);
    deargui.def("get_color_u32", py::overload_cast<ImGuiCol, float>(&ImGui::GetColorU32)
    , py::arg("idx")
    , py::arg("alpha_mul") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("get_color_u32", py::overload_cast<const ImVec4 &>(&ImGui::GetColorU32)
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    deargui.def("get_color_u32", py::overload_cast<ImU32>(&ImGui::GetColorU32)
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    deargui.def("push_item_width", &ImGui::PushItemWidth
    , py::arg("item_width")
    , py::rv_policy::automatic_reference);
    deargui.def("pop_item_width", &ImGui::PopItemWidth
    , py::rv_policy::automatic_reference);
    deargui.def("calc_item_width", &ImGui::CalcItemWidth
    , py::rv_policy::automatic_reference);
    deargui.def("push_text_wrap_pos", &ImGui::PushTextWrapPos
    , py::arg("wrap_pos_x") = 0.0f
    , py::rv_policy::automatic_reference);
    deargui.def("pop_text_wrap_pos", &ImGui::PopTextWrapPos
    , py::rv_policy::automatic_reference);
    deargui.def("push_button_repeat", &ImGui::PushButtonRepeat
    , py::arg("repeat")
    , py::rv_policy::automatic_reference);
    deargui.def("pop_button_repeat", &ImGui::PopButtonRepeat
    , py::rv_policy::automatic_reference);
    deargui.def("separator", &ImGui::Separator
    , py::rv_policy::automatic_reference);
    deargui.def("same_line", &ImGui::SameLine
    , py::arg("pos_x") = 0.0f
    , py::arg("spacing_w") = -1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("new_line", &ImGui::NewLine
    , py::rv_policy::automatic_reference);
    deargui.def("spacing", &ImGui::Spacing
    , py::rv_policy::automatic_reference);
    deargui.def("dummy", &ImGui::Dummy
    , py::arg("size")
    , py::rv_policy::automatic_reference);
    deargui.def("indent", &ImGui::Indent
    , py::arg("indent_w") = 0.0f
    , py::rv_policy::automatic_reference);
    deargui.def("unindent", &ImGui::Unindent
    , py::arg("indent_w") = 0.0f
    , py::rv_policy::automatic_reference);
    deargui.def("begin_group", &ImGui::BeginGroup
    , py::rv_policy::automatic_reference);
    deargui.def("end_group", &ImGui::EndGroup
    , py::rv_policy::automatic_reference);
    deargui.def("get_cursor_pos", &ImGui::GetCursorPos
    , py::rv_policy::automatic_reference);
    deargui.def("get_cursor_pos_x", &ImGui::GetCursorPosX
    , py::rv_policy::automatic_reference);
    deargui.def("get_cursor_pos_y", &ImGui::GetCursorPosY
    , py::rv_policy::automatic_reference);
    deargui.def("set_cursor_pos", &ImGui::SetCursorPos
    , py::arg("local_pos")
    , py::rv_policy::automatic_reference);
    deargui.def("set_cursor_pos_x", &ImGui::SetCursorPosX
    , py::arg("x")
    , py::rv_policy::automatic_reference);
    deargui.def("set_cursor_pos_y", &ImGui::SetCursorPosY
    , py::arg("y")
    , py::rv_policy::automatic_reference);
    deargui.def("get_cursor_start_pos", &ImGui::GetCursorStartPos
    , py::rv_policy::automatic_reference);
    deargui.def("get_cursor_screen_pos", &ImGui::GetCursorScreenPos
    , py::rv_policy::automatic_reference);
    deargui.def("set_cursor_screen_pos", &ImGui::SetCursorScreenPos
    , py::arg("screen_pos")
    , py::rv_policy::automatic_reference);
    deargui.def("align_text_to_frame_padding", &ImGui::AlignTextToFramePadding
    , py::rv_policy::automatic_reference);
    deargui.def("get_text_line_height", &ImGui::GetTextLineHeight
    , py::rv_policy::automatic_reference);
    deargui.def("get_text_line_height_with_spacing", &ImGui::GetTextLineHeightWithSpacing
    , py::rv_policy::automatic_reference);
    deargui.def("get_frame_height", &ImGui::GetFrameHeight
    , py::rv_policy::automatic_reference);
    deargui.def("get_frame_height_with_spacing", &ImGui::GetFrameHeightWithSpacing
    , py::rv_policy::automatic_reference);
    deargui.def("push_id", py::overload_cast<const char *>(&ImGui::PushID)
    , py::arg("str_id")
    , py::rv_policy::automatic_reference);
    deargui.def("push_id", py::overload_cast<const char *, const char *>(&ImGui::PushID)
    , py::arg("str_id_begin")
    , py::arg("str_id_end")
    , py::rv_policy::automatic_reference);
    deargui.def("push_id", py::overload_cast<const void *>(&ImGui::PushID)
    , py::arg("ptr_id")
    , py::rv_policy::automatic_reference);
    deargui.def("push_id", py::overload_cast<int>(&ImGui::PushID)
    , py::arg("int_id")
    , py::rv_policy::automatic_reference);
    deargui.def("pop_id", &ImGui::PopID
    , py::rv_policy::automatic_reference);
    deargui.def("get_id", py::overload_cast<const char *>(&ImGui::GetID)
    , py::arg("str_id")
    , py::rv_policy::automatic_reference);
    deargui.def("get_id", py::overload_cast<const char *, const char *>(&ImGui::GetID)
    , py::arg("str_id_begin")
    , py::arg("str_id_end")
    , py::rv_policy::automatic_reference);
    deargui.def("get_id", py::overload_cast<const void *>(&ImGui::GetID)
    , py::arg("ptr_id")
    , py::rv_policy::automatic_reference);
    deargui.def("text_unformatted", &ImGui::TextUnformatted
    , py::arg("text")
    , py::arg("text_end") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("text", [](const char * fmt)
    {
        ImGui::Text(fmt);
        return ;
    }
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("text_colored", [](const ImVec4 & col, const char * fmt)
    {
        ImGui::TextColored(col, fmt);
        return ;
    }
    , py::arg("col")
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("text_disabled", [](const char * fmt)
    {
        ImGui::TextDisabled(fmt);
        return ;
    }
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("text_wrapped", [](const char * fmt)
    {
        ImGui::TextWrapped(fmt);
        return ;
    }
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("label_text", [](const char * label, const char * fmt)
    {
        ImGui::LabelText(label, fmt);
        return ;
    }
    , py::arg("label")
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("bullet_text", [](const char * fmt)
    {
        ImGui::BulletText(fmt);
        return ;
    }
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("button", &ImGui::Button
    , py::arg("label")
    , py::arg("size") = ImVec2(0,0)
    , py::rv_policy::automatic_reference);
    deargui.def("small_button", &ImGui::SmallButton
    , py::arg("label")
    , py::rv_policy::automatic_reference);
    deargui.def("invisible_button", &ImGui::InvisibleButton
    , py::arg("str_id")
    , py::arg("size")
    , py::arg("flags")
    , py::rv_policy::automatic_reference);
    deargui.def("arrow_button", &ImGui::ArrowButton
    , py::arg("str_id")
    , py::arg("dir")
    , py::rv_policy::automatic_reference);
    deargui.def("image", &ImGui::Image
    , py::arg("user_texture_id")
    , py::arg("size")
    , py::arg("uv0") = ImVec2(0,0)
    , py::arg("uv1") = ImVec2(1,1)
    , py::arg("tint_col") = ImVec4(1,1,1,1)
    , py::arg("border_col") = ImVec4(0,0,0,0)
    , py::rv_policy::automatic_reference);
    deargui.def("image_button", &ImGui::ImageButton
    , py::arg("user_texture_id")
    , py::arg("size")
    , py::arg("uv0") = ImVec2(0,0)
    , py::arg("uv1") = ImVec2(1,1)
    , py::arg("frame_padding") = -1
    , py::arg("bg_col") = ImVec4(0,0,0,0)
    , py::arg("tint_col") = ImVec4(1,1,1,1)
    , py::rv_policy::automatic_reference);
    deargui.def("checkbox", [](const char * label, bool * v)
    {
        auto ret = ImGui::Checkbox(label, v);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::rv_policy::automatic_reference);
    deargui.def("checkbox_flags", [](const char * label, unsigned int * flags, unsigned int flags_value)
    {
        auto ret = ImGui::CheckboxFlags(label, flags, flags_value);
        return py::make_tuple(ret, flags);
    }
    , py::arg("label")
    , py::arg("flags")
    , py::arg("flags_value")
    , py::rv_policy::automatic_reference);
    deargui.def("radio_button", py::overload_cast<const char *, bool>(&ImGui::RadioButton)
    , py::arg("label")
    , py::arg("active")
    , py::rv_policy::automatic_reference);
    deargui.def("radio_button", [](const char * label, int * v, int v_button)
    {
        auto ret = ImGui::RadioButton(label, v, v_button);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_button")
    , py::rv_policy::automatic_reference);
    deargui.def("progress_bar", &ImGui::ProgressBar
    , py::arg("fraction")
    , py::arg("size_arg") = ImVec2(-1,0)
    , py::arg("overlay") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("bullet", &ImGui::Bullet
    , py::rv_policy::automatic_reference);
    deargui.def("begin_combo", &ImGui::BeginCombo
    , py::arg("label")
    , py::arg("preview_value")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("end_combo", &ImGui::EndCombo
    , py::rv_policy::automatic_reference);
    deargui.def("drag_float", [](const char * label, float * v, float v_speed, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0.0f
    , py::arg("v_max") = 0.0f
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("drag_float2", [](const char * label, std::array<float, 2>& v, float v_speed, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::DragFloat2(label, v.data(), v_speed, v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0.0f
    , py::arg("v_max") = 0.0f
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("drag_float3", [](const char * label, std::array<float, 3>& v, float v_speed, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::DragFloat3(label, &v[0], v_speed, v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0.0f
    , py::arg("v_max") = 0.0f
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("drag_float4", [](const char * label, std::array<float, 4>& v, float v_speed, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::DragFloat4(label, &v[0], v_speed, v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0.0f
    , py::arg("v_max") = 0.0f
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("drag_float_range2", [](const char * label, float * v_current_min, float * v_current_max, float v_speed, float v_min, float v_max, const char * format, const char * format_max, float power)
    {
        auto ret = ImGui::DragFloatRange2(label, v_current_min, v_current_max, v_speed, v_min, v_max, format, format_max, power);
        return py::make_tuple(ret, v_current_min, v_current_max);
    }
    , py::arg("label")
    , py::arg("v_current_min")
    , py::arg("v_current_max")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0.0f
    , py::arg("v_max") = 0.0f
    , py::arg("format") = nullptr
    , py::arg("format_max") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("drag_int", [](const char * label, int * v, float v_speed, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::DragInt(label, v, v_speed, v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0
    , py::arg("v_max") = 0
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("drag_int2", [](const char * label, std::array<int, 2>& v, float v_speed, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::DragInt2(label, &v[0], v_speed, v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0
    , py::arg("v_max") = 0
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("drag_int3", [](const char * label, std::array<int, 3>& v, float v_speed, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::DragInt3(label, &v[0], v_speed, v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0
    , py::arg("v_max") = 0
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("drag_int4", [](const char * label, std::array<int, 4>& v, float v_speed, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::DragInt4(label, &v[0], v_speed, v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0
    , py::arg("v_max") = 0
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("drag_int_range2", [](const char * label, int * v_current_min, int * v_current_max, float v_speed, int v_min, int v_max, const char * format, const char * format_max)
    {
        auto ret = ImGui::DragIntRange2(label, v_current_min, v_current_max, v_speed, v_min, v_max, format, format_max);
        return py::make_tuple(ret, v_current_min, v_current_max);
    }
    , py::arg("label")
    , py::arg("v_current_min")
    , py::arg("v_current_max")
    , py::arg("v_speed") = 1.0f
    , py::arg("v_min") = 0
    , py::arg("v_max") = 0
    , py::arg("format") = nullptr
    , py::arg("format_max") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("drag_scalar", &ImGui::DragScalar
    , py::arg("label")
    , py::arg("data_type")
    , py::arg("v")
    , py::arg("v_speed")
    , py::arg("v_min") = nullptr
    , py::arg("v_max") = nullptr
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("drag_scalar_n", &ImGui::DragScalarN
    , py::arg("label")
    , py::arg("data_type")
    , py::arg("v")
    , py::arg("components")
    , py::arg("v_speed")
    , py::arg("v_min") = nullptr
    , py::arg("v_max") = nullptr
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("slider_float", [](const char * label, float * v, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::SliderFloat(label, v, v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("slider_float2", [](const char * label, std::array<float, 2>& v, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::SliderFloat2(label, &v[0], v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("slider_float3", [](const char * label, std::array<float, 3>& v, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::SliderFloat3(label, &v[0], v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("slider_float4", [](const char * label, std::array<float, 4>& v, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::SliderFloat4(label, &v[0], v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("slider_angle", [](const char * label, float * v_rad, float v_degrees_min, float v_degrees_max)
    {
        auto ret = ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max);
        return py::make_tuple(ret, v_rad);
    }
    , py::arg("label")
    , py::arg("v_rad")
    , py::arg("v_degrees_min") = -360.0f
    , py::arg("v_degrees_max") = +360.0f
    , py::rv_policy::automatic_reference);
    deargui.def("slider_int", [](const char * label, int * v, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::SliderInt(label, v, v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("slider_int2", [](const char * label, std::array<int, 2>& v, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::SliderInt2(label, &v[0], v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("slider_int3", [](const char * label, std::array<int, 3>& v, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::SliderInt3(label, &v[0], v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("slider_int4", [](const char * label, std::array<int, 4>& v, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::SliderInt4(label, &v[0], v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("slider_scalar", &ImGui::SliderScalar
    , py::arg("label")
    , py::arg("data_type")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("slider_scalar_n", &ImGui::SliderScalarN
    , py::arg("label")
    , py::arg("data_type")
    , py::arg("v")
    , py::arg("components")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("v_slider_float", [](const char * label, const ImVec2 & size, float * v, float v_min, float v_max, const char * format, float power)
    {
        auto ret = ImGui::VSliderFloat(label, size, v, v_min, v_max, format, power);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("size")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("v_slider_int", [](const char * label, const ImVec2 & size, int * v, int v_min, int v_max, const char * format)
    {
        auto ret = ImGui::VSliderInt(label, size, v, v_min, v_max, format);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("size")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("v_slider_scalar", &ImGui::VSliderScalar
    , py::arg("label")
    , py::arg("size")
    , py::arg("data_type")
    , py::arg("v")
    , py::arg("v_min")
    , py::arg("v_max")
    , py::arg("format") = nullptr
    , py::arg("power") = 1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("input_float", [](const char * label, float * v, float step, float step_fast, const char * format, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputFloat(label, v, step, step_fast, format, extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("step") = 0.0f
    , py::arg("step_fast") = 0.0f
    , py::arg("format") = nullptr
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_float2", [](const char * label, std::array<float, 2>& v, const char * format, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputFloat2(label, &v[0], format, extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("format") = nullptr
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_float3", [](const char * label, std::array<float, 3>& v, const char * format, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputFloat3(label, &v[0], format, extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("format") = nullptr
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_float4", [](const char * label, std::array<float, 4>& v, const char * format, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputFloat4(label, &v[0], format, extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("format") = nullptr
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_int", [](const char * label, int * v, int step, int step_fast, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputInt(label, v, step, step_fast, extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("step") = 1
    , py::arg("step_fast") = 100
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_int2", [](const char * label, std::array<int, 2>& v, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputInt2(label, &v[0], extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_int3", [](const char * label, std::array<int, 3>& v, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputInt3(label, &v[0], extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_int4", [](const char * label, std::array<int, 4>& v, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputInt4(label, &v[0], extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_double", [](const char * label, double * v, double step, double step_fast, const char * format, ImGuiInputTextFlags extra_flags)
    {
        auto ret = ImGui::InputDouble(label, v, step, step_fast, format, extra_flags);
        return py::make_tuple(ret, v);
    }
    , py::arg("label")
    , py::arg("v")
    , py::arg("step") = 0.0f
    , py::arg("step_fast") = 0.0f
    , py::arg("format") = nullptr
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_scalar", &ImGui::InputScalar
    , py::arg("label")
    , py::arg("data_type")
    , py::arg("v")
    , py::arg("step") = nullptr
    , py::arg("step_fast") = nullptr
    , py::arg("format") = nullptr
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_scalar_n", &ImGui::InputScalarN
    , py::arg("label")
    , py::arg("data_type")
    , py::arg("v")
    , py::arg("components")
    , py::arg("step") = nullptr
    , py::arg("step_fast") = nullptr
    , py::arg("format") = nullptr
    , py::arg("extra_flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("color_edit3", [](const char * label, std::array<float, 3>& col, ImGuiColorEditFlags flags)
    {
        auto ret = ImGui::ColorEdit3(label, &col[0], flags);
        return py::make_tuple(ret, col);
    }
    , py::arg("label")
    , py::arg("col")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("color_edit4", [](const char * label, std::array<float, 4>& col, ImGuiColorEditFlags flags)
    {
        auto ret = ImGui::ColorEdit4(label, &col[0], flags);
        return py::make_tuple(ret, col);
    }
    , py::arg("label")
    , py::arg("col")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("color_picker3", [](const char * label, std::array<float, 3>& col, ImGuiColorEditFlags flags)
    {
        auto ret = ImGui::ColorPicker3(label, &col[0], flags);
        return py::make_tuple(ret, col);
    }
    , py::arg("label")
    , py::arg("col")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("color_picker4", [](const char * label, std::array<float, 4>& col, ImGuiColorEditFlags flags, const float * ref_col)
    {
        auto ret = ImGui::ColorPicker4(label, &col[0], flags, ref_col);
        return py::make_tuple(ret, col);
    }
    , py::arg("label")
    , py::arg("col")
    , py::arg("flags") = 0
    , py::arg("ref_col") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("color_button", &ImGui::ColorButton
    , py::arg("desc_id")
    , py::arg("col")
    , py::arg("flags") = 0
    , py::arg("size") = ImVec2(0,0)
    , py::rv_policy::automatic_reference);
    deargui.def("set_color_edit_options", &ImGui::SetColorEditOptions
    , py::arg("flags")
    , py::rv_policy::automatic_reference);
    deargui.def("tree_node", py::overload_cast<const char *>(&ImGui::TreeNode)
    , py::arg("label")
    , py::rv_policy::automatic_reference);
    deargui.def("tree_node", [](const char * str_id, const char * fmt)
    {
        auto ret = ImGui::TreeNode(str_id, fmt);
        return ret;
    }
    , py::arg("str_id")
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("tree_node", [](const void * ptr_id, const char * fmt)
    {
        auto ret = ImGui::TreeNode(ptr_id, fmt);
        return ret;
    }
    , py::arg("ptr_id")
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("tree_node_ex", py::overload_cast<const char *, ImGuiTreeNodeFlags>(&ImGui::TreeNodeEx)
    , py::arg("label")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("tree_node_ex", [](const char * str_id, ImGuiTreeNodeFlags flags, const char * fmt)
    {
        auto ret = ImGui::TreeNodeEx(str_id, flags, fmt);
        return ret;
    }
    , py::arg("str_id")
    , py::arg("flags")
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("tree_node_ex", [](const void * ptr_id, ImGuiTreeNodeFlags flags, const char * fmt)
    {
        auto ret = ImGui::TreeNodeEx(ptr_id, flags, fmt);
        return ret;
    }
    , py::arg("ptr_id")
    , py::arg("flags")
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("tree_push", py::overload_cast<const char *>(&ImGui::TreePush)
    , py::arg("str_id")
    , py::rv_policy::automatic_reference);
    deargui.def("tree_push", py::overload_cast<const void *>(&ImGui::TreePush)
    , py::arg("ptr_id") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("tree_pop", &ImGui::TreePop
    , py::rv_policy::automatic_reference);
    deargui.def("get_tree_node_to_label_spacing", &ImGui::GetTreeNodeToLabelSpacing
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_tree_node_open", &ImGui::SetNextItemOpen
    , py::arg("is_open")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("collapsing_header", py::overload_cast<const char *, ImGuiTreeNodeFlags>(&ImGui::CollapsingHeader)
    , py::arg("label")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("collapsing_header", [](const char * label, bool * p_open, ImGuiTreeNodeFlags flags)
    {
        auto ret = ImGui::CollapsingHeader(label, p_open, flags);
        return py::make_tuple(ret, p_open);
    }
    , py::arg("label")
    , py::arg("p_open")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("selectable", py::overload_cast<const char *, bool, ImGuiSelectableFlags, const ImVec2 &>(&ImGui::Selectable)
    , py::arg("label")
    , py::arg("selected") = false
    , py::arg("flags") = 0
    , py::arg("size") = ImVec2(0,0)
    , py::rv_policy::automatic_reference);
    deargui.def("selectable", [](const char * label, bool * p_selected, ImGuiSelectableFlags flags, const ImVec2 & size)
    {
        auto ret = ImGui::Selectable(label, p_selected, flags, size);
        return py::make_tuple(ret, p_selected);
    }
    , py::arg("label")
    , py::arg("p_selected")
    , py::arg("flags") = 0
    , py::arg("size") = ImVec2(0,0)
    , py::rv_policy::automatic_reference);
    deargui.def("value", py::overload_cast<const char *, bool>(&ImGui::Value)
    , py::arg("prefix")
    , py::arg("b")
    , py::rv_policy::automatic_reference);
    deargui.def("value", py::overload_cast<const char *, int>(&ImGui::Value)
    , py::arg("prefix")
    , py::arg("v")
    , py::rv_policy::automatic_reference);
    deargui.def("value", py::overload_cast<const char *, unsigned int>(&ImGui::Value)
    , py::arg("prefix")
    , py::arg("v")
    , py::rv_policy::automatic_reference);
    deargui.def("value", py::overload_cast<const char *, float, const char *>(&ImGui::Value)
    , py::arg("prefix")
    , py::arg("v")
    , py::arg("float_format") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("begin_main_menu_bar", &ImGui::BeginMainMenuBar
    , py::rv_policy::automatic_reference);
    deargui.def("end_main_menu_bar", &ImGui::EndMainMenuBar
    , py::rv_policy::automatic_reference);
    deargui.def("begin_menu_bar", &ImGui::BeginMenuBar
    , py::rv_policy::automatic_reference);
    deargui.def("end_menu_bar", &ImGui::EndMenuBar
    , py::rv_policy::automatic_reference);
    deargui.def("begin_menu", &ImGui::BeginMenu
    , py::arg("label")
    , py::arg("enabled") = true
    , py::rv_policy::automatic_reference);
    deargui.def("end_menu", &ImGui::EndMenu
    , py::rv_policy::automatic_reference);
    deargui.def("menu_item", py::overload_cast<const char *, const char *, bool, bool>(&ImGui::MenuItem)
    , py::arg("label")
    , py::arg("shortcut") = nullptr
    , py::arg("selected") = false
    , py::arg("enabled") = true
    , py::rv_policy::automatic_reference);
    deargui.def("menu_item", [](const char * label, const char * shortcut, bool * p_selected, bool enabled)
    {
        auto ret = ImGui::MenuItem(label, shortcut, p_selected, enabled);
        return py::make_tuple(ret, p_selected);
    }
    , py::arg("label")
    , py::arg("shortcut")
    , py::arg("p_selected")
    , py::arg("enabled") = true
    , py::rv_policy::automatic_reference);
    deargui.def("begin_tooltip", &ImGui::BeginTooltip
    , py::rv_policy::automatic_reference);
    deargui.def("end_tooltip", &ImGui::EndTooltip
    , py::rv_policy::automatic_reference);
    deargui.def("set_tooltip", [](const char * fmt)
    {
        ImGui::SetTooltip(fmt);
        return ;
    }
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
   
    deargui.def("open_popup", py::overload_cast<const char*, ImGuiPopupFlags>(&ImGui::OpenPopup)
    , py::arg("str_id")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("begin_popup", &ImGui::BeginPopup
    , py::arg("str_id")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("begin_popup_context_item", &ImGui::BeginPopupContextItem
    , py::arg("str_id") = nullptr
    , py::arg("mouse_button") = 1
    , py::rv_policy::automatic_reference);
    deargui.def("begin_popup_context_window", &ImGui::BeginPopupContextWindow
    , py::arg("str_id") = nullptr
    , py::arg("flags")
    , py::rv_policy::automatic_reference);
    deargui.def("begin_popup_context_void", &ImGui::BeginPopupContextVoid
    , py::arg("str_id") = nullptr
    , py::arg("mouse_button") = 1
    , py::rv_policy::automatic_reference);
    deargui.def("begin_popup_modal", [](const char * name, bool * p_open, ImGuiWindowFlags flags)
    {
        auto ret = ImGui::BeginPopupModal(name, p_open, flags);
        return py::make_tuple(ret, p_open);
    }
    , py::arg("name")
    , py::arg("p_open") = nullptr
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("end_popup", &ImGui::EndPopup
    , py::rv_policy::automatic_reference);
    deargui.def("open_popup_on_item_click", &ImGui::OpenPopupOnItemClick
    , py::arg("str_id") = nullptr
    , py::arg("mouse_button") = 1
    , py::rv_policy::automatic_reference);
    deargui.def("is_popup_open", py::overload_cast<const char *, ImGuiPopupFlags>(&ImGui::IsPopupOpen)
    , py::arg("str_id")
    , py::arg("flags")
    , py::rv_policy::automatic_reference);
    deargui.def("close_current_popup", &ImGui::CloseCurrentPopup
    , py::rv_policy::automatic_reference);
    deargui.def("columns", &ImGui::Columns
    , py::arg("count") = 1
    , py::arg("id") = nullptr
    , py::arg("border") = true
    , py::rv_policy::automatic_reference);
    deargui.def("next_column", &ImGui::NextColumn
    , py::rv_policy::automatic_reference);
    deargui.def("get_column_index", &ImGui::GetColumnIndex
    , py::rv_policy::automatic_reference);
    deargui.def("get_column_width", &ImGui::GetColumnWidth
    , py::arg("column_index") = -1
    , py::rv_policy::automatic_reference);
    deargui.def("set_column_width", &ImGui::SetColumnWidth
    , py::arg("column_index")
    , py::arg("width")
    , py::rv_policy::automatic_reference);
    deargui.def("get_column_offset", &ImGui::GetColumnOffset
    , py::arg("column_index") = -1
    , py::rv_policy::automatic_reference);
    deargui.def("set_column_offset", &ImGui::SetColumnOffset
    , py::arg("column_index")
    , py::arg("offset_x")
    , py::rv_policy::automatic_reference);
    deargui.def("get_columns_count", &ImGui::GetColumnsCount
    , py::rv_policy::automatic_reference);
    deargui.def("log_to_tty", &ImGui::LogToTTY
    , py::arg("max_depth") = -1
    , py::rv_policy::automatic_reference);
    deargui.def("log_to_file", &ImGui::LogToFile
    , py::arg("max_depth") = -1
    , py::arg("filename") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("log_to_clipboard", &ImGui::LogToClipboard
    , py::arg("max_depth") = -1
    , py::rv_policy::automatic_reference);
    deargui.def("log_finish", &ImGui::LogFinish
    , py::rv_policy::automatic_reference);
    deargui.def("log_buttons", &ImGui::LogButtons
    , py::rv_policy::automatic_reference);
    deargui.def("log_text", [](const char * fmt)
    {
        ImGui::LogText(fmt);
        return ;
    }
    , py::arg("fmt")
    , py::rv_policy::automatic_reference);
    deargui.def("begin_drag_drop_source", &ImGui::BeginDragDropSource
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("set_drag_drop_payload", &ImGui::SetDragDropPayload
    , py::arg("type")
    , py::arg("data")
    , py::arg("size")
    , py::arg("cond") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("end_drag_drop_source", &ImGui::EndDragDropSource
    , py::rv_policy::automatic_reference);
    deargui.def("begin_drag_drop_target", &ImGui::BeginDragDropTarget
    , py::rv_policy::automatic_reference);
    deargui.def("accept_drag_drop_payload", &ImGui::AcceptDragDropPayload
    , py::arg("type")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("end_drag_drop_target", &ImGui::EndDragDropTarget
    , py::rv_policy::automatic_reference);
    deargui.def("push_clip_rect", &ImGui::PushClipRect
    , py::arg("clip_rect_min")
    , py::arg("clip_rect_max")
    , py::arg("intersect_with_current_clip_rect")
    , py::rv_policy::automatic_reference);
    deargui.def("pop_clip_rect", &ImGui::PopClipRect
    , py::rv_policy::automatic_reference);
    deargui.def("set_item_default_focus", &ImGui::SetItemDefaultFocus
    , py::rv_policy::automatic_reference);
    deargui.def("set_keyboard_focus_here", &ImGui::SetKeyboardFocusHere
    , py::arg("offset") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("is_item_hovered", &ImGui::IsItemHovered
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("is_item_active", &ImGui::IsItemActive
    , py::rv_policy::automatic_reference);
    deargui.def("is_item_focused", &ImGui::IsItemFocused
    , py::rv_policy::automatic_reference);
    deargui.def("is_item_clicked", &ImGui::IsItemClicked
    , py::arg("mouse_button") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("is_item_visible", &ImGui::IsItemVisible
    , py::rv_policy::automatic_reference);
    deargui.def("is_item_edited", &ImGui::IsItemEdited
    , py::rv_policy::automatic_reference);
    deargui.def("is_item_deactivated", &ImGui::IsItemDeactivated
    , py::rv_policy::automatic_reference);
    deargui.def("is_item_deactivated_after_edit", &ImGui::IsItemDeactivatedAfterEdit
    , py::rv_policy::automatic_reference);
    deargui.def("is_any_item_hovered", &ImGui::IsAnyItemHovered
    , py::rv_policy::automatic_reference);
    deargui.def("is_any_item_active", &ImGui::IsAnyItemActive
    , py::rv_policy::automatic_reference);
    deargui.def("is_any_item_focused", &ImGui::IsAnyItemFocused
    , py::rv_policy::automatic_reference);
    deargui.def("get_item_rect_min", &ImGui::GetItemRectMin
    , py::rv_policy::automatic_reference);
    deargui.def("get_item_rect_max", &ImGui::GetItemRectMax
    , py::rv_policy::automatic_reference);
    deargui.def("get_item_rect_size", &ImGui::GetItemRectSize
    , py::rv_policy::automatic_reference);
    deargui.def("set_item_allow_overlap", &ImGui::SetItemAllowOverlap
    , py::rv_policy::automatic_reference);
    deargui.def("is_rect_visible", py::overload_cast<const ImVec2 &>(&ImGui::IsRectVisible)
    , py::arg("size")
    , py::rv_policy::automatic_reference);
    deargui.def("is_rect_visible", py::overload_cast<const ImVec2 &, const ImVec2 &>(&ImGui::IsRectVisible)
    , py::arg("rect_min")
    , py::arg("rect_max")
    , py::rv_policy::automatic_reference);
    deargui.def("get_time", &ImGui::GetTime
    , py::rv_policy::automatic_reference);
    deargui.def("get_frame_count", &ImGui::GetFrameCount
    , py::rv_policy::automatic_reference);
    deargui.def("get_foreground_draw_list", py::overload_cast<>(&ImGui::GetForegroundDrawList)
    , py::rv_policy::automatic_reference);
    //deargui.def("get_draw_list_shared_data", &ImGui::GetDrawListSharedData
    //, py::rv_policy::automatic_reference);
    deargui.def("get_style_color_name", &ImGui::GetStyleColorName
    , py::arg("idx")
    , py::rv_policy::automatic_reference);
    deargui.def("set_state_storage", &ImGui::SetStateStorage
    , py::arg("storage")
    , py::rv_policy::automatic_reference);
    deargui.def("get_state_storage", &ImGui::GetStateStorage
    , py::rv_policy::automatic_reference);
    deargui.def("calc_text_size", &ImGui::CalcTextSize
    , py::arg("text")
    , py::arg("text_end") = nullptr
    , py::arg("hide_text_after_double_hash") = false
    , py::arg("wrap_width") = -1.0f
    , py::rv_policy::automatic_reference);
    
    deargui.def("begin_child_frame", &ImGui::BeginChildFrame
    , py::arg("id")
    , py::arg("size")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("end_child_frame", &ImGui::EndChildFrame
    , py::rv_policy::automatic_reference);
    deargui.def("color_convert_u32_to_float4", &ImGui::ColorConvertU32ToFloat4
    , py::arg("in")
    , py::rv_policy::automatic_reference);
    deargui.def("color_convert_float4_to_u32", &ImGui::ColorConvertFloat4ToU32
    , py::arg("in")
    , py::rv_policy::automatic_reference);
    deargui.def("color_convert_rg_bto_hsv", [](float r, float g, float b, float & out_h, float & out_s, float & out_v)
    {
        ImGui::ColorConvertRGBtoHSV(r, g, b, out_h, out_s, out_v);
        return py::make_tuple(out_h, out_s, out_v);
    }
    , py::arg("r")
    , py::arg("g")
    , py::arg("b")
    , py::arg("out_h") = 0
    , py::arg("out_s") = 0
    , py::arg("out_v") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("color_convert_hs_vto_rgb", [](float h, float s, float v, float & out_r, float & out_g, float & out_b)
    {
        ImGui::ColorConvertHSVtoRGB(h, s, v, out_r, out_g, out_b);
        return py::make_tuple(out_r, out_g, out_b);
    }
    , py::arg("h")
    , py::arg("s")
    , py::arg("v")
    , py::arg("out_r") = 0
    , py::arg("out_g") = 0
    , py::arg("out_b")
    , py::rv_policy::automatic_reference);
    deargui.def("get_key_index", &ImGui::GetKeyIndex
    , py::arg("imgui_key")
    , py::rv_policy::automatic_reference);
    deargui.def("is_key_down", py::overload_cast<ImGuiKey>(&ImGui::IsKeyDown)
    , py::arg("user_key_index")
    , py::rv_policy::automatic_reference);
    deargui.def("is_key_pressed", py::overload_cast<ImGuiKey,bool>(&ImGui::IsKeyPressed)
    , py::arg("user_key_index")
    , py::arg("repeat") = true
    , py::rv_policy::automatic_reference);
    deargui.def("is_key_released", py::overload_cast<ImGuiKey>(&ImGui::IsKeyReleased)
    , py::arg("user_key_index")
    , py::rv_policy::automatic_reference);
    deargui.def("get_key_pressed_amount", &ImGui::GetKeyPressedAmount
    , py::arg("key_index")
    , py::arg("repeat_delay")
    , py::arg("rate")
    , py::rv_policy::automatic_reference);
    deargui.def("is_mouse_down", py::overload_cast<ImGuiMouseButton>(&ImGui::IsMouseDown)
    , py::arg("button")
    , py::rv_policy::automatic_reference);
    deargui.def("is_any_mouse_down", &ImGui::IsAnyMouseDown
    , py::rv_policy::automatic_reference);
    deargui.def("is_mouse_clicked", py::overload_cast<ImGuiMouseButton, bool>(&ImGui::IsMouseClicked)
    , py::arg("button")
    , py::arg("repeat") = false
    , py::rv_policy::automatic_reference);
    deargui.def("is_mouse_double_clicked", &ImGui::IsMouseDoubleClicked
    , py::arg("button")
    , py::rv_policy::automatic_reference);
    deargui.def("is_mouse_released", py::overload_cast<ImGuiMouseButton>(&ImGui::IsMouseReleased)
    , py::arg("button")
    , py::rv_policy::automatic_reference);
    deargui.def("is_mouse_dragging", &ImGui::IsMouseDragging
    , py::arg("button") = 0
    , py::arg("lock_threshold") = -1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("is_mouse_hovering_rect", &ImGui::IsMouseHoveringRect
    , py::arg("r_min")
    , py::arg("r_max")
    , py::arg("clip") = true
    , py::rv_policy::automatic_reference);
    deargui.def("is_mouse_pos_valid", &ImGui::IsMousePosValid
    , py::arg("mouse_pos") = nullptr
    , py::rv_policy::automatic_reference);
    deargui.def("get_mouse_pos", &ImGui::GetMousePos
    , py::rv_policy::automatic_reference);
    deargui.def("get_mouse_pos_on_opening_current_popup", &ImGui::GetMousePosOnOpeningCurrentPopup
    , py::rv_policy::automatic_reference);
    deargui.def("get_mouse_drag_delta", &ImGui::GetMouseDragDelta
    , py::arg("button") = 0
    , py::arg("lock_threshold") = -1.0f
    , py::rv_policy::automatic_reference);
    deargui.def("reset_mouse_drag_delta", &ImGui::ResetMouseDragDelta
    , py::arg("button") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("get_mouse_cursor", &ImGui::GetMouseCursor
    , py::rv_policy::automatic_reference);
    deargui.def("set_mouse_cursor", &ImGui::SetMouseCursor
    , py::arg("type")
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_frame_want_capture_keyboard", &ImGui::SetNextFrameWantCaptureKeyboard
    , py::arg("capture") = true
    , py::rv_policy::automatic_reference);
    deargui.def("set_next_frame_want_capture_mouse", &ImGui::SetNextFrameWantCaptureMouse
    , py::arg("capture") = true
    , py::rv_policy::automatic_reference);
    deargui.def("get_clipboard_text", &ImGui::GetClipboardText
    , py::rv_policy::automatic_reference);
    deargui.def("set_clipboard_text", &ImGui::SetClipboardText
    , py::arg("text")
    , py::rv_policy::automatic_reference);
    deargui.def("load_ini_settings_from_disk", &ImGui::LoadIniSettingsFromDisk
    , py::arg("ini_filename")
    , py::rv_policy::automatic_reference);
    deargui.def("load_ini_settings_from_memory", &ImGui::LoadIniSettingsFromMemory
    , py::arg("ini_data")
    , py::arg("ini_size") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("save_ini_settings_to_disk", &ImGui::SaveIniSettingsToDisk
    , py::arg("ini_filename")
    , py::rv_policy::automatic_reference);
    deargui.def("save_ini_settings_to_memory", [](size_t * out_ini_size)
    {
        auto ret = ImGui::SaveIniSettingsToMemory(out_ini_size);
        return py::make_tuple(ret, out_ini_size);
    }
    , py::arg("out_ini_size") = 0
    , py::rv_policy::automatic_reference);
    py::enum_<ImGuiWindowFlags_>(deargui, "WindowFlags", py::is_arithmetic())
        .value("WINDOW_FLAGS_NONE", ImGuiWindowFlags_None)
        .value("WINDOW_FLAGS_NO_TITLE_BAR", ImGuiWindowFlags_NoTitleBar)
        .value("WINDOW_FLAGS_NO_RESIZE", ImGuiWindowFlags_NoResize)
        .value("WINDOW_FLAGS_NO_MOVE", ImGuiWindowFlags_NoMove)
        .value("WINDOW_FLAGS_NO_SCROLLBAR", ImGuiWindowFlags_NoScrollbar)
        .value("WINDOW_FLAGS_NO_SCROLL_WITH_MOUSE", ImGuiWindowFlags_NoScrollWithMouse)
        .value("WINDOW_FLAGS_NO_COLLAPSE", ImGuiWindowFlags_NoCollapse)
        .value("WINDOW_FLAGS_ALWAYS_AUTO_RESIZE", ImGuiWindowFlags_AlwaysAutoResize)
        .value("WINDOW_FLAGS_NO_SAVED_SETTINGS", ImGuiWindowFlags_NoSavedSettings)
        .value("WINDOW_FLAGS_NO_INPUTS", ImGuiWindowFlags_NoInputs)
        .value("WINDOW_FLAGS_MENU_BAR", ImGuiWindowFlags_MenuBar)
        .value("WINDOW_FLAGS_HORIZONTAL_SCROLLBAR", ImGuiWindowFlags_HorizontalScrollbar)
        .value("WINDOW_FLAGS_NO_FOCUS_ON_APPEARING", ImGuiWindowFlags_NoFocusOnAppearing)
        .value("WINDOW_FLAGS_NO_BRING_TO_FRONT_ON_FOCUS", ImGuiWindowFlags_NoBringToFrontOnFocus)
        .value("WINDOW_FLAGS_ALWAYS_VERTICAL_SCROLLBAR", ImGuiWindowFlags_AlwaysVerticalScrollbar)
        .value("WINDOW_FLAGS_ALWAYS_HORIZONTAL_SCROLLBAR", ImGuiWindowFlags_AlwaysHorizontalScrollbar)
        .value("WINDOW_FLAGS_ALWAYS_USE_WINDOW_PADDING", ImGuiWindowFlags_AlwaysUseWindowPadding)
        .value("WINDOW_FLAGS_NO_NAV_INPUTS", ImGuiWindowFlags_NoNavInputs)
        .value("WINDOW_FLAGS_NO_NAV_FOCUS", ImGuiWindowFlags_NoNavFocus)
        .value("WINDOW_FLAGS_NO_NAV", ImGuiWindowFlags_NoNav)
        .value("WINDOW_FLAGS_NAV_FLATTENED", ImGuiWindowFlags_NavFlattened)
        .value("WINDOW_FLAGS_CHILD_WINDOW", ImGuiWindowFlags_ChildWindow)
        .value("WINDOW_FLAGS_TOOLTIP", ImGuiWindowFlags_Tooltip)
        .value("WINDOW_FLAGS_POPUP", ImGuiWindowFlags_Popup)
        .value("WINDOW_FLAGS_MODAL", ImGuiWindowFlags_Modal)
        .value("WINDOW_FLAGS_CHILD_MENU", ImGuiWindowFlags_ChildMenu)
        .export_values();

    py::enum_<ImGuiInputTextFlags_>(deargui, "InputTextFlags", py::is_arithmetic())
        .value("INPUT_TEXT_FLAGS_NONE", ImGuiInputTextFlags_None)
        .value("INPUT_TEXT_FLAGS_CHARS_DECIMAL", ImGuiInputTextFlags_CharsDecimal)
        .value("INPUT_TEXT_FLAGS_CHARS_HEXADECIMAL", ImGuiInputTextFlags_CharsHexadecimal)
        .value("INPUT_TEXT_FLAGS_CHARS_UPPERCASE", ImGuiInputTextFlags_CharsUppercase)
        .value("INPUT_TEXT_FLAGS_CHARS_NO_BLANK", ImGuiInputTextFlags_CharsNoBlank)
        .value("INPUT_TEXT_FLAGS_AUTO_SELECT_ALL", ImGuiInputTextFlags_AutoSelectAll)
        .value("INPUT_TEXT_FLAGS_ENTER_RETURNS_TRUE", ImGuiInputTextFlags_EnterReturnsTrue)
        .value("INPUT_TEXT_FLAGS_CALLBACK_COMPLETION", ImGuiInputTextFlags_CallbackCompletion)
        .value("INPUT_TEXT_FLAGS_CALLBACK_HISTORY", ImGuiInputTextFlags_CallbackHistory)
        .value("INPUT_TEXT_FLAGS_CALLBACK_ALWAYS", ImGuiInputTextFlags_CallbackAlways)
        .value("INPUT_TEXT_FLAGS_CALLBACK_CHAR_FILTER", ImGuiInputTextFlags_CallbackCharFilter)
        .value("INPUT_TEXT_FLAGS_ALLOW_TAB_INPUT", ImGuiInputTextFlags_AllowTabInput)
        .value("INPUT_TEXT_FLAGS_CTRL_ENTER_FOR_NEW_LINE", ImGuiInputTextFlags_CtrlEnterForNewLine)
        .value("INPUT_TEXT_FLAGS_NO_HORIZONTAL_SCROLL", ImGuiInputTextFlags_NoHorizontalScroll)
        .value("INPUT_TEXT_FLAGS_ALWAYS_OVERWRITE", ImGuiInputTextFlags_AlwaysOverwrite)
        .value("INPUT_TEXT_FLAGS_READ_ONLY", ImGuiInputTextFlags_ReadOnly)
        .value("INPUT_TEXT_FLAGS_PASSWORD", ImGuiInputTextFlags_Password)
        .value("INPUT_TEXT_FLAGS_NO_UNDO_REDO", ImGuiInputTextFlags_NoUndoRedo)
        .value("INPUT_TEXT_FLAGS_CHARS_SCIENTIFIC", ImGuiInputTextFlags_CharsScientific)
        .value("INPUT_TEXT_FLAGS_CALLBACK_RESIZE", ImGuiInputTextFlags_CallbackResize)
        .export_values();

    py::enum_<ImGuiTreeNodeFlags_>(deargui, "TreeNodeFlags", py::is_arithmetic())
        .value("TREE_NODE_FLAGS_NONE", ImGuiTreeNodeFlags_None)
        .value("TREE_NODE_FLAGS_SELECTED", ImGuiTreeNodeFlags_Selected)
        .value("TREE_NODE_FLAGS_FRAMED", ImGuiTreeNodeFlags_Framed)
        .value("TREE_NODE_FLAGS_ALLOW_ITEM_OVERLAP", ImGuiTreeNodeFlags_AllowItemOverlap)
        .value("TREE_NODE_FLAGS_NO_TREE_PUSH_ON_OPEN", ImGuiTreeNodeFlags_NoTreePushOnOpen)
        .value("TREE_NODE_FLAGS_NO_AUTO_OPEN_ON_LOG", ImGuiTreeNodeFlags_NoAutoOpenOnLog)
        .value("TREE_NODE_FLAGS_DEFAULT_OPEN", ImGuiTreeNodeFlags_DefaultOpen)
        .value("TREE_NODE_FLAGS_OPEN_ON_DOUBLE_CLICK", ImGuiTreeNodeFlags_OpenOnDoubleClick)
        .value("TREE_NODE_FLAGS_OPEN_ON_ARROW", ImGuiTreeNodeFlags_OpenOnArrow)
        .value("TREE_NODE_FLAGS_LEAF", ImGuiTreeNodeFlags_Leaf)
        .value("TREE_NODE_FLAGS_BULLET", ImGuiTreeNodeFlags_Bullet)
        .value("TREE_NODE_FLAGS_FRAME_PADDING", ImGuiTreeNodeFlags_FramePadding)
        .value("TREE_NODE_FLAGS_NAV_LEFT_JUMPS_BACK_HERE", ImGuiTreeNodeFlags_NavLeftJumpsBackHere)
        .value("TREE_NODE_FLAGS_COLLAPSING_HEADER", ImGuiTreeNodeFlags_CollapsingHeader)
        .export_values();

    py::enum_<ImGuiSelectableFlags_>(deargui, "SelectableFlags", py::is_arithmetic())
        .value("SELECTABLE_FLAGS_NONE", ImGuiSelectableFlags_None)
        .value("SELECTABLE_FLAGS_DONT_CLOSE_POPUPS", ImGuiSelectableFlags_DontClosePopups)
        .value("SELECTABLE_FLAGS_SPAN_ALL_COLUMNS", ImGuiSelectableFlags_SpanAllColumns)
        .value("SELECTABLE_FLAGS_ALLOW_DOUBLE_CLICK", ImGuiSelectableFlags_AllowDoubleClick)
        .value("SELECTABLE_FLAGS_DISABLED", ImGuiSelectableFlags_Disabled)
        .export_values();

    py::enum_<ImGuiComboFlags_>(deargui, "ComboFlags", py::is_arithmetic())
        .value("COMBO_FLAGS_NONE", ImGuiComboFlags_None)
        .value("COMBO_FLAGS_POPUP_ALIGN_LEFT", ImGuiComboFlags_PopupAlignLeft)
        .value("COMBO_FLAGS_HEIGHT_SMALL", ImGuiComboFlags_HeightSmall)
        .value("COMBO_FLAGS_HEIGHT_REGULAR", ImGuiComboFlags_HeightRegular)
        .value("COMBO_FLAGS_HEIGHT_LARGE", ImGuiComboFlags_HeightLarge)
        .value("COMBO_FLAGS_HEIGHT_LARGEST", ImGuiComboFlags_HeightLargest)
        .value("COMBO_FLAGS_NO_ARROW_BUTTON", ImGuiComboFlags_NoArrowButton)
        .value("COMBO_FLAGS_NO_PREVIEW", ImGuiComboFlags_NoPreview)
        .value("COMBO_FLAGS_HEIGHT_MASK", ImGuiComboFlags_HeightMask_)
        .export_values();

    py::enum_<ImGuiFocusedFlags_>(deargui, "FocusedFlags", py::is_arithmetic())
        .value("FOCUSED_FLAGS_NONE", ImGuiFocusedFlags_None)
        .value("FOCUSED_FLAGS_CHILD_WINDOWS", ImGuiFocusedFlags_ChildWindows)
        .value("FOCUSED_FLAGS_ROOT_WINDOW", ImGuiFocusedFlags_RootWindow)
        .value("FOCUSED_FLAGS_ANY_WINDOW", ImGuiFocusedFlags_AnyWindow)
        .value("FOCUSED_FLAGS_ROOT_AND_CHILD_WINDOWS", ImGuiFocusedFlags_RootAndChildWindows)
        .export_values();

    py::enum_<ImGuiHoveredFlags_>(deargui, "HoveredFlags", py::is_arithmetic())
        .value("HOVERED_FLAGS_NONE", ImGuiHoveredFlags_None)
        .value("HOVERED_FLAGS_CHILD_WINDOWS", ImGuiHoveredFlags_ChildWindows)
        .value("HOVERED_FLAGS_ROOT_WINDOW", ImGuiHoveredFlags_RootWindow)
        .value("HOVERED_FLAGS_ANY_WINDOW", ImGuiHoveredFlags_AnyWindow)
        .value("HOVERED_FLAGS_ALLOW_WHEN_BLOCKED_BY_POPUP", ImGuiHoveredFlags_AllowWhenBlockedByPopup)
        .value("HOVERED_FLAGS_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM", ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
        .value("HOVERED_FLAGS_ALLOW_WHEN_OVERLAPPED", ImGuiHoveredFlags_AllowWhenOverlapped)
        .value("HOVERED_FLAGS_ALLOW_WHEN_DISABLED", ImGuiHoveredFlags_AllowWhenDisabled)
        .value("HOVERED_FLAGS_RECT_ONLY", ImGuiHoveredFlags_RectOnly)
        .value("HOVERED_FLAGS_ROOT_AND_CHILD_WINDOWS", ImGuiHoveredFlags_RootAndChildWindows)
        .export_values();

    py::enum_<ImGuiDragDropFlags_>(deargui, "DragDropFlags", py::is_arithmetic())
        .value("DRAG_DROP_FLAGS_NONE", ImGuiDragDropFlags_None)
        .value("DRAG_DROP_FLAGS_SOURCE_NO_PREVIEW_TOOLTIP", ImGuiDragDropFlags_SourceNoPreviewTooltip)
        .value("DRAG_DROP_FLAGS_SOURCE_NO_DISABLE_HOVER", ImGuiDragDropFlags_SourceNoDisableHover)
        .value("DRAG_DROP_FLAGS_SOURCE_NO_HOLD_TO_OPEN_OTHERS", ImGuiDragDropFlags_SourceNoHoldToOpenOthers)
        .value("DRAG_DROP_FLAGS_SOURCE_ALLOW_NULL_ID", ImGuiDragDropFlags_SourceAllowNullID)
        .value("DRAG_DROP_FLAGS_SOURCE_EXTERN", ImGuiDragDropFlags_SourceExtern)
        .value("DRAG_DROP_FLAGS_SOURCE_AUTO_EXPIRE_PAYLOAD", ImGuiDragDropFlags_SourceAutoExpirePayload)
        .value("DRAG_DROP_FLAGS_ACCEPT_BEFORE_DELIVERY", ImGuiDragDropFlags_AcceptBeforeDelivery)
        .value("DRAG_DROP_FLAGS_ACCEPT_NO_DRAW_DEFAULT_RECT", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)
        .value("DRAG_DROP_FLAGS_ACCEPT_NO_PREVIEW_TOOLTIP", ImGuiDragDropFlags_AcceptNoPreviewTooltip)
        .value("DRAG_DROP_FLAGS_ACCEPT_PEEK_ONLY", ImGuiDragDropFlags_AcceptPeekOnly)
        .export_values();

    py::enum_<ImGuiDataType_>(deargui, "DataType", py::is_arithmetic())
        .value("DATA_TYPE_S32", ImGuiDataType_S32)
        .value("DATA_TYPE_U32", ImGuiDataType_U32)
        .value("DATA_TYPE_S64", ImGuiDataType_S64)
        .value("DATA_TYPE_U64", ImGuiDataType_U64)
        .value("DATA_TYPE_FLOAT", ImGuiDataType_Float)
        .value("DATA_TYPE_DOUBLE", ImGuiDataType_Double)
        .value("DATA_TYPE_COUNT", ImGuiDataType_COUNT)
        .export_values();

    py::enum_<ImGuiDir_>(deargui, "Dir", py::is_arithmetic())
        .value("DIR_NONE", ImGuiDir_None)
        .value("DIR_LEFT", ImGuiDir_Left)
        .value("DIR_RIGHT", ImGuiDir_Right)
        .value("DIR_UP", ImGuiDir_Up)
        .value("DIR_DOWN", ImGuiDir_Down)
        .value("DIR_COUNT", ImGuiDir_COUNT)
        .export_values();

    py::enum_<ImGuiKey>(deargui, "Key", py::is_arithmetic())
        .value("KEY_TAB", ImGuiKey_Tab)
        .value("KEY_LEFT_ARROW", ImGuiKey_LeftArrow)
        .value("KEY_RIGHT_ARROW", ImGuiKey_RightArrow)
        .value("KEY_UP_ARROW", ImGuiKey_UpArrow)
        .value("KEY_DOWN_ARROW", ImGuiKey_DownArrow)
        .value("KEY_PAGE_UP", ImGuiKey_PageUp)
        .value("KEY_PAGE_DOWN", ImGuiKey_PageDown)
        .value("KEY_HOME", ImGuiKey_Home)
        .value("KEY_END", ImGuiKey_End)
        .value("KEY_INSERT", ImGuiKey_Insert)
        .value("KEY_DELETE", ImGuiKey_Delete)
        .value("KEY_BACKSPACE", ImGuiKey_Backspace)
        .value("KEY_SPACE", ImGuiKey_Space)
        .value("KEY_ENTER", ImGuiKey_Enter)
        .value("KEY_ESCAPE", ImGuiKey_Escape)
        .value("KEY_A", ImGuiKey_A)
        .value("KEY_C", ImGuiKey_C)
        .value("KEY_V", ImGuiKey_V)
        .value("KEY_X", ImGuiKey_X)
        .value("KEY_Y", ImGuiKey_Y)
        .value("KEY_Z", ImGuiKey_Z)
        .value("KEY_COUNT", ImGuiKey_COUNT)
        .export_values();

    py::enum_<ImGuiNavInput>(deargui, "NavInput", py::is_arithmetic())
        .value("NAV_INPUT_ACTIVATE", ImGuiNavInput_Activate)
        .value("NAV_INPUT_CANCEL", ImGuiNavInput_Cancel)
        .value("NAV_INPUT_INPUT", ImGuiNavInput_Input)
        .value("NAV_INPUT_MENU", ImGuiNavInput_Menu)
        .value("NAV_INPUT_DPAD_LEFT", ImGuiNavInput_DpadLeft)
        .value("NAV_INPUT_DPAD_RIGHT", ImGuiNavInput_DpadRight)
        .value("NAV_INPUT_DPAD_UP", ImGuiNavInput_DpadUp)
        .value("NAV_INPUT_DPAD_DOWN", ImGuiNavInput_DpadDown)
        .value("NAV_INPUT_L_STICK_LEFT", ImGuiNavInput_LStickLeft)
        .value("NAV_INPUT_L_STICK_RIGHT", ImGuiNavInput_LStickRight)
        .value("NAV_INPUT_L_STICK_UP", ImGuiNavInput_LStickUp)
        .value("NAV_INPUT_L_STICK_DOWN", ImGuiNavInput_LStickDown)
        .value("NAV_INPUT_FOCUS_PREV", ImGuiNavInput_FocusPrev)
        .value("NAV_INPUT_FOCUS_NEXT", ImGuiNavInput_FocusNext)
        .value("NAV_INPUT_TWEAK_SLOW", ImGuiNavInput_TweakSlow)
        .value("NAV_INPUT_TWEAK_FAST", ImGuiNavInput_TweakFast)
        .value("NAV_INPUT_COUNT", ImGuiNavInput_COUNT)
        .export_values();

    py::enum_<ImGuiConfigFlags_>(deargui, "ConfigFlags", py::is_arithmetic())
        .value("CONFIG_FLAGS_NAV_ENABLE_KEYBOARD", ImGuiConfigFlags_NavEnableKeyboard)
        .value("CONFIG_FLAGS_NAV_ENABLE_GAMEPAD", ImGuiConfigFlags_NavEnableGamepad)
        .value("CONFIG_FLAGS_NAV_ENABLE_SET_MOUSE_POS", ImGuiConfigFlags_NavEnableSetMousePos)
        .value("CONFIG_FLAGS_NAV_NO_CAPTURE_KEYBOARD", ImGuiConfigFlags_NavNoCaptureKeyboard)
        .value("CONFIG_FLAGS_NO_MOUSE", ImGuiConfigFlags_NoMouse)
        .value("CONFIG_FLAGS_NO_MOUSE_CURSOR_CHANGE", ImGuiConfigFlags_NoMouseCursorChange)
        .value("CONFIG_FLAGS_IS_SRGB", ImGuiConfigFlags_IsSRGB)
        .value("CONFIG_FLAGS_IS_TOUCH_SCREEN", ImGuiConfigFlags_IsTouchScreen)
        .export_values();

    py::enum_<ImGuiBackendFlags_>(deargui, "BackendFlags", py::is_arithmetic())
        .value("BACKEND_FLAGS_HAS_GAMEPAD", ImGuiBackendFlags_HasGamepad)
        .value("BACKEND_FLAGS_HAS_MOUSE_CURSORS", ImGuiBackendFlags_HasMouseCursors)
        .value("BACKEND_FLAGS_HAS_SET_MOUSE_POS", ImGuiBackendFlags_HasSetMousePos)
        .export_values();

    py::enum_<ImGuiCol_>(deargui, "Col", py::is_arithmetic())
        .value("COL_TEXT", ImGuiCol_Text)
        .value("COL_TEXT_DISABLED", ImGuiCol_TextDisabled)
        .value("COL_WINDOW_BG", ImGuiCol_WindowBg)
        .value("COL_CHILD_BG", ImGuiCol_ChildBg)
        .value("COL_POPUP_BG", ImGuiCol_PopupBg)
        .value("COL_BORDER", ImGuiCol_Border)
        .value("COL_BORDER_SHADOW", ImGuiCol_BorderShadow)
        .value("COL_FRAME_BG", ImGuiCol_FrameBg)
        .value("COL_FRAME_BG_HOVERED", ImGuiCol_FrameBgHovered)
        .value("COL_FRAME_BG_ACTIVE", ImGuiCol_FrameBgActive)
        .value("COL_TITLE_BG", ImGuiCol_TitleBg)
        .value("COL_TITLE_BG_ACTIVE", ImGuiCol_TitleBgActive)
        .value("COL_TITLE_BG_COLLAPSED", ImGuiCol_TitleBgCollapsed)
        .value("COL_MENU_BAR_BG", ImGuiCol_MenuBarBg)
        .value("COL_SCROLLBAR_BG", ImGuiCol_ScrollbarBg)
        .value("COL_SCROLLBAR_GRAB", ImGuiCol_ScrollbarGrab)
        .value("COL_SCROLLBAR_GRAB_HOVERED", ImGuiCol_ScrollbarGrabHovered)
        .value("COL_SCROLLBAR_GRAB_ACTIVE", ImGuiCol_ScrollbarGrabActive)
        .value("COL_CHECK_MARK", ImGuiCol_CheckMark)
        .value("COL_SLIDER_GRAB", ImGuiCol_SliderGrab)
        .value("COL_SLIDER_GRAB_ACTIVE", ImGuiCol_SliderGrabActive)
        .value("COL_BUTTON", ImGuiCol_Button)
        .value("COL_BUTTON_HOVERED", ImGuiCol_ButtonHovered)
        .value("COL_BUTTON_ACTIVE", ImGuiCol_ButtonActive)
        .value("COL_HEADER", ImGuiCol_Header)
        .value("COL_HEADER_HOVERED", ImGuiCol_HeaderHovered)
        .value("COL_HEADER_ACTIVE", ImGuiCol_HeaderActive)
        .value("COL_SEPARATOR", ImGuiCol_Separator)
        .value("COL_SEPARATOR_HOVERED", ImGuiCol_SeparatorHovered)
        .value("COL_SEPARATOR_ACTIVE", ImGuiCol_SeparatorActive)
        .value("COL_RESIZE_GRIP", ImGuiCol_ResizeGrip)
        .value("COL_RESIZE_GRIP_HOVERED", ImGuiCol_ResizeGripHovered)
        .value("COL_RESIZE_GRIP_ACTIVE", ImGuiCol_ResizeGripActive)
        .value("COL_PLOT_LINES", ImGuiCol_PlotLines)
        .value("COL_PLOT_LINES_HOVERED", ImGuiCol_PlotLinesHovered)
        .value("COL_PLOT_HISTOGRAM", ImGuiCol_PlotHistogram)
        .value("COL_PLOT_HISTOGRAM_HOVERED", ImGuiCol_PlotHistogramHovered)
        .value("COL_TEXT_SELECTED_BG", ImGuiCol_TextSelectedBg)
        .value("COL_DRAG_DROP_TARGET", ImGuiCol_DragDropTarget)
        .value("COL_NAV_HIGHLIGHT", ImGuiCol_NavHighlight)
        .value("COL_NAV_WINDOWING_HIGHLIGHT", ImGuiCol_NavWindowingHighlight)
        .value("COL_NAV_WINDOWING_DIM_BG", ImGuiCol_NavWindowingDimBg)
        .value("COL_MODAL_WINDOW_DIM_BG", ImGuiCol_ModalWindowDimBg)
        .value("COL_COUNT", ImGuiCol_COUNT)
        .export_values();

    py::enum_<ImGuiStyleVar_>(deargui, "StyleVar", py::is_arithmetic())
        .value("STYLE_VAR_ALPHA", ImGuiStyleVar_Alpha)
        .value("STYLE_VAR_WINDOW_PADDING", ImGuiStyleVar_WindowPadding)
        .value("STYLE_VAR_WINDOW_ROUNDING", ImGuiStyleVar_WindowRounding)
        .value("STYLE_VAR_WINDOW_BORDER_SIZE", ImGuiStyleVar_WindowBorderSize)
        .value("STYLE_VAR_WINDOW_MIN_SIZE", ImGuiStyleVar_WindowMinSize)
        .value("STYLE_VAR_WINDOW_TITLE_ALIGN", ImGuiStyleVar_WindowTitleAlign)
        .value("STYLE_VAR_CHILD_ROUNDING", ImGuiStyleVar_ChildRounding)
        .value("STYLE_VAR_CHILD_BORDER_SIZE", ImGuiStyleVar_ChildBorderSize)
        .value("STYLE_VAR_POPUP_ROUNDING", ImGuiStyleVar_PopupRounding)
        .value("STYLE_VAR_POPUP_BORDER_SIZE", ImGuiStyleVar_PopupBorderSize)
        .value("STYLE_VAR_FRAME_PADDING", ImGuiStyleVar_FramePadding)
        .value("STYLE_VAR_FRAME_ROUNDING", ImGuiStyleVar_FrameRounding)
        .value("STYLE_VAR_FRAME_BORDER_SIZE", ImGuiStyleVar_FrameBorderSize)
        .value("STYLE_VAR_ITEM_SPACING", ImGuiStyleVar_ItemSpacing)
        .value("STYLE_VAR_ITEM_INNER_SPACING", ImGuiStyleVar_ItemInnerSpacing)
        .value("STYLE_VAR_INDENT_SPACING", ImGuiStyleVar_IndentSpacing)
        .value("STYLE_VAR_SCROLLBAR_SIZE", ImGuiStyleVar_ScrollbarSize)
        .value("STYLE_VAR_SCROLLBAR_ROUNDING", ImGuiStyleVar_ScrollbarRounding)
        .value("STYLE_VAR_GRAB_MIN_SIZE", ImGuiStyleVar_GrabMinSize)
        .value("STYLE_VAR_GRAB_ROUNDING", ImGuiStyleVar_GrabRounding)
        .value("STYLE_VAR_BUTTON_TEXT_ALIGN", ImGuiStyleVar_ButtonTextAlign)
        .value("STYLE_VAR_COUNT", ImGuiStyleVar_COUNT)
        .export_values();

    py::enum_<ImGuiColorEditFlags_>(deargui, "ColorEditFlags", py::is_arithmetic())
        .value("COLOR_EDIT_FLAGS_NONE", ImGuiColorEditFlags_None)
        .value("COLOR_EDIT_FLAGS_NO_ALPHA", ImGuiColorEditFlags_NoAlpha)
        .value("COLOR_EDIT_FLAGS_NO_PICKER", ImGuiColorEditFlags_NoPicker)
        .value("COLOR_EDIT_FLAGS_NO_OPTIONS", ImGuiColorEditFlags_NoOptions)
        .value("COLOR_EDIT_FLAGS_NO_SMALL_PREVIEW", ImGuiColorEditFlags_NoSmallPreview)
        .value("COLOR_EDIT_FLAGS_NO_INPUTS", ImGuiColorEditFlags_NoInputs)
        .value("COLOR_EDIT_FLAGS_NO_TOOLTIP", ImGuiColorEditFlags_NoTooltip)
        .value("COLOR_EDIT_FLAGS_NO_LABEL", ImGuiColorEditFlags_NoLabel)
        .value("COLOR_EDIT_FLAGS_NO_SIDE_PREVIEW", ImGuiColorEditFlags_NoSidePreview)
        .value("COLOR_EDIT_FLAGS_NO_DRAG_DROP", ImGuiColorEditFlags_NoDragDrop)
        .value("COLOR_EDIT_FLAGS_ALPHA_BAR", ImGuiColorEditFlags_AlphaBar)
        .value("COLOR_EDIT_FLAGS_ALPHA_PREVIEW", ImGuiColorEditFlags_AlphaPreview)
        .value("COLOR_EDIT_FLAGS_ALPHA_PREVIEW_HALF", ImGuiColorEditFlags_AlphaPreviewHalf)
        .value("COLOR_EDIT_FLAGS_HDR", ImGuiColorEditFlags_HDR)
        .value("COLOR_EDIT_FLAGS_DISPLAY_RGB", ImGuiColorEditFlags_DisplayRGB)
        .value("COLOR_EDIT_FLAGS_DISPLAY_HSV", ImGuiColorEditFlags_DisplayHSV)
        .value("COLOR_EDIT_FLAGS_DISPLAY_HEX", ImGuiColorEditFlags_DisplayHex)
        .value("COLOR_EDIT_FLAGS_UINT8", ImGuiColorEditFlags_Uint8)
        .value("COLOR_EDIT_FLAGS_FLOAT", ImGuiColorEditFlags_Float)
        .value("COLOR_EDIT_FLAGS_PICKER_HUE_BAR", ImGuiColorEditFlags_PickerHueBar)
        .value("COLOR_EDIT_FLAGS_PICKER_HUE_WHEEL", ImGuiColorEditFlags_PickerHueWheel)
        .export_values();

    py::enum_<ImGuiMouseCursor_>(deargui, "MouseCursor", py::is_arithmetic())
        .value("MOUSE_CURSOR_NONE", ImGuiMouseCursor_None)
        .value("MOUSE_CURSOR_ARROW", ImGuiMouseCursor_Arrow)
        .value("MOUSE_CURSOR_TEXT_INPUT", ImGuiMouseCursor_TextInput)
        .value("MOUSE_CURSOR_RESIZE_ALL", ImGuiMouseCursor_ResizeAll)
        .value("MOUSE_CURSOR_RESIZE_NS", ImGuiMouseCursor_ResizeNS)
        .value("MOUSE_CURSOR_RESIZE_EW", ImGuiMouseCursor_ResizeEW)
        .value("MOUSE_CURSOR_RESIZE_NESW", ImGuiMouseCursor_ResizeNESW)
        .value("MOUSE_CURSOR_RESIZE_NWSE", ImGuiMouseCursor_ResizeNWSE)
        .value("MOUSE_CURSOR_HAND", ImGuiMouseCursor_Hand)
        .value("MOUSE_CURSOR_COUNT", ImGuiMouseCursor_COUNT)
        .export_values();

    py::enum_<ImGuiCond_>(deargui, "Cond", py::is_arithmetic())
        .value("COND_ALWAYS", ImGuiCond_Always)
        .value("COND_ONCE", ImGuiCond_Once)
        .value("COND_FIRST_USE_EVER", ImGuiCond_FirstUseEver)
        .value("COND_APPEARING", ImGuiCond_Appearing)
        .export_values();

    py::class_<ImGuiStyle> Style(deargui, "Style");
    Style.def_rw("alpha", &ImGuiStyle::Alpha);
    Style.def_rw("window_padding", &ImGuiStyle::WindowPadding);
    Style.def_rw("window_rounding", &ImGuiStyle::WindowRounding);
    Style.def_rw("window_border_size", &ImGuiStyle::WindowBorderSize);
    Style.def_rw("window_min_size", &ImGuiStyle::WindowMinSize);
    Style.def_rw("window_title_align", &ImGuiStyle::WindowTitleAlign);
    Style.def_rw("child_rounding", &ImGuiStyle::ChildRounding);
    Style.def_rw("child_border_size", &ImGuiStyle::ChildBorderSize);
    Style.def_rw("popup_rounding", &ImGuiStyle::PopupRounding);
    Style.def_rw("popup_border_size", &ImGuiStyle::PopupBorderSize);
    Style.def_rw("frame_padding", &ImGuiStyle::FramePadding);
    Style.def_rw("frame_rounding", &ImGuiStyle::FrameRounding);
    Style.def_rw("frame_border_size", &ImGuiStyle::FrameBorderSize);
    Style.def_rw("item_spacing", &ImGuiStyle::ItemSpacing);
    Style.def_rw("item_inner_spacing", &ImGuiStyle::ItemInnerSpacing);
    Style.def_rw("touch_extra_padding", &ImGuiStyle::TouchExtraPadding);
    Style.def_rw("indent_spacing", &ImGuiStyle::IndentSpacing);
    Style.def_rw("columns_min_spacing", &ImGuiStyle::ColumnsMinSpacing);
    Style.def_rw("scrollbar_size", &ImGuiStyle::ScrollbarSize);
    Style.def_rw("scrollbar_rounding", &ImGuiStyle::ScrollbarRounding);
    Style.def_rw("grab_min_size", &ImGuiStyle::GrabMinSize);
    Style.def_rw("grab_rounding", &ImGuiStyle::GrabRounding);
    Style.def_rw("button_text_align", &ImGuiStyle::ButtonTextAlign);
    Style.def_rw("display_window_padding", &ImGuiStyle::DisplayWindowPadding);
    Style.def_rw("display_safe_area_padding", &ImGuiStyle::DisplaySafeAreaPadding);
    Style.def_rw("mouse_cursor_scale", &ImGuiStyle::MouseCursorScale);
    Style.def_rw("anti_aliased_lines", &ImGuiStyle::AntiAliasedLines);
    Style.def_rw("anti_aliased_fill", &ImGuiStyle::AntiAliasedFill);
    Style.def_rw("curve_tessellation_tol", &ImGuiStyle::CurveTessellationTol);
    Style.def_ro("colors", &ImGuiStyle::Colors);
    Style.def(py::init<>());
    Style.def("scale_all_sizes", &ImGuiStyle::ScaleAllSizes
    , py::arg("scale_factor")
    , py::rv_policy::automatic_reference);
    py::class_<ImGuiIO> IO(deargui, "IO");
    IO.def_rw("config_flags", &ImGuiIO::ConfigFlags);
    IO.def_rw("backend_flags", &ImGuiIO::BackendFlags);
    IO.def_rw("display_size", &ImGuiIO::DisplaySize);
    IO.def_rw("delta_time", &ImGuiIO::DeltaTime);
    IO.def_rw("ini_saving_rate", &ImGuiIO::IniSavingRate);
    IO.def_rw("ini_filename", &ImGuiIO::IniFilename);
    IO.def_rw("log_filename", &ImGuiIO::LogFilename);
    IO.def_rw("mouse_double_click_time", &ImGuiIO::MouseDoubleClickTime);
    IO.def_rw("mouse_double_click_max_dist", &ImGuiIO::MouseDoubleClickMaxDist);
    IO.def_rw("mouse_drag_threshold", &ImGuiIO::MouseDragThreshold);
//    IO.def_ro("key_map", &ImGuiIO::KeyMap);
    IO.def_rw("key_repeat_delay", &ImGuiIO::KeyRepeatDelay);
    IO.def_rw("key_repeat_rate", &ImGuiIO::KeyRepeatRate);
    IO.def_rw("user_data", &ImGuiIO::UserData);
    IO.def_rw("fonts", &ImGuiIO::Fonts);
    IO.def_rw("font_global_scale", &ImGuiIO::FontGlobalScale);
    IO.def_rw("font_allow_user_scaling", &ImGuiIO::FontAllowUserScaling);
    IO.def_rw("font_default", &ImGuiIO::FontDefault);
    IO.def_rw("display_framebuffer_scale", &ImGuiIO::DisplayFramebufferScale);
    IO.def_rw("mouse_draw_cursor", &ImGuiIO::MouseDrawCursor);
    IO.def_rw("config_mac_osx_behaviors", &ImGuiIO::ConfigMacOSXBehaviors);
    IO.def_rw("config_input_text_cursor_blink", &ImGuiIO::ConfigInputTextCursorBlink);
    IO.def_rw("clipboard_user_data", &ImGuiIO::ClipboardUserData);
    IO.def_rw("mouse_pos", &ImGuiIO::MousePos);
    IO.def_rw("mouse_wheel", &ImGuiIO::MouseWheel);
    IO.def_rw("mouse_wheel_h", &ImGuiIO::MouseWheelH);
    IO.def_rw("key_ctrl", &ImGuiIO::KeyCtrl);
    IO.def_rw("key_shift", &ImGuiIO::KeyShift);
    IO.def_rw("key_alt", &ImGuiIO::KeyAlt);
    IO.def_rw("key_super", &ImGuiIO::KeySuper);
    IO.def("add_input_character", &ImGuiIO::AddInputCharacter
    , py::arg("c")
    , py::rv_policy::automatic_reference);
    IO.def("add_input_characters_utf8", &ImGuiIO::AddInputCharactersUTF8
    , py::arg("utf8_chars")
    , py::rv_policy::automatic_reference);
    IO.def("clear_input_characters", &ImGuiIO::ClearInputCharacters
    , py::rv_policy::automatic_reference);
    IO.def_rw("want_capture_mouse", &ImGuiIO::WantCaptureMouse);
    IO.def_rw("want_capture_keyboard", &ImGuiIO::WantCaptureKeyboard);
    IO.def_rw("want_text_input", &ImGuiIO::WantTextInput);
    IO.def_rw("want_set_mouse_pos", &ImGuiIO::WantSetMousePos);
    IO.def_rw("want_save_ini_settings", &ImGuiIO::WantSaveIniSettings);
    IO.def_rw("nav_active", &ImGuiIO::NavActive);
    IO.def_rw("nav_visible", &ImGuiIO::NavVisible);
    IO.def_rw("framerate", &ImGuiIO::Framerate);
    IO.def_rw("metrics_render_vertices", &ImGuiIO::MetricsRenderVertices);
    IO.def_rw("metrics_render_indices", &ImGuiIO::MetricsRenderIndices);
    IO.def_rw("metrics_render_windows", &ImGuiIO::MetricsRenderWindows);
    IO.def_rw("metrics_active_windows", &ImGuiIO::MetricsActiveWindows);
    IO.def_rw("metrics_active_allocations", &ImGuiIO::MetricsActiveAllocations);
    IO.def_rw("mouse_delta", &ImGuiIO::MouseDelta);
    IO.def_rw("mouse_pos_prev", &ImGuiIO::MousePosPrev);
    IO.def_ro("mouse_clicked_pos", &ImGuiIO::MouseClickedPos);
//    IO.def_ro("mouse_clicked_time", &ImGuiIO::MouseClickedTime);
    IO.def_ro("mouse_clicked", &ImGuiIO::MouseClicked);
    IO.def_ro("mouse_double_clicked", &ImGuiIO::MouseDoubleClicked);
    IO.def_ro("mouse_released", &ImGuiIO::MouseReleased);
    IO.def_ro("mouse_down_owned", &ImGuiIO::MouseDownOwned);
//    IO.def_ro("mouse_down_duration", &ImGuiIO::MouseDownDuration);
//    IO.def_ro("mouse_down_duration_prev", &ImGuiIO::MouseDownDurationPrev);
    IO.def_ro("mouse_drag_max_distance_abs", &ImGuiIO::MouseDragMaxDistanceAbs);
//    IO.def_ro("mouse_drag_max_distance_sqr", &ImGuiIO::MouseDragMaxDistanceSqr);
    IO.def(py::init<>());
    py::class_<ImGuiOnceUponAFrame> OnceUponAFrame(deargui, "OnceUponAFrame");
    OnceUponAFrame.def(py::init<>());
    OnceUponAFrame.def_rw("ref_frame", &ImGuiOnceUponAFrame::RefFrame);
    py::class_<ImGuiTextFilter> TextFilter(deargui, "TextFilter");
    TextFilter.def(py::init<const char *>()
    , py::arg("default_filter") = nullptr
    );
    TextFilter.def("draw", &ImGuiTextFilter::Draw
    , py::arg("label") = nullptr
    , py::arg("width") = 0.0f
    , py::rv_policy::automatic_reference);
    TextFilter.def("pass_filter", &ImGuiTextFilter::PassFilter
    , py::arg("text")
    , py::arg("text_end") = nullptr
    , py::rv_policy::automatic_reference);
    TextFilter.def("build", &ImGuiTextFilter::Build
    , py::rv_policy::automatic_reference);
    TextFilter.def("clear", &ImGuiTextFilter::Clear
    , py::rv_policy::automatic_reference);
    TextFilter.def("is_active", &ImGuiTextFilter::IsActive
    , py::rv_policy::automatic_reference);
    TextFilter.def_ro("input_buf", &ImGuiTextFilter::InputBuf);
    TextFilter.def_rw("count_grep", &ImGuiTextFilter::CountGrep);
    py::class_<ImGuiStorage> Storage(deargui, "Storage");
    Storage.def("clear", &ImGuiStorage::Clear
    , py::rv_policy::automatic_reference);
    Storage.def("get_int", &ImGuiStorage::GetInt
    , py::arg("key")
    , py::arg("default_val") = 0
    , py::rv_policy::automatic_reference);
    Storage.def("set_int", &ImGuiStorage::SetInt
    , py::arg("key")
    , py::arg("val")
    , py::rv_policy::automatic_reference);
    Storage.def("get_bool", &ImGuiStorage::GetBool
    , py::arg("key")
    , py::arg("default_val") = false
    , py::rv_policy::automatic_reference);
    Storage.def("set_bool", &ImGuiStorage::SetBool
    , py::arg("key")
    , py::arg("val")
    , py::rv_policy::automatic_reference);
    Storage.def("get_float", &ImGuiStorage::GetFloat
    , py::arg("key")
    , py::arg("default_val") = 0.0f
    , py::rv_policy::automatic_reference);
    Storage.def("set_float", &ImGuiStorage::SetFloat
    , py::arg("key")
    , py::arg("val")
    , py::rv_policy::automatic_reference);
    Storage.def("get_void_ptr", &ImGuiStorage::GetVoidPtr
    , py::arg("key")
    , py::rv_policy::automatic_reference);
    Storage.def("set_void_ptr", &ImGuiStorage::SetVoidPtr
    , py::arg("key")
    , py::arg("val")
    , py::rv_policy::automatic_reference);
    Storage.def("get_int_ref", &ImGuiStorage::GetIntRef
    , py::arg("key")
    , py::arg("default_val") = 0
    , py::rv_policy::automatic_reference);
    Storage.def("get_bool_ref", &ImGuiStorage::GetBoolRef
    , py::arg("key")
    , py::arg("default_val") = false
    , py::rv_policy::automatic_reference);
    Storage.def("get_float_ref", &ImGuiStorage::GetFloatRef
    , py::arg("key")
    , py::arg("default_val") = 0.0f
    , py::rv_policy::automatic_reference);
    Storage.def("get_void_ptr_ref", &ImGuiStorage::GetVoidPtrRef
    , py::arg("key")
    , py::arg("default_val") = nullptr
    , py::rv_policy::automatic_reference);
    Storage.def("set_all_int", &ImGuiStorage::SetAllInt
    , py::arg("val")
    , py::rv_policy::automatic_reference);
    Storage.def("build_sort_by_key", &ImGuiStorage::BuildSortByKey
    , py::rv_policy::automatic_reference);
    py::class_<ImGuiInputTextCallbackData> InputTextCallbackData(deargui, "InputTextCallbackData");
    InputTextCallbackData.def_rw("event_flag", &ImGuiInputTextCallbackData::EventFlag);
    InputTextCallbackData.def_rw("flags", &ImGuiInputTextCallbackData::Flags);
    InputTextCallbackData.def_rw("user_data", &ImGuiInputTextCallbackData::UserData);
    InputTextCallbackData.def_rw("event_char", &ImGuiInputTextCallbackData::EventChar);
    InputTextCallbackData.def_rw("event_key", &ImGuiInputTextCallbackData::EventKey);
    InputTextCallbackData.def_ro("buf", &ImGuiInputTextCallbackData::Buf);
    InputTextCallbackData.def_rw("buf_text_len", &ImGuiInputTextCallbackData::BufTextLen);
    InputTextCallbackData.def_rw("buf_size", &ImGuiInputTextCallbackData::BufSize);
    InputTextCallbackData.def_rw("buf_dirty", &ImGuiInputTextCallbackData::BufDirty);
    InputTextCallbackData.def_rw("cursor_pos", &ImGuiInputTextCallbackData::CursorPos);
    InputTextCallbackData.def_rw("selection_start", &ImGuiInputTextCallbackData::SelectionStart);
    InputTextCallbackData.def_rw("selection_end", &ImGuiInputTextCallbackData::SelectionEnd);
    InputTextCallbackData.def(py::init<>());
    InputTextCallbackData.def("delete_chars", &ImGuiInputTextCallbackData::DeleteChars
    , py::arg("pos")
    , py::arg("bytes_count")
    , py::rv_policy::automatic_reference);
    InputTextCallbackData.def("insert_chars", &ImGuiInputTextCallbackData::InsertChars
    , py::arg("pos")
    , py::arg("text")
    , py::arg("text_end") = nullptr
    , py::rv_policy::automatic_reference);
    InputTextCallbackData.def("has_selection", &ImGuiInputTextCallbackData::HasSelection
    , py::rv_policy::automatic_reference);
    py::class_<ImGuiSizeCallbackData> SizeCallbackData(deargui, "SizeCallbackData");
    SizeCallbackData.def_rw("user_data", &ImGuiSizeCallbackData::UserData);
    SizeCallbackData.def_rw("pos", &ImGuiSizeCallbackData::Pos);
    SizeCallbackData.def_rw("current_size", &ImGuiSizeCallbackData::CurrentSize);
    SizeCallbackData.def_rw("desired_size", &ImGuiSizeCallbackData::DesiredSize);
    py::class_<ImGuiPayload> Payload(deargui, "Payload");
    Payload.def_rw("data", &ImGuiPayload::Data);
    Payload.def_rw("data_size", &ImGuiPayload::DataSize);
    Payload.def_rw("source_id", &ImGuiPayload::SourceId);
    Payload.def_rw("source_parent_id", &ImGuiPayload::SourceParentId);
    Payload.def_rw("data_frame_count", &ImGuiPayload::DataFrameCount);
    Payload.def_ro("data_type", &ImGuiPayload::DataType);
    Payload.def_rw("preview", &ImGuiPayload::Preview);
    Payload.def_rw("delivery", &ImGuiPayload::Delivery);
    Payload.def(py::init<>());
    Payload.def("clear", &ImGuiPayload::Clear
    , py::rv_policy::automatic_reference);
    Payload.def("is_data_type", &ImGuiPayload::IsDataType
    , py::arg("type")
    , py::rv_policy::automatic_reference);
    Payload.def("is_preview", &ImGuiPayload::IsPreview
    , py::rv_policy::automatic_reference);
    Payload.def("is_delivery", &ImGuiPayload::IsDelivery
    , py::rv_policy::automatic_reference);
    py::class_<ImColor> Color(deargui, "Color");
    Color.def_rw("value", &ImColor::Value);
    Color.def(py::init<>());
    Color.def(py::init<int, int, int, int>()
    , py::arg("r")
    , py::arg("g")
    , py::arg("b")
    , py::arg("a") = 255
    );
    Color.def(py::init<ImU32>()
    , py::arg("rgba")
    );
    Color.def(py::init<float, float, float, float>()
    , py::arg("r")
    , py::arg("g")
    , py::arg("b")
    , py::arg("a") = 1.0f
    );
    Color.def(py::init<const ImVec4 &>()
    , py::arg("col")
    );
    Color.def("set_hsv", &ImColor::SetHSV
    , py::arg("h")
    , py::arg("s")
    , py::arg("v")
    , py::arg("a") = 1.0f
    , py::rv_policy::automatic_reference);
    py::class_<ImGuiListClipper> ListClipper(deargui, "ListClipper");
    ListClipper.def_rw("start_pos_y", &ImGuiListClipper::StartPosY);
    ListClipper.def_rw("items_height", &ImGuiListClipper::ItemsHeight);
    ListClipper.def_rw("items_count", &ImGuiListClipper::ItemsCount);
    ListClipper.def_rw("display_start", &ImGuiListClipper::DisplayStart);
    ListClipper.def_rw("display_end", &ImGuiListClipper::DisplayEnd);
    ListClipper.def("step", &ImGuiListClipper::Step
    , py::rv_policy::automatic_reference);
    ListClipper.def("begin", &ImGuiListClipper::Begin
    , py::arg("items_count")
    , py::arg("items_height") = -1.0f
    , py::rv_policy::automatic_reference);
    ListClipper.def("end", &ImGuiListClipper::End
    , py::rv_policy::automatic_reference);
    py::class_<ImDrawCmd> DrawCmd(deargui, "DrawCmd");
    DrawCmd.def_rw("elem_count", &ImDrawCmd::ElemCount);
    DrawCmd.def_rw("clip_rect", &ImDrawCmd::ClipRect);
    DrawCmd.def_rw("texture_id", &ImDrawCmd::TextureId);
    DrawCmd.def_rw("user_callback_data", &ImDrawCmd::UserCallbackData);
    DrawCmd.def(py::init<>());
    py::class_<ImDrawVert> DrawVert(deargui, "DrawVert");
    DrawVert.def_rw("pos", &ImDrawVert::pos);
    DrawVert.def_rw("uv", &ImDrawVert::uv);
    DrawVert.def_rw("col", &ImDrawVert::col);
    py::class_<ImDrawChannel> DrawChannel(deargui, "DrawChannel");
    DrawChannel.def_rw("cmd_buffer", &ImDrawChannel::_CmdBuffer);
    DrawChannel.def_rw("idx_buffer", &ImDrawChannel::_IdxBuffer);

    py::enum_<ImDrawListFlags_>(deargui, "DrawListFlags", py::is_arithmetic())
        .value("DRAW_LIST_FLAGS_ANTI_ALIASED_LINES", ImDrawListFlags_AntiAliasedLines)
        .value("DRAW_LIST_FLAGS_ANTI_ALIASED_FILL", ImDrawListFlags_AntiAliasedFill)
        .export_values();

    py::class_<ImDrawList> DrawList(deargui, "DrawList");
    DrawList.def_rw("cmd_buffer", &ImDrawList::CmdBuffer);
    DrawList.def_rw("idx_buffer", &ImDrawList::IdxBuffer);
    DrawList.def_rw("vtx_buffer", &ImDrawList::VtxBuffer);
    DrawList.def_rw("flags", &ImDrawList::Flags);
//    DrawList.def(py::init<ImDrawListSharedData *>()
//    , py::arg("shared_data")
//    );
    DrawList.def("push_clip_rect", &ImDrawList::PushClipRect
    , py::arg("clip_rect_min")
    , py::arg("clip_rect_max")
    , py::arg("intersect_with_current_clip_rect") = false
    , py::rv_policy::automatic_reference);
    DrawList.def("push_clip_rect_full_screen", &ImDrawList::PushClipRectFullScreen
    , py::rv_policy::automatic_reference);
    DrawList.def("pop_clip_rect", &ImDrawList::PopClipRect
    , py::rv_policy::automatic_reference);
    DrawList.def("push_texture_id", &ImDrawList::PushTextureID
    , py::arg("texture_id")
    , py::rv_policy::automatic_reference);
    DrawList.def("pop_texture_id", &ImDrawList::PopTextureID
    , py::rv_policy::automatic_reference);
    DrawList.def("get_clip_rect_min", &ImDrawList::GetClipRectMin
    , py::rv_policy::automatic_reference);
    DrawList.def("get_clip_rect_max", &ImDrawList::GetClipRectMax
    , py::rv_policy::automatic_reference);
    DrawList.def("add_line", &ImDrawList::AddLine
    , py::arg("a")
    , py::arg("b")
    , py::arg("col")
    , py::arg("thickness") = 1.0f
    , py::rv_policy::automatic_reference);
    DrawList.def("add_rect", &ImDrawList::AddRect
    , py::arg("a")
    , py::arg("b")
    , py::arg("col")
    , py::arg("rounding") = 0.0f
    , py::arg("flags") = (int)ImDrawFlags_::ImDrawFlags_RoundCornersAll
    , py::arg("thickness") = 1.0f
    , py::rv_policy::automatic_reference);
    DrawList.def("add_rect_filled", &ImDrawList::AddRectFilled
    , py::arg("a")
    , py::arg("b")
    , py::arg("col")
    , py::arg("rounding") = 0.0f
    , py::arg("flags") = (int)ImDrawFlags_::ImDrawFlags_RoundCornersAll
    , py::rv_policy::automatic_reference);
    DrawList.def("add_rect_filled_multi_color", &ImDrawList::AddRectFilledMultiColor
    , py::arg("a")
    , py::arg("b")
    , py::arg("col_upr_left")
    , py::arg("col_upr_right")
    , py::arg("col_bot_right")
    , py::arg("col_bot_left")
    , py::rv_policy::automatic_reference);
    DrawList.def("add_quad", &ImDrawList::AddQuad
    , py::arg("a")
    , py::arg("b")
    , py::arg("c")
    , py::arg("d")
    , py::arg("col")
    , py::arg("thickness") = 1.0f
    , py::rv_policy::automatic_reference);
    DrawList.def("add_quad_filled", &ImDrawList::AddQuadFilled
    , py::arg("a")
    , py::arg("b")
    , py::arg("c")
    , py::arg("d")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    DrawList.def("add_triangle", &ImDrawList::AddTriangle
    , py::arg("a")
    , py::arg("b")
    , py::arg("c")
    , py::arg("col")
    , py::arg("thickness") = 1.0f
    , py::rv_policy::automatic_reference);
    DrawList.def("add_triangle_filled", &ImDrawList::AddTriangleFilled
    , py::arg("a")
    , py::arg("b")
    , py::arg("c")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    DrawList.def("add_circle", &ImDrawList::AddCircle
    , py::arg("centre")
    , py::arg("radius")
    , py::arg("col")
    , py::arg("num_segments") = 12
    , py::arg("thickness") = 1.0f
    , py::rv_policy::automatic_reference);
    DrawList.def("add_circle_filled", &ImDrawList::AddCircleFilled
    , py::arg("centre")
    , py::arg("radius")
    , py::arg("col")
    , py::arg("num_segments") = 12
    , py::rv_policy::automatic_reference);
    DrawList.def("add_text", py::overload_cast<const ImVec2 &, ImU32, const char *, const char *>(&ImDrawList::AddText)
    , py::arg("pos")
    , py::arg("col")
    , py::arg("text_begin")
    , py::arg("text_end") = nullptr
    , py::rv_policy::automatic_reference);
    DrawList.def("add_text", py::overload_cast<const ImFont *, float, const ImVec2 &, ImU32, const char *, const char *, float, const ImVec4 *>(&ImDrawList::AddText)
    , py::arg("font")
    , py::arg("font_size")
    , py::arg("pos")
    , py::arg("col")
    , py::arg("text_begin")
    , py::arg("text_end") = nullptr
    , py::arg("wrap_width") = 0.0f
    , py::arg("cpu_fine_clip_rect") = nullptr
    , py::rv_policy::automatic_reference);
    DrawList.def("add_image", &ImDrawList::AddImage
    , py::arg("user_texture_id")
    , py::arg("a")
    , py::arg("b")
    , py::arg("uv_a") = ImVec2(0,0)
    , py::arg("uv_b") = ImVec2(1,1)
    , py::arg("col") = 0xFFFFFFFF
    , py::rv_policy::automatic_reference);
    DrawList.def("add_image_quad", &ImDrawList::AddImageQuad
    , py::arg("user_texture_id")
    , py::arg("a")
    , py::arg("b")
    , py::arg("c")
    , py::arg("d")
    , py::arg("uv_a") = ImVec2(0,0)
    , py::arg("uv_b") = ImVec2(1,0)
    , py::arg("uv_c") = ImVec2(1,1)
    , py::arg("uv_d") = ImVec2(0,1)
    , py::arg("col") = 0xFFFFFFFF
    , py::rv_policy::automatic_reference);
    DrawList.def("add_image_rounded", &ImDrawList::AddImageRounded
    , py::arg("user_texture_id")
    , py::arg("a")
    , py::arg("b")
    , py::arg("uv_a")
    , py::arg("uv_b")
    , py::arg("col")
    , py::arg("rounding")
    , py::arg("flags") = (int)ImDrawFlags_::ImDrawFlags_RoundCornersAll
    , py::rv_policy::automatic_reference);
    DrawList.def("add_polyline", &ImDrawList::AddPolyline
    , py::arg("points")
    , py::arg("num_points")
    , py::arg("col")
    , py::arg("closed")
    , py::arg("thickness")
    , py::rv_policy::automatic_reference);
    DrawList.def("add_convex_poly_filled", &ImDrawList::AddConvexPolyFilled
    , py::arg("points")
    , py::arg("num_points")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    DrawList.def("add_bezier_cubic", &ImDrawList::AddBezierCubic
    , py::arg("p1")
    , py::arg("p2")
    , py::arg("p3")
    , py::arg("p4")
    , py::arg("col")
    , py::arg("thickness")
    , py::arg("num_segments") = 0
    , py::rv_policy::automatic_reference);
    DrawList.def("add_bezier_quadratic", &ImDrawList::AddBezierQuadratic
    , py::arg("p1")
    , py::arg("p2")
    , py::arg("p3")
    , py::arg("col")
    , py::arg("thickness")
    , py::arg("num_segments") = 0
    , py::rv_policy::automatic_reference);
    DrawList.def("path_clear", &ImDrawList::PathClear
    , py::rv_policy::automatic_reference);
    DrawList.def("path_line_to", &ImDrawList::PathLineTo
    , py::arg("pos")
    , py::rv_policy::automatic_reference);
    DrawList.def("path_line_to_merge_duplicate", &ImDrawList::PathLineToMergeDuplicate
    , py::arg("pos")
    , py::rv_policy::automatic_reference);
    DrawList.def("path_fill_convex", &ImDrawList::PathFillConvex
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    DrawList.def("path_stroke", &ImDrawList::PathStroke
    , py::arg("col")
    , py::arg("closed")
    , py::arg("thickness") = 1.0f
    , py::rv_policy::automatic_reference);
    DrawList.def("path_arc_to", &ImDrawList::PathArcTo
    , py::arg("centre")
    , py::arg("radius")
    , py::arg("a_min")
    , py::arg("a_max")
    , py::arg("num_segments") = 10
    , py::rv_policy::automatic_reference);
    DrawList.def("path_arc_to_fast", &ImDrawList::PathArcToFast
    , py::arg("centre")
    , py::arg("radius")
    , py::arg("a_min_of_12")
    , py::arg("a_max_of_12")
    , py::rv_policy::automatic_reference);
    DrawList.def("path_bezier_curve_to", &ImDrawList::PathBezierCubicCurveTo
    , py::arg("p1")
    , py::arg("p2")
    , py::arg("p3")
    , py::arg("num_segments") = 0
    , py::rv_policy::automatic_reference);
    DrawList.def("path_bezier_quadratic_curve_to", &ImDrawList::PathBezierQuadraticCurveTo
    , py::arg("p2")
    , py::arg("p3")
    , py::arg("num_segments") = 0
    , py::rv_policy::automatic_reference);
    DrawList.def("path_rect", &ImDrawList::PathRect
    , py::arg("rect_min")
    , py::arg("rect_max")
    , py::arg("rounding") = 0.0f
    , py::arg("flags") = (int)ImDrawFlags_::ImDrawFlags_RoundCornersAll
    , py::rv_policy::automatic_reference);
    DrawList.def("channels_split", &ImDrawList::ChannelsSplit
    , py::arg("channels_count")
    , py::rv_policy::automatic_reference);
    DrawList.def("channels_merge", &ImDrawList::ChannelsMerge
    , py::rv_policy::automatic_reference);
    DrawList.def("channels_set_current", &ImDrawList::ChannelsSetCurrent
    , py::arg("channel_index")
    , py::rv_policy::automatic_reference);
    DrawList.def("add_draw_cmd", &ImDrawList::AddDrawCmd
    , py::rv_policy::automatic_reference);
    DrawList.def("clone_output", &ImDrawList::CloneOutput
    , py::rv_policy::automatic_reference);
    DrawList.def("prim_reserve", &ImDrawList::PrimReserve
    , py::arg("idx_count")
    , py::arg("vtx_count")
    , py::rv_policy::automatic_reference);
    DrawList.def("prim_rect", &ImDrawList::PrimRect
    , py::arg("a")
    , py::arg("b")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    DrawList.def("prim_rect_uv", &ImDrawList::PrimRectUV
    , py::arg("a")
    , py::arg("b")
    , py::arg("uv_a")
    , py::arg("uv_b")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    DrawList.def("prim_quad_uv", &ImDrawList::PrimQuadUV
    , py::arg("a")
    , py::arg("b")
    , py::arg("c")
    , py::arg("d")
    , py::arg("uv_a")
    , py::arg("uv_b")
    , py::arg("uv_c")
    , py::arg("uv_d")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    DrawList.def("prim_write_vtx", &ImDrawList::PrimWriteVtx
    , py::arg("pos")
    , py::arg("uv")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    DrawList.def("prim_write_idx", &ImDrawList::PrimWriteIdx
    , py::arg("idx")
    , py::rv_policy::automatic_reference);
    DrawList.def("prim_vtx", &ImDrawList::PrimVtx
    , py::arg("pos")
    , py::arg("uv")
    , py::arg("col")
    , py::rv_policy::automatic_reference);
    py::class_<ImDrawData> DrawData(deargui, "DrawData");
    DrawData.def_rw("valid", &ImDrawData::Valid);
    DrawData.def_rw("cmd_lists_count", &ImDrawData::CmdListsCount);
    DrawData.def_rw("total_idx_count", &ImDrawData::TotalIdxCount);
    DrawData.def_rw("total_vtx_count", &ImDrawData::TotalVtxCount);
    DrawData.def_rw("display_pos", &ImDrawData::DisplayPos);
    DrawData.def_rw("display_size", &ImDrawData::DisplaySize);
    DrawData.def(py::init<>());
    DrawData.def("clear", &ImDrawData::Clear
    , py::rv_policy::automatic_reference);
    DrawData.def("de_index_all_buffers", &ImDrawData::DeIndexAllBuffers
    , py::rv_policy::automatic_reference);
    DrawData.def("scale_clip_rects", &ImDrawData::ScaleClipRects
    , py::arg("sc")
    , py::rv_policy::automatic_reference);
    py::class_<ImFontConfig> FontConfig(deargui, "FontConfig");
    FontConfig.def_rw("font_data", &ImFontConfig::FontData);
    FontConfig.def_rw("font_data_size", &ImFontConfig::FontDataSize);
    FontConfig.def_rw("font_data_owned_by_atlas", &ImFontConfig::FontDataOwnedByAtlas);
    FontConfig.def_rw("font_no", &ImFontConfig::FontNo);
    FontConfig.def_rw("size_pixels", &ImFontConfig::SizePixels);
    FontConfig.def_rw("oversample_h", &ImFontConfig::OversampleH);
    FontConfig.def_rw("oversample_v", &ImFontConfig::OversampleV);
    FontConfig.def_rw("pixel_snap_h", &ImFontConfig::PixelSnapH);
    FontConfig.def_rw("glyph_extra_spacing", &ImFontConfig::GlyphExtraSpacing);
    FontConfig.def_rw("glyph_offset", &ImFontConfig::GlyphOffset);
//    FontConfig.def_ro("glyph_ranges", &ImFontConfig::GlyphRanges);
    FontConfig.def_rw("glyph_min_advance_x", &ImFontConfig::GlyphMinAdvanceX);
    FontConfig.def_rw("glyph_max_advance_x", &ImFontConfig::GlyphMaxAdvanceX);
    FontConfig.def_rw("merge_mode", &ImFontConfig::MergeMode);
    FontConfig.def_rw("font_builder_flags", &ImFontConfig::FontBuilderFlags);
    FontConfig.def_rw("rasterizer_multiply", &ImFontConfig::RasterizerMultiply);
    FontConfig.def_ro("name", &ImFontConfig::Name);
    FontConfig.def_rw("dst_font", &ImFontConfig::DstFont);
    FontConfig.def(py::init<>());
    py::class_<ImFontGlyph> FontGlyph(deargui, "FontGlyph");
    FontGlyph.def_rw("advance_x", &ImFontGlyph::AdvanceX);
    FontGlyph.def_rw("x0", &ImFontGlyph::X0);
    FontGlyph.def_rw("y0", &ImFontGlyph::Y0);
    FontGlyph.def_rw("x1", &ImFontGlyph::X1);
    FontGlyph.def_rw("y1", &ImFontGlyph::Y1);
    FontGlyph.def_rw("u0", &ImFontGlyph::U0);
    FontGlyph.def_rw("v0", &ImFontGlyph::V0);
    FontGlyph.def_rw("u1", &ImFontGlyph::U1);
    FontGlyph.def_rw("v1", &ImFontGlyph::V1);
    py::enum_<ImFontAtlasFlags_>(deargui, "FontAtlasFlags", py::is_arithmetic())
        .value("FONT_ATLAS_FLAGS_NONE", ImFontAtlasFlags_None)
        .value("FONT_ATLAS_FLAGS_NO_POWER_OF_TWO_HEIGHT", ImFontAtlasFlags_NoPowerOfTwoHeight)
        .value("FONT_ATLAS_FLAGS_NO_MOUSE_CURSORS", ImFontAtlasFlags_NoMouseCursors)
        .export_values();

    py::class_<ImFontAtlas> FontAtlas(deargui, "FontAtlas");
    FontAtlas.def(py::init<>());
    FontAtlas.def("add_font", &ImFontAtlas::AddFont
    , py::arg("font_cfg")
    , py::rv_policy::automatic_reference);
    FontAtlas.def("add_font_default", &ImFontAtlas::AddFontDefault
    , py::arg("font_cfg") = nullptr
    , py::rv_policy::automatic_reference);
    FontAtlas.def("add_font_from_file_ttf", &ImFontAtlas::AddFontFromFileTTF
    , py::arg("filename")
    , py::arg("size_pixels")
    , py::arg("font_cfg") = nullptr
    , py::arg("glyph_ranges") = nullptr
    , py::rv_policy::automatic_reference);
    FontAtlas.def("add_font_from_memory_ttf", &ImFontAtlas::AddFontFromMemoryTTF
    , py::arg("font_data")
    , py::arg("font_size")
    , py::arg("size_pixels")
    , py::arg("font_cfg") = nullptr
    , py::arg("glyph_ranges") = nullptr
    , py::rv_policy::automatic_reference);
    FontAtlas.def("add_font_from_memory_compressed_ttf", &ImFontAtlas::AddFontFromMemoryCompressedTTF
    , py::arg("compressed_font_data")
    , py::arg("compressed_font_size")
    , py::arg("size_pixels")
    , py::arg("font_cfg") = nullptr
    , py::arg("glyph_ranges") = nullptr
    , py::rv_policy::automatic_reference);
    FontAtlas.def("add_font_from_memory_compressed_base85_ttf", &ImFontAtlas::AddFontFromMemoryCompressedBase85TTF
    , py::arg("compressed_font_data_base85")
    , py::arg("size_pixels")
    , py::arg("font_cfg") = nullptr
    , py::arg("glyph_ranges") = nullptr
    , py::rv_policy::automatic_reference);
    FontAtlas.def("clear_input_data", &ImFontAtlas::ClearInputData
    , py::rv_policy::automatic_reference);
    FontAtlas.def("clear_tex_data", &ImFontAtlas::ClearTexData
    , py::rv_policy::automatic_reference);
    FontAtlas.def("clear_fonts", &ImFontAtlas::ClearFonts
    , py::rv_policy::automatic_reference);
    FontAtlas.def("clear", &ImFontAtlas::Clear
    , py::rv_policy::automatic_reference);
    FontAtlas.def("build", &ImFontAtlas::Build
    , py::rv_policy::automatic_reference);
    FontAtlas.def("is_built", &ImFontAtlas::IsBuilt
    , py::rv_policy::automatic_reference);
    FontAtlas.def("set_tex_id", &ImFontAtlas::SetTexID
    , py::arg("id")
    , py::rv_policy::automatic_reference);
//    FontAtlas.def("get_glyph_ranges_default", &ImFontAtlas::GetGlyphRangesDefault
//    , py::rv_policy::automatic_reference);
//    FontAtlas.def("get_glyph_ranges_korean", &ImFontAtlas::GetGlyphRangesKorean
//    , py::rv_policy::automatic_reference);
//    FontAtlas.def("get_glyph_ranges_japanese", &ImFontAtlas::GetGlyphRangesJapanese
//    , py::rv_policy::automatic_reference);
//    FontAtlas.def("get_glyph_ranges_chinese_full", &ImFontAtlas::GetGlyphRangesChineseFull
//    , py::rv_policy::automatic_reference);
//    FontAtlas.def("get_glyph_ranges_chinese_simplified_common", &ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon
//    , py::rv_policy::automatic_reference);
//    FontAtlas.def("get_glyph_ranges_cyrillic", &ImFontAtlas::GetGlyphRangesCyrillic
//    , py::rv_policy::automatic_reference);
//    FontAtlas.def("get_glyph_ranges_thai", &ImFontAtlas::GetGlyphRangesThai
//    , py::rv_policy::automatic_reference);
    FontAtlas.def_rw("locked", &ImFontAtlas::Locked);
    FontAtlas.def_rw("flags", &ImFontAtlas::Flags);
    FontAtlas.def_rw("tex_id", &ImFontAtlas::TexID);
    FontAtlas.def_rw("tex_desired_width", &ImFontAtlas::TexDesiredWidth);
    FontAtlas.def_rw("tex_glyph_padding", &ImFontAtlas::TexGlyphPadding);
    py::class_<ImFont> Font(deargui, "Font");
    Font.def_rw("font_size", &ImFont::FontSize);
    Font.def_rw("scale", &ImFont::Scale);
    Font.def_rw("glyphs", &ImFont::Glyphs);
    Font.def_rw("index_advance_x", &ImFont::IndexAdvanceX);
    Font.def_rw("index_lookup", &ImFont::IndexLookup);
    Font.def_rw("fallback_glyph", &ImFont::FallbackGlyph);
    Font.def_rw("fallback_advance_x", &ImFont::FallbackAdvanceX);
    Font.def_rw("fallback_char", &ImFont::FallbackChar);
    Font.def_rw("config_data_count", &ImFont::ConfigDataCount);
    Font.def_rw("config_data", &ImFont::ConfigData);
    Font.def_rw("container_atlas", &ImFont::ContainerAtlas);
    Font.def_rw("ascent", &ImFont::Ascent);
    Font.def_rw("descent", &ImFont::Descent);
    Font.def_rw("dirty_lookup_tables", &ImFont::DirtyLookupTables);
    Font.def_rw("metrics_total_surface", &ImFont::MetricsTotalSurface);
    Font.def(py::init<>());
    Font.def("clear_output_data", &ImFont::ClearOutputData
    , py::rv_policy::automatic_reference);
    Font.def("build_lookup_table", &ImFont::BuildLookupTable
    , py::rv_policy::automatic_reference);
    Font.def("find_glyph", &ImFont::FindGlyph
    , py::arg("c")
    , py::rv_policy::automatic_reference);
    Font.def("find_glyph_no_fallback", &ImFont::FindGlyphNoFallback
    , py::arg("c")
    , py::rv_policy::automatic_reference);
    Font.def("get_char_advance", &ImFont::GetCharAdvance
    , py::arg("c")
    , py::rv_policy::automatic_reference);
    Font.def("is_loaded", &ImFont::IsLoaded
    , py::rv_policy::automatic_reference);
    Font.def("get_debug_name", &ImFont::GetDebugName
    , py::rv_policy::automatic_reference);
    Font.def("calc_word_wrap_position_a", &ImFont::CalcWordWrapPositionA
    , py::arg("scale")
    , py::arg("text")
    , py::arg("text_end")
    , py::arg("wrap_width")
    , py::rv_policy::automatic_reference);
    Font.def("render_char", &ImFont::RenderChar
    , py::arg("draw_list")
    , py::arg("size")
    , py::arg("pos")
    , py::arg("col")
    , py::arg("c")
    , py::rv_policy::automatic_reference);
    Font.def("render_text", &ImFont::RenderText
    , py::arg("draw_list")
    , py::arg("size")
    , py::arg("pos")
    , py::arg("col")
    , py::arg("clip_rect")
    , py::arg("text_begin")
    , py::arg("text_end")
    , py::arg("wrap_width") = 0.0f
    , py::arg("cpu_fine_clip") = false
    , py::rv_policy::automatic_reference);
    Font.def("grow_index", &ImFont::GrowIndex
    , py::arg("new_size")
    , py::rv_policy::automatic_reference);
    Font.def("add_glyph", &ImFont::AddGlyph
    , py::arg("src_cfg") = nullptr
    , py::arg("c")
    , py::arg("x0")
    , py::arg("y0")
    , py::arg("x1")
    , py::arg("y1")
    , py::arg("u0")
    , py::arg("v0")
    , py::arg("u1")
    , py::arg("v1")
    , py::arg("advance_x")
    , py::rv_policy::automatic_reference);
    Font.def("add_remap_char", &ImFont::AddRemapChar
    , py::arg("dst")
    , py::arg("src")
    , py::arg("overwrite_dst") = true
    , py::rv_policy::automatic_reference);

    Style.def("set_color", [](ImGuiStyle& self, int item, ImVec4 color)
    {
        if (item < 0) throw py::index_error();
        if (item >= IM_ARRAYSIZE(self.Colors)) throw py::index_error();
        self.Colors[item] = color;
    }, py::arg("item"), py::arg("color"));
    IO.def("set_mouse_down", [](ImGuiIO& self, int button, bool down)
    {
        if (button < 0) throw py::index_error();
        if (button >= IM_ARRAYSIZE(self.MouseDown)) throw py::index_error();
        self.MouseDown[button] = down;
    }, py::arg("button"), py::arg("down"));
    IO.def("set_key_down", [](ImGuiIO& self, int key, bool down)
    {
        if (key < 0) throw py::index_error();
        if (key >= IM_ARRAYSIZE(self.KeysDown)) throw py::index_error();
        self.KeysDown[key] = down;
    }, py::arg("key"), py::arg("down"));
    IO.def("set_key_map", [](ImGuiIO& self, int key, int value)
    {
        if (key < 0) throw py::index_error();
        if (key >= IM_ARRAYSIZE(self.KeyMap)) throw py::index_error();
        self.KeyMap[key] = value;
    }, py::arg("key"), py::arg("value"));
    DrawData.def_prop_ro("cmd_lists", [](const ImDrawData& self)
    {
        py::list ret;
        for(int i = 0; i < self.CmdListsCount; i++)
        {
            ret.append(self.CmdLists[i]);
        }
        return ret;
    });
    DrawVert.def_prop_ro_static("pos_offset", [](py::object)
    {
        return IM_OFFSETOF(ImDrawVert, pos);
    });
    DrawVert.def_prop_ro_static("uv_offset", [](py::object)
    {
        return IM_OFFSETOF(ImDrawVert, uv);
    });
    DrawVert.def_prop_ro_static("col_offset", [](py::object)
    {
        return IM_OFFSETOF(ImDrawVert, col);
    });
    FontAtlas.def("get_tex_data_as_alpha8", [](ImFontAtlas& atlas)
    {
        unsigned char* pixels;
        int width, height, bytes_per_pixel;
        atlas.GetTexDataAsAlpha8(&pixels, &width, &height, &bytes_per_pixel);
        std::string data((char*)pixels, width * height * bytes_per_pixel);
        return py::make_tuple(width, height, py::bytes(data.data()));
    });
    FontAtlas.def("get_tex_data_as_rgba32", [](ImFontAtlas& atlas)
    {
        unsigned char* pixels;
        int width, height, bytes_per_pixel;
        atlas.GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);
        std::string data((char*)pixels, width * height * bytes_per_pixel);
        return py::make_tuple(width, height, py::bytes(data.data()));
    });
    deargui.def("init", []()
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(100.0, 100.0);
        unsigned char* pixels;
        int w, h;
        io.Fonts->GetTexDataAsAlpha8(&pixels, &w, &h, nullptr);
    });
    deargui.def("input_text", [](const char* label, const char* data, size_t max_size, ImGuiInputTextFlags flags)
    {
        max_size++;
        char* text = (char*)malloc(max_size * sizeof(char));
        strncpy(text, data, max_size);
        auto ret = ImGui::InputText(label, text, max_size, flags, nullptr, NULL);
        std::string output(text);
        free(text);
        return py::make_tuple(ret, output);
    }
    , py::arg("label")
    , py::arg("data")
    , py::arg("max_size")
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("input_text_multiline", [](const char* label, const char* data, size_t max_size, const ImVec2& size, ImGuiInputTextFlags flags)
    {
        max_size++;
        char* text = (char*)malloc(max_size * sizeof(char));
        strncpy(text, data, max_size);
        auto ret = ImGui::InputTextMultiline(label, text, max_size, size, flags, nullptr, NULL);
        std::string output(text);
        free(text);
        return py::make_tuple(ret, output);
    }
    , py::arg("label")
    , py::arg("data")
    , py::arg("max_size")
    , py::arg("size") = ImVec2(0,0)
    , py::arg("flags") = 0
    , py::rv_policy::automatic_reference);
    deargui.def("combo", [](const char* label, int * current_item, std::vector<std::string> items, int popup_max_height_in_items)
    {
        std::vector<const char*> ptrs;
        for (const std::string& s : items)
        {
            ptrs.push_back(s.c_str());
        }
        auto ret = ImGui::Combo(label, current_item, ptrs.data(), ptrs.size(), popup_max_height_in_items);
        return py::make_tuple(ret, current_item);
    }
    , py::arg("label")
    , py::arg("current_item")
    , py::arg("items")
    , py::arg("popup_max_height_in_items") = -1
    , py::rv_policy::automatic_reference);
    deargui.def("list_box", [](const char* label, int * current_item, std::vector<std::string> items, int height_in_items)
    {
        std::vector<const char*> ptrs;
        for (const std::string& s : items)
        {
            ptrs.push_back(s.c_str());
        }
        auto ret = ImGui::ListBox(label, current_item, ptrs.data(), ptrs.size(), height_in_items);
        return py::make_tuple(ret, current_item);
    }
    , py::arg("label")
    , py::arg("current_item")
    , py::arg("items")
    , py::arg("height_in_items") = -1
    , py::rv_policy::automatic_reference);
    deargui.def("plot_lines", [](const char* label, std::vector<float> values, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
    {
        ImGui::PlotLines(label, values.data(), values.size(), values_offset, overlay_text, scale_min, scale_max, graph_size, sizeof(float));
    }
    , py::arg("label")
    , py::arg("values")
    , py::arg("values_offset") = 0
    , py::arg("overlay_text") = nullptr
    , py::arg("scale_min") = FLT_MAX
    , py::arg("scale_max") = FLT_MAX
    , py::arg("graph_size") = ImVec2(0,0)
    );
    deargui.def("plot_histogram", [](const char* label, std::vector<float> values, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
    {
        ImGui::PlotHistogram(label, values.data(), values.size(), values_offset, overlay_text, scale_min, scale_max, graph_size, sizeof(float));
    }
    , py::arg("label")
    , py::arg("values")
    , py::arg("values_offset") = 0
    , py::arg("overlay_text") = nullptr
    , py::arg("scale_min") = FLT_MAX
    , py::arg("scale_max") = FLT_MAX
    , py::arg("graph_size") = ImVec2(0,0)
    );
}

namespace Scripting
{
void RegisterDearguiModule()
{
    Scripting::ScriptServer::RegisterModuleInit([]() {
        PyImport_AppendInittab("deargui", PyInit_deargui);
        });
}
};