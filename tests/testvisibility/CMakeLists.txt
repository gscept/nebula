
fips_begin_app(testvisibility cmdline)
fips_src(. *.* GROUP test foundation render)
fips_deps(foundation testbase render imgui dynui)
target_precompile_headers(testvisibility PRIVATE [["foundation/stdneb.h"]] [["render/stdneb.h"]])
fips_end_app()