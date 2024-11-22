
nebula_begin_app(testtbui windowed)
fips_src(. *.* EXCEPT work/* GROUP test)

fips_files(
    main.cc
    tbuifeatureunit.h
    tbuifeatureunit.cc
    gamestatemanager.h
    gamestatemanager.cc
)

fips_dir(managers)
    fips_files(
        inputmanager.h
        inputmanager.cc
    )

fips_deps(foundation application graphicsfeature render resource dynui tbui editor)
target_precompile_headers(testtbui PRIVATE [["stdneb.h"]] [["foundation/stdneb.h"]] [["render/stdneb.h"]])
nebula_end_app()
