IF(USE_DOTNET)

nebula_idl_generate_cs_target(
    components/nsharpcomponents.json
)

nebula_dotnet_project(
    NAME NSharpTests
    PROJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dotnet/NSharpTests
    OUTPUT_DIR ${FIPS_PROJECT_DEPLOY_DIR}
    SOURCES ${NROOT}/tests/testnsharp/nsharptests/Main.cs
    GENERATED_SOURCES ${NIDL_Cs_Files}
    DEPENDS ${NIDL_Cs_Deps} ${NebulaEngine_DOTNET_TARGET}
    REFERENCES ${NebulaEngine_DOTNET_PROJECT}
)

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
    target_precompile_headers(testnsharp PRIVATE [["foundation/stdneb.h"]])
    add_dependencies(testnsharp NSharpTestsManaged)

    IF(FIPS_POSIX)
        target_link_options(testnsharp PRIVATE "-Wl,--export-dynamic")
        target_link_options(testnsharp PRIVATE
            "-Wl,--whole-archive"
            "$<TARGET_FILE:nsharp>"
            "-Wl,--no-whole-archive"
        )
    ENDIF(FIPS_POSIX)

nebula_end_app()

ENDIF(USE_DOTNET)
