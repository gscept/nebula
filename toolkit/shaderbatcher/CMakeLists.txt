#-------------------------------------------------------------------------------
# shaderbatcher
#-------------------------------------------------------------------------------

fips_begin_lib(shadercompiler)
target_include_directories(shadercompiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation)
add_definitions(-D__ANYFX__)
fips_deps(anyfx glslang)
fips_files(shadercompiler.cc
           shadercompiler.h)        
fips_end_lib()
fips_begin_app(shaderbatcher cmdline)
    fips_vs_warning_level(3)
    fips_deps(foundation toolkit-common shadercompiler anyfx antlr4 mcpp)
        fips_files(
            shaderbatcher.cc
            shaderbatcherapp.cc
            shaderbatcherapp.h
        )
fips_end_app()