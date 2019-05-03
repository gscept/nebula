"""control nebula toolkit settings

nebula work [working directory]
nebula toolkit [toolkit directory]
physx [win-vs15,winvs16]
cleannidl
"""

from mod import log, util, settings
import os
import sys
import shutil
import subprocess

if sys.platform == "win32" :
    if sys.version_info.major > 2:
        import winreg as _winreg
    else:
        import _winreg
        

    base_reg = r"SOFTWARE\gscept\ToolkitShared"

    def argToKey(key) :
        """translate argument to registry key"""
        keys = {
            "work" : "workdir",
            "toolkit" : "path"
            }
        return keys.get(key,"")
    
    def setKey(key, value) :
        """set nebula key"""
        try :
            if key == "work" or key == "toolkit" :
                path = os.path.abspath(value)
                _winreg.CreateKey(_winreg.HKEY_CURRENT_USER,base_reg)            
                reg_key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER, base_reg, 0, _winreg.KEY_WRITE)
                _winreg.SetValueEx(reg_key, argToKey(key), 0, _winreg.REG_SZ, path)
                _winreg.CloseKey(reg_key)
        except WindowsError:
            log.error("error setting registry key")
    
    def run(fips_dir, proj_dir, args) :
        """run the 'nebula' verb"""
        if len(args) > 0 :
            noun = args[0]
            if noun == 'set' :
                if len(args) > 2 :
                    setKey(args[1], args[2])
                else :
                    log.error("expected setting and value")
            elif noun == 'physx' :
            
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
                log.info("Building checked version")
                subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/fips" + args[1] +"/PhysXSDK.sln /Build checked /Project INSTALL")
                log.info("Building release version")
                subprocess.call(devenvPath + " " + proj_dir+"/../physx/physx/compiler/fips" + args[1] +"/PhysXSDK.sln /Build release /Project INSTALL")                
            elif noun == 'get' :
                if len(args) > 1 :
                    key = argToKey(args[1])
                    if key != "" :
                        reg_key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER, base_reg, 0, _winreg.KEY_READ)
                        keyval, regtype = _winreg.QueryValueEx(reg_key,key)
                        _winreg.CloseKey(reg_key)
                        log.info(keyval)
                    else :
                        log.error("invalid setting")
                else :
                    log.error("expected setting name")
            elif noun == 'cleannidl' :
                proj = util.get_project_name_from_dir(proj_dir)
                cfg = settings.get(proj_dir, 'config')
                path = util.get_build_dir(fips_dir,proj,cfg)+"/nidl"                
                shutil.rmtree(path,True)
        else :
            try:
                reg_key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER, base_reg, 0, _winreg.KEY_READ)
                workval, regtype = _winreg.QueryValueEx(reg_key,"workdir")
                rootval, regtype = _winreg.QueryValueEx(reg_key,"path")
                _winreg.CloseKey(reg_key)
                log.info(log.YELLOW +
                    "Current settings:\n"
                    "Project directory: " + workval + "\n"
                    "Nebula root directory: " + rootval + "\n")
            except WindowsError:
                log.info(log.YELLOW + "No Nebula settings in registry\n")
else:
    def run(fips_dir,proj_dir,args):
        log.error("Not supported")

def help():
    """print 'nebula' help"""
    log.info(log.YELLOW +
             "fips nebula [set|get]\n"
             "  work [working directory]\n"
             "  toolkit [nebula root/toolkit directory]\n"
             "fips nebula\n"
             "  prints current configuration"
             "fips cleannidl\n"
             "  cleans all nidl files which forces a regeneration upon compilation")
