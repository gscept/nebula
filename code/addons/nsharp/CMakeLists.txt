#-------------------------------------------------------------------------------
# nsharp
#-------------------------------------------------------------------------------

IF (USE_DOTNET)

fips_begin_sharedlib(NebulaEngine)
    enable_language(CSharp)
    fips_dir(NebulaEngine/)
        fips_files(
            Main.cs
            runtimeconfig.template.json
        )
    fips_dir(NebulaEngine/util)
        fips_files(
            String.cs
			ListExtensions.cs
        )
    fips_dir(NebulaEngine/engine)
        fips_files(
            Api.cs
			Entity.cs
			Component.cs
			Property.cs
			PropertyManager.cs
            World.cs
            Msg.cs
            ConsoleHook.cs
        )
    fips_dir(NebulaEngine/xna)
        fips_files(
            BoundingBox.cs
            BoundingFrustum.cs
            BoundingSphere.cs
            Color.cs
            ContainmentType.cs
            MathHelper.cs
            Matrix.cs
            Plane.cs
            PlaneIntersectionType.cs
            Quaternion.cs
            Ray.cs
            Rectangle.cs
            Vector2.cs
            Vector3.cs
            Vector4.cs
            Point.cs
        )
fips_end_sharedlib()

target_compile_options(NebulaEngine
    PRIVATE "/langversion:7.2"
    PRIVATE "/unsafe"
)

set_target_properties(NebulaEngine PROPERTIES 
    DOTNET_SDK "Microsoft.NET.Sdk"    
    DOTNET_TARGET_FRAMEWORK "net7.0"
)

IF(WIN32)
    set_target_properties(NebulaEngine PROPERTIES VS_GLOBAL_GenerateRuntimeConfigurationFiles "true")
ENDIF(WIN32)

fips_begin_lib(nsharp)
    fips_libs(foundation application ${DOTNET_NETHOST_LIBRARIES})
    fips_deps(NebulaEngine)

    fips_files(
        assemblyid.h
        scriptfeatureunit.cc
        scriptfeatureunit.h
        nsconfig.h
        nsharpserver.cc
        nsharpserver.h
        nsbindings.h
        nsbindings.cc
    )
    fips_dir(api)
        fips_files(
            application.h
            application.cc
            audio.h
            audio.cc
            render.h
            render.cc
            game.h
            game.cc
        )
    fips_dir(conversion)
        fips_files(
            entity.h
            entity.cc
        )
fips_end_lib()

IF(WIN32)
    set_target_properties(nsharp PROPERTIES VS_GLOBAL_ResolveNuGetPackages "false")
ENDIF(WIN32)

target_include_directories(nsharp PUBLIC ${CODE_ROOT}/foundation ${DOTNET_NETHOST_INCLUDE_DIR})
target_precompile_headers(nsharp REUSE_FROM foundation)
target_link_options(nsharp PUBLIC "/WHOLEARCHIVE:nsharp")
target_compile_options(nsharp PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

FILE(COPY ${DOTNET_NETHOST_DLL} DESTINATION ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}/)

ENDIF(USE_DOTNET)
