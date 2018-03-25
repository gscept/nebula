#-------------------------------------------------------------------------------
# idlcompiler
#-------------------------------------------------------------------------------
fips_begin_module(idlcompiler)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    fips_deps(foundation toolkitutil)
        fips_files(stdneb.cc)
        fips_files(
            idlc.cc
            idlcompiler.cc
            idlcompiler.h
        )
fips_end_module()
