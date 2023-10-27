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
    attribute.h
    attributeid.h
    tableid.h
    table.cc
    table.h
    tablesignature.h
    attributeregistry.h
    attributeregistry.cc
)
fips_end_lib()
