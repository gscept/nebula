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
            
            # FIXME all of this only works on windows at the moment and is super hacky
            
            if len(args) != 2 :
                log.error("expected compiler target (win-vs15, win-vs16)")
                
                
            preset = util.fix_path(os.path.dirname(os.path.abspath(__file__))) + "/physx-presets/" +"fips" + args[1] + ".xml"
            if not os.path.isfile(preset) :
                log.error("unrecognized compiler target")                                
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
                log.error("could not detect visual studio installation")
            log.info("Using Visual Studio from" + devenvPath)
            log.info("Compiling PhysX, this might take a while")
            log.info("Building debug version")
            subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/fips" + args[1] +"/PhysXSDK.sln /Build debug /Project INSTALL")
            log.info("Building release version")
            subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/fips" + args[1] +"/PhysXSDK.sln /Build release /Project INSTALL")                            
        if noun == 'deploy' :            
            ps_deploy = util.get_workspace_dir(fips_dir) + "/fips-deploy/physx/bin/"
            cur_cfg = settings.get(proj_dir, "config")
            cfg = config.load(fips_dir, proj_dir, cur_cfg)[0]
            px_target = cfg['defines']['PX_TARGET']
            target_dir = util.get_deploy_dir(fips_dir, util.get_project_name_from_dir(proj_dir), cur_cfg)            
            for dll in glob.glob(ps_deploy + px_target + "/debug/*.dll") :
                shutil.copy2(dll, target_dir)
            for dll in glob.glob(ps_deploy + px_target + "/release/*.dll") :
                shutil.copy2(dll, target_dir)
    else:
        def run(fips_dir,proj_dir,args):
            log.error("Not supported")            

def help():
    """print 'physx' help"""
    log.info(log.YELLOW +
             "fips physx build [win-vs15,win-vs16]\n"             
             "  builds physx dlls with the given toolchain\n"
             "fips physx deploy\n"
             "  copys physx dlls to the projects deploy dir")
