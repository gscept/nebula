#-------------------------------------------------------------------------------
# archiver
#-------------------------------------------------------------------------------

fips_begin_module(archiver)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    fips_deps(foundation toolkitutil)
        fips_files(
            archiver3.cc
            archiverapp.cc
            archiverapp.h
        )
fips_end_module()