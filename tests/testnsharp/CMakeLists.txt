IF(USE_DOTNET)

nebula_idl_generate_cs_target(
    components/nsharpcomponents.json
)

add_subdirectory(nsharptests)

nebula_begin_app(testnsharp windowed)
    fips_dir(.)
        fips_files(
            main.cc
            nsharptest.cc
            nsharptest.h
        )
    
    fips_dir(components)
        nebula_idl_compile(nsharpcomponents.json)

    fips_dir(.)

    fips_deps(foundation testbase nsharp)
    target_precompile_headers(testgame PRIVATE [["foundation/stdneb.h"]])

    nebula_add_script_library(NSharpTests)

nebula_end_app()

ENDIF(USE_DOTNET)
