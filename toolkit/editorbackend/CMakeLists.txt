fips_begin_app(editorbackend cmdline)
    fips_vs_warning_level(3)
    fips_deps(foundation db memdb)
    fips_files(
        main.cc
        appsettings.h
        appsettings.cc
        attributes.h
        attributes.cc
    )
fips_end_app()