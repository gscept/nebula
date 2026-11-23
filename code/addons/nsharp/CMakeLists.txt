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
    **/*.cs
)

nebula_dotnet_project(
    NAME NebulaEngine
    PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NebulaEngine/
    OUTPUT_DIR ${FIPS_PROJECT_DEPLOY_DIR}
    RUNTIME_TEMPLATE ${NROOT}/code/addons/nsharp/NebulaEngine/runtimeconfig.template.json
    SOURCES ${NEBULAENGINE_CS_FILES}
    GENERATED_SOURCES ${NIDL_Cs_Files}
    DEPENDS ${NIDL_Cs_Deps}
)

add_library(nsharpbindings OBJECT
    nsbindings.cc
    api/application.cc
    api/audio.cc
    api/render.cc
    api/game.cc
    api/input.cc
)
target_link_libraries(nsharpbindings PRIVATE foundation application input)
target_include_directories(nsharpbindings PUBLIC ${CODE_ROOT}/foundation ${DOTNET_NETHOST_INCLUDE_DIR})
target_precompile_headers(nsharpbindings REUSE_FROM foundation)
set_target_properties(nsharpbindings PROPERTIES CXX_VISIBILITY_PRESET hidden)

nebula_begin_lib(nsharp)
    fips_libs(foundation application input ${DOTNET_NETHOST_LIBRARIES})
    if(N_EDITOR)
        # The feature defines its own editor contribution, so it needs the editor's registry
        fips_libs(neweditor)
    endif()

    fips_files(
        assemblyid.h
        scriptfeatureunit.cc
        scriptfeatureunit.h
        nsconfig.h
        nsharpserver.cc
        nsharpserver.h
        nsbindings.h
        propertyextension.h
        propertyextension.cc
    )
    fips_dir(api)
        fips_files(
            application.h
            audio.h
            render.h
            game.h
            input.h
        )
    fips_dir(conversion)
        fips_files(
            entity.h
            entity.cc
        )
    fips_dir(editor)
        fips_files(
            scripteditor.h
            scripteditor.cc
        )
nebula_end_lib()

target_include_directories(nsharp PUBLIC ${CODE_ROOT}/foundation ${DOTNET_NETHOST_INCLUDE_DIR})
target_precompile_headers(nsharp REUSE_FROM foundation)
target_sources(nsharp INTERFACE $<TARGET_OBJECTS:nsharpbindings>)
IF(MSVC)
    set_target_properties(nsharp PROPERTIES VS_GLOBAL_ResolveNuGetPackages "false")
ENDIF(MSVC)
target_compile_options(nsharp PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
target_compile_options(nsharpbindings PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

IF(FIPS_POSIX AND NOT FIPS_OSX)
    target_compile_options(nsharp INTERFACE "-fvisibility=hidden")
    target_link_options(nsharp INTERFACE "-Wl,--export-dynamic" "-Wl,--exclude-libs,ALL")
ENDIF()

IF(WIN32)
    FILE(COPY ${DOTNET_NETHOST_DLL} DESTINATION ${FIPS_PROJECT_DEPLOY_DIR}/)
ENDIF(WIN32)

ENDIF(USE_DOTNET)
