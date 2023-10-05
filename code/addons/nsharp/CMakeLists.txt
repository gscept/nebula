#-------------------------------------------------------------------------------
# nsharp
#-------------------------------------------------------------------------------

IF (USE_MONO)

fips_begin_sharedlib(NebulaEngine)
    enable_language(CSharp)
    fips_dir(NebulaEngine/)
        fips_files(
            Main.cs
        )
    fips_dir(NebulaEngine/util)
        fips_files(
            String.cs
        )
    fips_dir(NebulaEngine/engine)
        fips_files(
            Api.cs
			Entity.cs
			Component.cs
			Property.cs
			PropertyManager.cs
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
    VS_DOTNET_REFERENCES "System.Runtime.Serialization"
)
# we need to convert .pdb to .mdb files to load debugging symbols correctly
add_custom_command(TARGET NebulaEngine
    COMMAND ${MONO_ROOT}/bin/pdb2mdb ${FIPS_PROJECT_DEPLOY_DIR}/NebulaEngine${CMAKE_SHARED_LIBRARY_SUFFIX}
    VERBATIM
    POST_BUILD
)

fips_begin_lib(nsharp)
    fips_libs(foundation application ${MONO_LIBRARIES})
    fips_deps(NebulaEngine)

    fips_files(
        monoconfig.h
        monoserver.cc
        monoserver.h
        monobindings.h
        monobindings.cc
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
            monoconfig.h
            mat4.h
            mat4.cc
            vector4.h
            vector4.cc
            vector3.h
            vector3.cc
            vector2.h
            vector2.cc
            quaternion.h
            quaternion.cc
            bbox.h
            bbox.cc
            entity.h
            entity.cc
        )
fips_end_lib()
target_include_directories(nsharp PUBLIC ${CODE_ROOT}/foundation ${MONO_INCLUDE_DIRS})
target_precompile_headers(nsharp REUSE_FROM foundation)
target_link_options(nsharp PUBLIC "/WHOLEARCHIVE:nsharp")
target_compile_options(nsharp PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

FILE(COPY ${MONO_ROOT}/bin/mono-2.0-sgen.dll DESTINATION ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}/)
IF(N_DEBUG_SYMBOLS)
    FILE(COPY ${MONO_ROOT}/bin/mono-2.0-sgen.pdb DESTINATION ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}/)
ENDIF(N_DEBUG_SYMBOLS)

ENDIF(USE_MONO)
