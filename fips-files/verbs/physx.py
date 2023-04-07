"""build physx binaries

build [win-vs15,win-vs16]
deploy [config]
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
                
                if len(args) != 3 :
                    log.error("expected compiler target [vc15, vc16, vc17] [debug, checked, release]")
                
                buildmapping = {"debug" : "Debug", "checked" : "Checked", "release":"Release" } 
                if args[2] not in buildmapping :
                    log.error("unknown build configuration, should be debug, checked or release")
                buildconfig = buildmapping[args[2]]
                    
                vcstring = args[1] + "win64"
                physxbuild = os.path.abspath(proj_dir + "/../physx/physx")
                subprocess.run([physxbuild + "/generate_projects.bat", vcstring] , cwd = physxbuild)
                
                vswhere = os.path.expandvars("%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe")
                # figure out a version number for vswhere
                version = args[1][2:]              
                versionrange = "[" + version + ".0," + str(int(version) + 1) + "]"
                #use vswhere to figure out where vs is
                devenvPath = subprocess.check_output(vswhere + " -version " + versionrange + " -property productPath").decode("utf-8").rstrip()
                devenvPath = util.fix_path(devenvPath)
                if not os.path.isfile(devenvPath) :
                    #try to find msbuild
                    log.info("Could not find Visual Studio, trying to find lastest version of MSBuild..." + devenvPath)
                    devenvPath = subprocess.check_output(vswhere + " -latest -products * -requires Microsoft.Component.MSBuild -property installationPath").decode("utf-8").rstrip()
                    devenvPath = util.fix_path(devenvPath + "/MSBuild/Current/Bin/MSBuild.exe")
                    if not os.path.isfile(devenvPath):
                        log.error("Could not detect Visual Studio installation.")
                    log.optional("Using MSBuild from", devenvPath)

                    log.info("Compiling PhysX, this might take a while.")
                    log.info("Building " + buildconfig + " version...")
                    #-noConsoleLogger
                    retcode = subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/" + vcstring +"/INSTALL.vcxproj /verbosity:quiet /noLogo /noConsoleLogger /property:Configuration=" + buildconfig)
                    if retcode == 0:
                        log.colored(log.GREEN, "PhysX " + buildconfig + " build completed.")
                    else:
                        log.colored(log.RED, "PhysX " + buildconfig + " build failed!")
                else:
                    log.optional("Using Visual Studio from", devenvPath)

                    log.info("Compiling PhysX, this might take a while...")
                    log.info("Building " + buildconfig + " version...")
                    retcode = subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/" + vcstring +"/PhysXSDK.sln /Build " + args[2] + " /Project INSTALL")
                    if retcode == 0:
                        log.colored(log.GREEN, "PhysX " + buildconfig + " build completed.")
                    else:
                        log.colored(log.RED, "PhysX debug " + buildconfig + " failed!")
            else :
                subprocess.run([proj_dir+"/../physx/physx/generate_projects.sh", "linux"], cwd=proj_dir + "/../physx/physx/")
                subprocess.run(["make", "-j", "10"], cwd=proj_dir + "/../physx/physx/compiler/linux-checked")
                subprocess.run(["make", "install"], cwd=proj_dir + "/../physx/physx/compiler/linux-checked")
        if noun == 'deploy' :
            if sys.platform == "win32" : 
                if len(args) != 3 :
                    log.error("expected compiler and config [vc15, vc16, vc17] [debug, checked, release]")
                
                physxconfig = args[2]
                
                cur_cfg = settings.get(proj_dir, 'config')
                
                vcmapping = {"vc15":"win.x86_64.vc141.mt", "vc16":"win.x86_64.vc142.mt", "vc17":"win.x86_64.vc143.mt"}
                if args[1] not in vcmapping :
                    log.error("unknown compiler target, should be [vc15,vc16,vc17]")
                px_target = vcmapping[args[1]]

                ps_deploy = util.get_workspace_dir(fips_dir) + "/fips-deploy/physx/bin/"
                cfg = config.load(fips_dir, proj_dir, cur_cfg)[0]
                target_dir = util.get_deploy_dir(fips_dir, util.get_project_name_from_dir(proj_dir), cur_cfg)
                
                dllFiles = glob.glob(ps_deploy + px_target + "/" + physxconfig + "/*.dll")
                log.info("Looking for PhysX dlls in '{}/'".format(ps_deploy + px_target))
                if not dllFiles:
                    log.error("PhysX dlls not found! Have you built them? (fips physx build [compiler target] [config])")
                else:
                    for dll in dllFiles :
                        shutil.copy2(dll, target_dir)
                    log.colored(log.GREEN, "Deployed PhysX binaries to '{}'".format(target_dir))
                
    else:
        def run(fips_dir,proj_dir,args):
            log.error("Not supported") 

def help():
    """print 'physx' help"""
    log.info(log.YELLOW +
             "fips physx build [linux, vc15, vc16, vc17] [debug, checked, release]\n"
             "  builds PhysX dlls with the given toolchain\n"
             "fips physx deploy [vc15, vc16, vc17] [debug, checked, release]\n"
             "  copies PhysX dlls to the project's deploy dir")
