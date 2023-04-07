#-------------------------------------------------------------------------------
# dynui module
#-------------------------------------------------------------------------------

nebula_begin_module(staticui)
fips_ide_group(addons)
fips_deps(render ultralight scripting)
target_include_directories(staticui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/resource ${CODE_ROOT}/render ${CODE_ROOT}/foundation ${Python_INCLUDE_DIRS})
add_shaders(staticui.fx)
fips_files(
    staticuicontext.cc
    staticuicontext.h
)

fips_dir(ultralight)
    fips_files(

        ultralightrenderer.cc
        ultralightrenderer.h
    )

nebula_end_module()

target_compile_options(staticui PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
target_link_options(staticui PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/WHOLEARCHIVE:staticui>)
