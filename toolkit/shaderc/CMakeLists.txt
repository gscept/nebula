#-------------------------------------------------------------------------------
# shaderc
#-------------------------------------------------------------------------------

fips_begin_app(shaderc cmdline)
    fips_vs_warning_level(3)
    target_include_directories(shaderc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation)
    fips_deps(foundation toolkit-common shadercompiler anyfx antlr4 mcpp render)
    add_definitions(-D__ANYFX__)
        fips_files(
            shaderc.cc
            shadercompilerapp.cc
            shadercompilerapp.h
            singleshadercompiler.cc
            singleshadercompiler.h
        )
fips_end_app()