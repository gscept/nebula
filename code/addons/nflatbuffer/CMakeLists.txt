#-------------------------------------------------------------------------------
# nflatbuffer
#-------------------------------------------------------------------------------

nebula_begin_lib(nflatbuffer)
fips_include_directories(${CODE_ROOT}/foundation )
fips_dir(.)
    fips_files(
        nebula_flat.cc
        nebula_flat.h
    )
nebula_flatc(SYSTEM foundation/math.fbs)
fips_deps(flatbuffers sqlite3)
nebula_end_lib()
