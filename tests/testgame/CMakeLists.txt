nebula_begin_app(testgame windowed)

fips_files(databasetest.cc
    databasetest.h
    entitysystemtest.cc
    entitysystemtest.h
    hierarchytransformtest.cc
    hierarchytransformtest.h
    idtest.cc
    idtest.h
    main.cc
    scriptingtest.cc
    scriptingtest.h
    )

# fips_src(. *.* GROUP test)
set(abs_output_folder "${FIPS_PROJECT_DEPLOY_DIR}")

nebula_idl_compile(testcomponents.json)

fips_deps(foundation application testbase scripting nflatbuffer)
target_precompile_headers(testgame PRIVATE [["foundation/stdneb.h"]] [["application/stdneb.h"]])
nebula_end_app()
