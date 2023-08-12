
nebula_begin_app(testviewer cmdline)
fips_src(. *.* EXCEPT work/* GROUP test)
fips_src(scenes/ *.* GROUP scenes)
fips_deps(foundation testbase render dynui physics)
target_precompile_headers(testviewer PRIVATE [["stdneb.h"]] [["foundation/stdneb.h"]] [["render/stdneb.h"]])
nebula_end_app()
