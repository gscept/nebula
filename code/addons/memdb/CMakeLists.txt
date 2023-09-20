fips_begin_lib(memdb)
fips_ide_group(addons)
target_include_directories(memdb PUBLIC ${CODE_ROOT}/foundation)
target_precompile_headers(memdb REUSE_FROM foundation)
fips_deps(foundation)
fips_files(
    database.h
    database.cc
    dataset.cc
    dataset.h
    filterset.h
    componentdescription.h
    componentid.h
    table.h
    tablesignature.h
    typeregistry.h
    typeregistry.cc
)
fips_end_lib()
