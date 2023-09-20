#-------------------------------------------------------------------------------
# nflatbuffer
#-------------------------------------------------------------------------------

nebula_begin_lib(nflatbuffer)
fips_ide_group(addons)
target_include_directories(nflatbuffer PRIVATE ${CODE_ROOT}/foundation )
target_precompile_headers(nflatbuffer REUSE_FROM foundation)
fips_dir(.)
    fips_files(
        nebula_flat.cc
        nebula_flat.h
        flatbufferinterface.cc
        flatbufferinterface.h
    )
nebula_flatc(SYSTEM foundation/math.fbs)
fips_deps(flatbuffers sqlite3 foundation)
nebula_end_lib()
