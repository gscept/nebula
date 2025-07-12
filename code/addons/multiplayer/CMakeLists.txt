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
        multiplayerserver.cc
        multiplayerserver.h
    )
fips_dir(client)
    fips_files(
        multiplayerclient.cc
        multiplayerclient.h
    )
fips_dir(.)

nebula_flatc(SYSTEM addons/multiplayer/protocol.fbs)
nebula_end_module()