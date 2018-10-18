# FIXME this should be split into seperate modules later on

fips_begin_module(gltf)
fips_include_directories(${CODE_ROOT}/foundation)
fips_deps(foundation)
fips_files_ex(. *.* GROUP "gltf")
fips_end_module()