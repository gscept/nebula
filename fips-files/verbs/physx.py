"""build physx binaries

build [vc17,vc16,vc15] [debug,checked,release]
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
                
                vcversions = ["vc15", "vc16", "vc17"]
                if args[1] not in vcversions :
                    log.error("unknown vc version configuration, should be vc15, vc16 or vc17")

                vcstring = args[1] + "win64-cpu-only-nebula"
                physxbuild = os.path.abspath(proj_dir + "/../physx/physx")
                subprocess.run([physxbuild + "/generate_projects.bat", vcstring] , cwd = physxbuild) #, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

                vswhere = os.path.expandvars("%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe")
                # figure out a version number for vswhere
                version = args[1][2:]              
                versionrange = "[" + version + ".0," + str(int(version) + 1) + "]"
                
                devenvPath = subprocess.check_output(vswhere + " -latest -products * -requires Microsoft.Component.MSBuild -property installationPath").decode("utf-8").rstrip()
                devenvPath = util.fix_path(devenvPath + "/MSBuild/Current/Bin/MSBuild.exe")
                if not os.path.isfile(devenvPath):
                    log.info("Could not find MSBuild, trying to find lastest version of Visual Studio...")
                    devenvPath = subprocess.check_output(vswhere + " -version " + versionrange + " -property productPath").decode("utf-8").rstrip()
                    devenvPath = util.fix_path(devenvPath)
                    if not os.path.isfile(devenvPath):
                        log.error("Could not detect Visual Studio installation.")
                    
                    log.optional("Using Visual Studio from", devenvPath)

                    log.info(log.YELLOW + "Compiling PhysX, this might take a while..." + log.DEF)
                    log.info("Building " + buildconfig + " version...")
                    retcode = subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/" + vcstring +"/PhysXSDK.sln /Build " + args[2] + " /Project INSTALL")
                    if retcode == 0:
                        log.colored(log.GREEN, "PhysX " + buildconfig + " build completed.")
                    else:
                        log.colored(log.RED, "PhysX debug " + buildconfig + " failed!")
                else:
                    log.optional("Using MSBuild from", devenvPath)

                    log.info(log.YELLOW + "Compiling PhysX, this might take a while..." + log.DEF)
                    log.info("Building " + buildconfig + " version...")
                    retcode = subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/" + vcstring +"/INSTALL.vcxproj /verbosity:quiet /noLogo /noConsoleLogger /property:Configuration=" + buildconfig)
                    if retcode == 0:
                        log.colored(log.GREEN, "PhysX " + buildconfig + " build completed.")
                    else:
                        log.colored(log.RED, "PhysX " + buildconfig + " build failed!")
            else :
                subprocess.run([proj_dir+"/../physx/physx/generate_projects.sh", "linux-clang-cpu-only-nebula"], cwd=proj_dir + "/../physx/physx/")
                subprocess.run(["make", "-j", "10"], cwd=proj_dir + "/../physx/physx/compiler/linux-checked")
                subprocess.run(["make", "install"], cwd=proj_dir + "/../physx/physx/compiler/linux-checked")
    else:
        help()
        def run(fips_dir,proj_dir,args):
            log.error("Not supported") 

def help():
    """print 'physx' help"""
    log.info(log.YELLOW +
             "fips physx [build, deploy] [linux, vc15, vc16, vc17] [debug, checked, release]\n"
             "  builds PhysX dlls with the given toolchain\n"
             )
