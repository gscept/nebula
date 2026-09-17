#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------
fips_begin_lib(toolkitutil)
    target_include_directories(toolkitutil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/render ${CODE_ROOT}/)
    target_precompile_headers(toolkitutil PRIVATE [["foundation/stdneb.h"]] [["render/stdneb.h"]] [["model/import/base/scenenode.h"]])
    
    fips_deps(foundation)
        fips_files(
            applauncher.cc
            applauncher.h       
            logger.cc
            logger.h
            platform.cc
            platform.h
            projectinfo.cc
            projectinfo.h
            rcswrapper.cc
            rcswrapper.h     
            text.cc
            text.h
            toolkitapp.cc
            toolkitapp.h
            toolkitconsolehandler.cc
            toolkitconsolehandler.h
            toolkitversion.cc
            toolkitversion.h            
        )       
        fips_dir(base)
            fips_files(
                applauncherbase.h
                exporttypes.h
            )
        fips_dir(converters)
            fips_files(
                binaryxmlconverter.cc
                binaryxmlconverter.h
                #n2batchconverter.cc
                #n2batchconverter.h
                #n2converter.cc
                #n2converter.h
            )
        IF (MSVC)
            fips_dir(win32)
                fips_files(
                    win32applauncher.cc
                    win32applauncher.h
                )
        ELSE()
            fips_dir(posix)
            fips_files(
                posixapplauncher.cc
                posixapplauncher.h
            )
        ENDIF()

        fips_dir(distributedtools)
            fips_files(
                distributedjobscheduler.cc
                distributedjobscheduler.h
                distributedtoolkitapp.cc
                distributedtoolkitapp.h
                remotejobservice.cc
                remotejobservice.h
                shareddircontrol.cc
                shareddircontrol.h
                shareddircreator.cc
                shareddircreator.h
                shareddirfilesystem.cc
                shareddirfilesystem.h
                shareddirftp.cc
                shareddirftp.h
            )
            fips_dir(distributedtools/distributedjobs)
                fips_files(
                    distributedappjob.cc
                    distributedappjob.h
                    distributedcreateemptyfilejob.cc
                    distributedcreateemptyfilejob.h
                    distributeddeletedirectorycontentjob.cc
                    distributeddeletedirectorycontentjob.h
                    distributedjob.cc
                    distributedjob.h
                )

    if(FIPS_WINDOWS)
        fips_libs(CMP_Core_MD CMP_Framework_MD)
    else()
        fips_libs(xml2 CMP_Framework CMP_Common CMP_Core CMP_Core_SSE CMP_Core_AVX512 CMP_Core_AVX CMP_Compressonator CMP_GpuDecode Image_DDS Image_EXR Image_KTX Image_TGA)
    endif()
    
    fips_dir(.)
        fips_files(
            assetfile.cc
            assetfile.h
            assetregistry.cc
            assetregistry.h
            assetupdater.cc
            assetupdater.h
            audioexporter.cc
            audioexporter.h

            #resourcedictionarybuilder.cc
            #resourcedictionarybuilder.h
        )

        fips_dir(asset)
            fips_files(
                assetbatchprocessor.cc
                assetbatchprocessor.h
				assetimporter.cc
				assetimporter.h
				assetpackager.cc
				assetpackager.h
                assetprocessorbase.cc
                assetprocessorbase.h
            )
        fips_dir(filedb)
            fips_files(
                filedbattrs.cc
                filedbattrs.h
                filedb.cc
                filedb.h
            )
        fips_dir(model/animutil)
            fips_files(
                animbuilder.cc
                animbuilder.h
                animbuilderclip.cc
                animbuilderclip.h
                animbuildercurve.cc
                animbuildercurve.h
                animbuildersaver.cc
                animbuildersaver.h
            )
        fips_dir(model/meshutil)
            fips_files(
                meshbuilder.cc
                meshbuilder.h
                meshbuildergroup.cc
                meshbuildergroup.h
                meshbuildersaver.cc
                meshbuildersaver.h
                meshbuildertriangle.cc
                meshbuildertriangle.h
                meshbuildervertex.cc
                meshbuildervertex.h
            )
        fips_dir(model/n3util)
            fips_files(
                n3modeldata.h
            )
        fips_dir(model/skeletonutil)
            fips_files(
                skeletonbuilder.h
                skeletonbuildersaver.cc
                skeletonbuildersaver.h
            )
        fips_dir(texutil)
            fips_files(
                # cubefilterer.cc
                # cubefilterer.h
                # imageconverter.cc
                # imageconverter.h
                # textureattrs.cc
                # textureattrs.h
                # textureattrtable.cc
                # textureattrtable.h
                # textureconversionjob.cc
                # textureconversionjob.h
                textureconverter.cc
                textureconverter.h
            )   
        fips_dir(model/import/base)
            fips_files(
                modelimporter.cc
                modelimporter.h 
                scene.cc
                scene.h
                scenenode.cc
                scenenode.h
                uniquestring.cc
                uniquestring.h
            )

        fips_dir(model/import/fbx)
            fips_files(
                fbxfileimporter.cc 
                fbxfileimporter.h
            )

        fips_dir(model/import/fbx/node)
            fips_files(
                nfbxjointnode.cc
                nfbxjointnode.h
                nfbxlightnode.cc
                nfbxlightnode.h
                nfbxmeshnode.cc
                nfbxmeshnode.h
                nfbxnode.cc
                nfbxnode.h
                nfbxscene.cc
                nfbxscene.h
            )

        fips_dir(model/import/gltf)
            fips_files(
                gltfdata.h
                gltfreader.cc
                gltffileimporter.cc
                gltffileimporter.h
                gltffilematerialextractor.cc
                gltffilematerialextractor.h
            )
            fips_dir(model/import/gltf/node)
                fips_files(
                    ngltfmesh.cc
                    ngltfmesh.h
                    ngltfnode.cc
                    ngltfnode.h
                    ngltfscene.cc
                    ngltfscene.h
                    meshprimitive.cc
                    meshprimitive.h
                )
        fips_dir(game)
           fips_files(
               gameexporter.cc
               gameexporter.h
               levelexporter.cc
               levelexporter.h
           )

        fips_dir(model/modelutil)
            fips_files(
                clip.cc
                clip.h
                clipevent.cc
                clipevent.h
                modelattributes.cc
                modelattributes.h
                modelbuilder.cc
                modelbuilder.h
                modelconstants.cc
                modelconstants.h
                modeldatabase.cc
                modeldatabase.h
                modelphysics.cc
                modelphysics.h
                take.cc
                take.h
            )
        fips_dir(model)
            fips_files(
                animsplitterhelper.cc
                animsplitterhelper.h
                batchattributes.cc
                batchattributes.h
                binarymodelwriter.cc
                binarymodelwriter.h
                modelwriter.cc
                modelwriter.h
                scenewriter.cc
                scenewriter.h
                skinhelper.cc
                skinhelper.h
                xmlmodelwriter.cc
                xmlmodelwriter.h
            )
        fips_dir(surface)
            fips_files(
                surfaceexporter.cc
                surfaceexporter.h
                surfacebuilder.h
                surfacebuilder.cc
            )
        fips_dir(particle)
            fips_files(
                particleexporter.cc
                particleexporter.h
            )


	fips_dir(flatbuffers)
		nebula_flatc(LOCAL anim.fbs audio.fbs material.fbs mesh.fbs model.fbs particle.fbs physics.fbs scene.fbs skeleton.fbs texture.fbs)

    fips_deps(foundation render physics application db)
fips_end_lib()