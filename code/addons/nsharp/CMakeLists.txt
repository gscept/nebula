#-------------------------------------------------------------------------------
# nsharp
#-------------------------------------------------------------------------------

IF (USE_DOTNET)

nebula_idl_generate_cs_target(
    ${NROOT}/code/addons/audiofeature/components/audiofeature.json
    ${NROOT}/code/addons/graphicsfeature/components/camera.json
    ${NROOT}/code/addons/graphicsfeature/components/decal.json
    ${NROOT}/code/addons/graphicsfeature/components/lighting.json
    ${NROOT}/code/addons/graphicsfeature/components/model.json
    ${NROOT}/code/addons/physicsfeature/components/physicsfeature.json
    ${NROOT}/code/application/basegamefeature/components/basegamefeature.json
)

SET(NEBULAENGINE_CS_FILES
    ${NROOT}/code/addons/nsharp/NebulaEngine/Main.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/util/String.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/util/ListExtensions.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/Api.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/Entity.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/Component.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/Property.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/PropertyManager.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/World.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/Msg.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/MsgDispatcher.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/engine/ConsoleHook.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/BoundingBox.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/BoundingFrustum.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/BoundingSphere.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Color.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/ContainmentType.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Mathf.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/PQuatFeedbackLoop.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Matrix.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Plane.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/PlaneIntersectionType.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Point.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Quaternion.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Ray.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Rectangle.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Vector2.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Vector3.cs
    ${NROOT}/code/addons/nsharp/NebulaEngine/mathf/Vector4.cs
)

nebula_dotnet_project(
    NAME NebulaEngine
    PROJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dotnet/NebulaEngine
    OUTPUT_DIR ${FIPS_PROJECT_DEPLOY_DIR}
    RUNTIME_TEMPLATE ${NROOT}/code/addons/nsharp/NebulaEngine/runtimeconfig.template.json
    SOURCES ${NEBULAENGINE_CS_FILES}
    GENERATED_SOURCES ${NIDL_Cs_Files}
    DEPENDS ${NIDL_Cs_Deps}
)

fips_begin_lib(nsharp)
    fips_libs(foundation application ${DOTNET_NETHOST_LIBRARIES})

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

target_include_directories(nsharp PUBLIC ${CODE_ROOT}/foundation ${DOTNET_NETHOST_INCLUDE_DIR})
target_precompile_headers(nsharp REUSE_FROM foundation)
IF(MSVC)
    target_link_options(nsharp PUBLIC "/WHOLEARCHIVE:nsharp")
    set_target_properties(nsharp PROPERTIES VS_GLOBAL_ResolveNuGetPackages "false")
ENDIF(MSVC)
target_compile_options(nsharp PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

IF(WIN32)
    FILE(COPY ${DOTNET_NETHOST_DLL} DESTINATION ${FIPS_PROJECT_DEPLOY_DIR}/)
ENDIF(WIN32)

ENDIF(USE_DOTNET)
