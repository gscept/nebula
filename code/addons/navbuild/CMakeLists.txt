#-------------------------------------------------------------------------------
# navbuild
#
# Builds Detour navmesh data from geometry. Kept free of the game and render
# layers so the editor, the asset batcher and tests can link it.
#-------------------------------------------------------------------------------
nebula_begin_module(navbuild)
fips_deps(foundation nflatbuffer physics Recast Detour)
target_precompile_headers(navbuild PRIVATE [["foundation/stdneb.h"]])
fips_files(
    navmeshbuilder.h
    navmeshbuilder.cc
    geometrysources.h
    geometrysources.cc
    navmeshasset.h
    navmeshasset.cc
    navareas.h
    navareas.cc
)
nebula_flatc(SYSTEM navigation/navmesh.fbs navigation/navmeshsettings.fbs navigation/agentkinds.fbs navigation/bakednavmesh.fbs navigation/navareas.fbs)
nebula_end_module()
