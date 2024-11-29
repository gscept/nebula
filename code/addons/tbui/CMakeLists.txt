#-------------------------------------------------------------------------------
# tbui module
#-------------------------------------------------------------------------------

nebula_begin_module(tbui)
fips_ide_group(addons)
#target_include_directories(tbui 
#    PUBLIC 
#    ${CMAKE_CURRENT_SOURCE_DIR} 
#    ${CMAKE_CURRENT_SOURCE_DIR}/backend
#    ${CODE_ROOT}/resource 
#    ${CODE_ROOT}/render
#    ${CODE_ROOT}/foundation 
#    ${Python_INCLUDE_DIRS}
#    BEFORE
#)
fips_deps(render scripting turbobadger)
target_precompile_headers(tbui PRIVATE <render/stdneb.h>)

fips_files(
#    tb_config.h
    tbuicontext.cc
    tbuicontext.h
    tbuiinputhandler.cc
    tbuiinputhandler.h
    tbuiview.cc
    tbuiview.h
)

add_shaders(tbui.fx)

fips_dir(backend)
    fips_files(
        tbuibatch.h
        tbuibitmap.cc
        tbuibitmap.h
        tbuiclipboard.cc
        tbuifile.cc
        tbuifile.h
        tbuirenderer.cc
        tbuirenderer.h
        tbuisystem.cc
        tbuivertex.h
    )

#fips_dir(tb/animation)
#    fips_files(
#        tb_animation_utils.h
#        tb_animation.cpp
#        tb_animation.h
#        tb_widget_animation.cpp
#        tb_widget_animation.h
#    )
#
#fips_dir(tb/image)
#    fips_files(
#        tb_image_manager.cpp
#        tb_image_manager.h
#        tb_image_widget.cpp
#        tb_image_widget.h
#    )
#
#fips_dir(tb/parser)
#    fips_files(
#        tb_parser.cpp
#        tb_parser.h
#    )
#
#fips_dir(tb/renderers)
#    fips_files(
#        tb_renderer_batcher.cpp
#        tb_renderer_batcher.h
##       tb_renderer_gl.cpp
##       tb_renderer_gl.h
#    )
#
##fips_dir(tb/tests)
##    fips_files(
##        tb_test.cpp
##        tb_test.h
##        test_tb_color.cpp
##        test_tb_dimension.cpp
##        test_tb_geometry.cpp
##        test_tb_hashtable.cpp
##        test_tb_linklist.cpp
##        test_tb_node_ref_tree.cpp
##        test_tb_object.cpp
##        test_tb_parser_definitions.tb.txt
##        test_tb_parser_included.tb.txt
##        test_tb_parser.cpp
##        test_tb_parser.tb.txt
##        test_tb_space_allocator.cpp
##        test_tb_style_edit.cpp
##        test_tb_tempbuffer.cpp
##        test_tb_test.cpp
##        test_tb_value.cpp
##        test_tb_widget_value.cpp
##    )
#
#fips_dir(tb/thirdparty)
#    fips_files(
#        stb_image.h
#        stb_truetype.h
#    )
#
#fips_dir(tb/utf8)
#    fips_files(
#        utf8.cpp
#        utf8.h
#    )
#
#fips_dir(tb)
#    fips_files(
#        tb_bitmap_fragment.cpp
#        tb_bitmap_fragment.h
##        tb_clipboard_dummy.cpp
##        tb_clipboard_glfw.cpp
##        tb_clipboard_win.cpp
#        tb_color.cpp
#        tb_color.h
##        tb_config.h
#        tb_core.cpp
#        tb_core.h
#        tb_debug.cpp
#        tb_debug.h
#        tb_dimension.cpp
#        tb_dimension.h
#        tb_editfield.cpp
#        tb_editfield.h
##        tb_file_posix.cpp
#        tb_font_desc.h
#        tb_font_renderer.cpp
#        tb_font_renderer.h
#        tb_font_renderer_freetype.cpp
#        tb_font_renderer_stb.cpp
#        tb_font_renderer_tbbf.cpp
#        tb_geometry.cpp
#        tb_geometry.h
#        tb_hash.cpp
#        tb_hash.h
#        tb_hashtable.cpp
#        tb_hashtable.h
#        tb_id.cpp
#        tb_id.h
#        tb_image_loader_stb.cpp
#        tb_inline_select.cpp
#        tb_inline_select.h
#        tb_language.cpp
#        tb_language.h
#        tb_layout.cpp
#        tb_layout.h
#        tb_linklist.cpp
#        tb_linklist.h
#        tb_list.cpp
#        tb_list.h
#        tb_menu_window.cpp
#        tb_menu_window.h
#        tb_message_window.cpp
#        tb_message_window.h
#        tb_msg.cpp
#        tb_msg.h
#        tb_node_ref_tree.cpp
#        tb_node_ref_tree.h
#        tb_node_tree.cpp
#        tb_node_tree.h
#        tb_object.cpp
#        tb_object.h
#        tb_popup_window.cpp
#        tb_popup_window.h
#        tb_renderer.cpp
#        tb_renderer.h
#        tb_scroller.cpp
#        tb_scroller.h
#        tb_scroll_container.cpp
#        tb_scroll_container.h
#        tb_select.cpp
#        tb_select.h
#        tb_select_item.cpp
#        tb_select_item.h
#        tb_skin.cpp
#        tb_skin.h
#        tb_skin_util.cpp
#        tb_skin_util.h
#        tb_sort.h
#        tb_str.cpp
#        tb_str.h
#        tb_style_edit.cpp
#        tb_style_edit.h
#        tb_style_edit_content.cpp
#        tb_style_edit_content.h
#        tb_system.h
##        tb_system_android.cpp
##        tb_system_linux.cpp
##        tb_system_win.cpp
#        tb_tab_container.cpp
#        tb_tab_container.h
#        tb_tempbuffer.cpp
#        tb_tempbuffer.h
#        tb_toggle_container.cpp
#        tb_toggle_container.h
#        tb_types.h
#        tb_value.cpp
#        tb_value.h
#        tb_widgets.cpp
#        tb_widgets.h
#        tb_widgets_common.cpp
#        tb_widgets_common.h
#        tb_widgets_listener.cpp
#        tb_widgets_listener.h
#        tb_widgets_reader.cpp
#        tb_widgets_reader.h
#        tb_widget_skin_condition_context.cpp
#        tb_widget_skin_condition_context.h
#        tb_widget_value.cpp
#        tb_widget_value.h
#        tb_window.cpp
#        tb_window.h
#    )

nebula_end_module()

target_compile_options(tbui PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
target_link_options(tbui PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/WHOLEARCHIVE:tbui>)
#target_include_directories(tbui BEFORE PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR} )
