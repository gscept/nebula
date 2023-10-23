IF(USE_DOTNET)

nebula_idl_generate_cs_target(
    components/nsharpcomponents.json
)

fips_begin_sharedlib(NSharpTests)
    enable_language(CSharp)
    fips_libs(NebulaEngine)

    nebula_register_nidl_cs()

    fips_dir(nsharptests/)
        fips_files(
            Main.cs
        )
fips_end_sharedlib()

target_compile_options(NSharpTests
    PRIVATE "/langversion:7.2"
    PRIVATE "/unsafe"
)

set_target_properties(NSharpTests PROPERTIES 
    DOTNET_SDK "Microsoft.NET.Sdk"        
    DOTNET_TARGET_FRAMEWORK "net7.0"
)

IF(WIN32)
    set_target_properties(NSharpTests PROPERTIES VS_GLOBAL_GenerateRuntimeConfigurationFiles "true")
ENDIF(WIN32)

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
