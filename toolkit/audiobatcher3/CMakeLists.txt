#-------------------------------------------------------------------------------
# audiobatcher
#-------------------------------------------------------------------------------
fips_begin_lib(audiobatcher)
    fips_vs_warning_level(3)
    target_include_directories(audiobatcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    fips_deps(foundation toolkitutil)
        fips_files(
            audiobatcher3.cc
            audiobatcherapp.cc
            audiobatcherapp.h
        )
fips_end_lib()