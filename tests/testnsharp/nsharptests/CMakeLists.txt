fips_begin_sharedlib(NSharpTests)
    enable_language(CSharp)
    
    #fips_libs(NebulaEngine)

    nebula_register_nidl_cs()

    fips_dir(.)
        fips_files(
            Main.cs
        )
fips_end_sharedlib()

set_property(TARGET NSharpTests PROPERTY 
    VS_DOTNET_REFERENCE_NebulaEngine "${FIPS_DEPLOY_DIR}/nebula/${FIPS_CONFIG}/NebulaEngine.dll")

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