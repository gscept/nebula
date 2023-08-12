
nebula_begin_app(testgame windowed)
fips_src(. *.* GROUP test)

set(abs_output_folder "${FIPS_PROJECT_DEPLOY_DIR}")
add_custom_command(
    OUTPUT ${abs_output_folder}/blueprints_test.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/blueprints_test.json ${abs_output_folder}/blueprints_test.json
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/blueprints_test.json
)

nebula_add_nidl(testproperties.nidl)
fips_deps(foundation application testbase scripting )
target_precompile_headers(testgame PRIVATE [["foundation/stdneb.h"]] [["application/stdneb.h"]])
nebula_end_app()
