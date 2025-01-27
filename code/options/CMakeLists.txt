#-------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------

nebula_begin_module(options)
    target_include_directories(options PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/options)
    target_precompile_headers(options PRIVATE <foundation/stdneb.h>)
    fips_deps(foundation)
        fips_files(
            options.cc
            options.h
        )
    nebula_flatc(SYSTEM options/projectsettings.fbs options/levelsettings.fbs)

nebula_end_module()
