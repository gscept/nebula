#-------------------------------------------------------------------------------
# shaderc
#-------------------------------------------------------------------------------

fips_begin_app(shaderc cmdline)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation)
    fips_deps(foundation toolkit-common shadercompiler anyfx antlr4 mcpp)
    add_definitions(-D__ANYFX__)
        fips_files(
            shaderc.cc
            shadercompilerapp.cc
            shadercompilerapp.h
            singleshadercompiler.cc
            singleshadercompiler.h
        )
fips_end_app()