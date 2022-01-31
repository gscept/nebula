# gltf module

fips_begin_module(gltf)
fips_ide_group(addons)
fips_include_directories(${CODE_ROOT}/foundation ${CODE_ROOT}/render)
fips_deps(foundation)
fips_files_ex(. *.* GROUP "gltf")
fips_end_module()
