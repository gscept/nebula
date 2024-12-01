#-------------------------------------------------------------------------------
# tbui module
#-------------------------------------------------------------------------------

nebula_begin_module(tbui)
fips_ide_group(addons)

fips_deps(render scripting turbobadger)
target_precompile_headers(tbui PRIVATE <render/stdneb.h>)
target_include_directories(tbui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

fips_files(
    tbuicontext.cc
    tbuicontext.h
    tbuiinputhandler.cc
    tbuiinputhandler.h
    tbuiview.cc
    tbuiview.h
)

add_shaders(tbui.fx)

fips_dir(backend)
    fips_files(
        tbuibatch.h
        tbuibitmap.cc
        tbuibitmap.h
        tbuiclipboard.cc
        tbuifile.cc
        tbuifile.h
        tbuirenderer.cc
        tbuirenderer.h
        tbuisystem.cc
        tbuivertex.h
    )

nebula_end_module()
