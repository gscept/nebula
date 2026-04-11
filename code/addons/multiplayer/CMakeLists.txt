#-------------------------------------------------------------------------------
# multiplayer
#-------------------------------------------------------------------------------

nebula_begin_module(multiplayer)
fips_ide_group(addons)
fips_libs(foundation application)
fips_deps(GameNetworkingSockets)

target_include_directories(multiplayer PUBLIC ${CODE_ROOT}/foundation)
target_precompile_headers(multiplayer PRIVATE <foundation/stdneb.h>)

fips_files(
    multiplayerfeatureunit.cc
    multiplayerfeatureunit.h
)
fips_dir(server)
    fips_files(
        clientconnection.cc
        clientconnection.h
        basemultiplayerserver.cc
        basemultiplayerserver.h
        serverprocessors.cc
        serverprocessors.h
        standardmultiplayerserver.cc
        standardmultiplayerserver.h
    )
fips_dir(client)
    fips_files(
        basemultiplayerclient.cc
        basemultiplayerclient.h
        clientprocessors.cc
        clientprocessors.h
        standardmultiplayerclient.cc
        standardmultiplayerclient.h
    )
fips_dir(components)
		nebula_idl_compile(
			multiplayer.json
		)
fips_dir(.)

nebula_flatc(SYSTEM addons/multiplayer/standardprotocol.fbs)
nebula_end_module()
