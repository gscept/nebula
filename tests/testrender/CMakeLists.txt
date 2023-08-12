
fips_begin_app(testrender windowed)
fips_files(
    main.cc
    animtest.cc
    animtest.h
    rendertest.cc
    rendertest.h
)
fips_src(. *.* GROUP test foundation render resources)
fips_deps(foundation render resource testbase imgui dynui)
target_precompile_headers(testrender PRIVATE [["stdneb.h"]] [["foundation/stdneb.h"]] [["render/stdneb.h"]])
fips_end_app()