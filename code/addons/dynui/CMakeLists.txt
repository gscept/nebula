#-------------------------------------------------------------------------------
# dynui module
#-------------------------------------------------------------------------------

nebula_begin_module(dynui)
fips_ide_group(addons)
fips_deps(render imgui scripting)
target_include_directories(dynui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/resource ${CODE_ROOT}/render ${CODE_ROOT}/foundation ${Python_INCLUDE_DIRS})

fips_files(
    imguiinputhandler.cc
    imguiinputhandler.h
    imguicontext.cc
    imguicontext.h
)

fips_dir(imgui/shaders)
    add_shaders(imgui.fx)
fips_dir(im3d/shaders)
    add_shaders(im3d.fx) 

fips_dir(console)
    fips_files(
        imguiconsole.cc
        imguiconsole.h
        imguiconsolehandler.cc
        imguiconsolehandler.h
    )

fips_dir(profiler)
    fips_files(
        imguiprofiler.cc
        imguiprofiler.h
    )
fips_dir(im3d)
    fips_files(im3d.h im3d.cpp im3d_config.h im3d_math.h im3dcontext.h im3dcontext.cc)
fips_dir(graph)
    fips_files(imguigraph.h imguigraph.cc)
fips_dir(scripting)
    fips_files(deargui.cpp)
nebula_end_module()

target_compile_options(dynui PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
target_link_options(dynui PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/WHOLEARCHIVE:dynui>)
target_precompile_headers(dynui REUSE_FROM render)
