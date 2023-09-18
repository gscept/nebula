IF(USE_MONO)

fips_begin_sharedlib(NSharpTests)
    enable_language(CSharp)
    fips_libs(NebulaEngine)
    fips_dir(nsharptests/)
        fips_files(
            Main.cs
        )
fips_end_sharedlib()

target_compile_options(NebulaEngine
    PRIVATE "/langversion:7.2"
    PRIVATE "/unsafe"
)
set_target_properties(NebulaEngine PROPERTIES 
    VS_DOTNET_REFERENCES "System.Runtime.Serialization"
)
# we need to convert .pdb to .mdb files to load debugging symbols correctly
add_custom_command(TARGET NSharpTests
    COMMAND ${MONO_ROOT}/bin/pdb2mdb ${FIPS_PROJECT_DEPLOY_DIR}/NSharpTests${CMAKE_SHARED_LIBRARY_SUFFIX}
    VERBATIM
    POST_BUILD
)

nebula_begin_app(testnsharp windowed)
fips_src(. *.* GROUP test)

fips_deps(foundation testbase nsharp NSharpTests)
target_precompile_headers(testgame PRIVATE [["foundation/stdneb.h"]])
nebula_end_app()

ENDIF(USE_MONO)
