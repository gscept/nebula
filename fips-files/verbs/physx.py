"""build physx binaries

build [win-vs15,winvs16]
deploy [win-vs15,winvs16]
"""

from mod import log, util, settings, config
import os
import sys
import shutil
import subprocess
import glob

    
def run(fips_dir, proj_dir, args) :
    """run the 'physx' verb"""
    if len(args) > 0 :
        noun = args[0]            
        if noun == 'build' :
            if sys.platform == "win32" :
                # FIXME all of this only works on windows at the moment and is super hacky
                
                if len(args) != 2 :
                    log.error("expected compiler target (win-vs15, win-vs16)")
                    
                    
                preset = util.fix_path(os.path.dirname(os.path.abspath(__file__))) + "/physx-presets/" +"fips" + args[1] + ".xml"
                if not os.path.isfile(preset) :
                    log.error("Unrecognized compiler target")
                shutil.copy2(preset, proj_dir + "/../physx/physx/buildtools/presets/")
                subprocess.call(proj_dir+"/../physx/physx/generate_projects.bat fips" + args[1])
                
                # figure out a version number for vswhere
                version = args[1][6:]
                version_next = str(int(version) +1)
                version = version+".0,"+version_next+".0"
                #use vswhere to figure out where vs is
                devenvPath = subprocess.check_output(proj_dir+"/../physx/externals/vswhere/vswhere -version [" + version + "] -property productPath").decode("utf-8").rstrip()
                devenvPath = util.fix_path(devenvPath)
                if not os.path.isfile(devenvPath) :
                    #try to find msbuild
                    log.info("Could not find Visual Studio, trying to find lastest version of MSBuild..." + devenvPath)
                    devenvPath = subprocess.check_output(proj_dir+"/../physx/externals/vswhere/vswhere -version [" + version + "] -products * -requires Microsoft.Component.MSBuild -property installationPath").decode("utf-8").rstrip()
                    devenvPath = util.fix_path(devenvPath + "/MSBuild/Current/Bin/MSBuild.exe")
                    if not os.path.isfile(devenvPath):
                        log.error("Could not detect Visual Studio installation.")
                    log.optional("Using MSBuild from", devenvPath)

                    log.info("Compiling PhysX, this might take a while.")
                    log.info("Building debug version...")
                    #-noConsoleLogger
                    retcode = subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/fips" + args[1] +"/INSTALL.vcxproj /verbosity:quiet /noLogo /noConsoleLogger /property:Configuration=Debug")
                    if retcode == 0:
                        log.colored(log.GREEN, "PhysX debug build completed.")
                    else:
                        log.colored(log.RED, "PhysX debug build failed!")

                    log.info("Building release version...")
                    retcode = subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/fips" + args[1] +"/INSTALL.vcxproj /verbosity:quiet /noLogo /noConsoleLogger /property:Configuration=Release")
                    if retcode == 0:
                        log.colored(log.GREEN, "PhysX release build completed.")
                    else:
                        log.colored(log.RED, "PhysX release build failed!")
                else:
                    log.optional("Using Visual Studio from", devenvPath)

                    log.info("Compiling PhysX, this might take a while...")
                    log.info("Building debug version...")
                    retcode = subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/fips" + args[1] +"/PhysXSDK.sln /Build debug /Project INSTALL")
                    if retcode == 0:
                        log.colored(log.GREEN, "PhysX debug build completed.")
                    else:
                        log.colored(log.RED, "PhysX debug build failed!")

                    log.info("Building release version...")
                    retcode = subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/fips" + args[1] +"/PhysXSDK.sln /Build release /Project INSTALL")
                    if retcode == 0:
                        log.colored(log.GREEN, "PhysX release build completed.")
                    else:
                        log.colored(log.RED, "PhysX release build failed!")
            else :
                preset = util.fix_path(os.path.dirname(os.path.abspath(__file__))) + "/physx-presets/fipslinux.xml"
                shutil.copy2(preset, proj_dir + "/../physx/physx/buildtools/presets/")
                subprocess.run([proj_dir+"/../physx/physx/generate_projects.sh", "fipslinux"])
                subprocess.run(["make", "-j", "10"], cwd=proj_dir + "/../physx/physx/compiler/fipslinux-checked")
                subprocess.run(["make", "install"], cwd=proj_dir + "/../physx/physx/compiler/fipslinux-checked")
        if noun == 'deploy' :
            if sys.platform == "win32" : 
                ps_deploy = util.get_workspace_dir(fips_dir) + "/fips-deploy/physx/bin/"
                cur_cfg = settings.get(proj_dir, "config")
                cfg = config.load(fips_dir, proj_dir, cur_cfg)[0]
                px_target = cfg['defines']['PX_TARGET']
                target_dir = util.get_deploy_dir(fips_dir, util.get_project_name_from_dir(proj_dir), cur_cfg)            
                
                dllFiles = glob.glob(ps_deploy + px_target + "/debug/*.dll")
                log.info("Looking for PhysX dlls in '{}/'".format(ps_deploy + px_target))
                if not dllFiles:
                    log.error("PhysX debug dlls not found! Have you built them? (fips physx build [compiler target])")
                else:
                    for dll in dllFiles :
                        shutil.copy2(dll, target_dir)
                    log.colored(log.GREEN, "Deployed PhysX Debug binaries to '{}'".format(target_dir))
                
                dllFiles = glob.glob(ps_deploy + px_target + "/release/*.dll")
                if not dllFiles:
                    log.error("PhysX release dlls not found! Have you built them? (fips physx build [compiler target])")
                else:
                    for dll in dllFiles :
                        shutil.copy2(dll, target_dir)
                    log.colored(log.GREEN, "Deployed PhysX release binaries to '{}'".format(target_dir))
    else:
        def run(fips_dir,proj_dir,args):
            log.error("Not supported")            

def help():
    """print 'physx' help"""
    log.info(log.YELLOW +
             "fips physx build [win-vs15, win-vs16]\n"             
             "  builds PhysX dlls with the given toolchain\n"
             "fips physx deploy\n"
             "  copies PhysX dlls to the project's deploy dir")
