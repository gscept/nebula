"""control nebula toolkit settings

nebula work [working directory]
nebula toolkit [toolkit directory]
cleannidl
"""

from mod import log, util, settings, project, config
import os
import sys
import shutil
import subprocess

keys = {
        "work" : "workdir",
        "toolkit" : "path"
        }

def argToKey(key) :
        """translate argument to registry key"""
        return keys[key]

if sys.platform == "win32" :
    if sys.version_info.major > 2:
        import winreg as _winreg
    else:
        import _winreg
        
    base_reg = r"SOFTWARE\gscept\ToolkitShared"
 
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
                    log.error("Expected setting and value")            
            elif noun == 'get' :
                if len(args) > 1 :
                    key = argToKey(args[1])
                    if key != "" :
                        reg_key = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER, base_reg, 0, _winreg.KEY_READ)
                        keyval, regtype = _winreg.QueryValueEx(reg_key,key)
                        _winreg.CloseKey(reg_key)
                        log.info(keyval)
                    else :
                        log.error("Invalid setting")
                else :
                    log.error("Expected setting name")
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
                log.colored(log.YELLOW, "Current settings")
                log.optional("Project directory", workval)
                log.optional("Nebula root director", rootval)
            except WindowsError:
                log.colored(log.YELLOW, "No Nebula settings in registry\n")
else:
    import json
    from os.path import expanduser

    def storeProjectPath(fips_dir, proj_dir, path) :
        if os.path.isdir(path) :
            proj = util.get_project_name_from_dir(proj_dir)
            cfg = settings.get(proj_dir, 'config')
            build_dir = util.get_build_dir(fips_dir,proj,cfg)
            os.makedirs(build_dir, exist_ok=True)
            with open(build_dir + '/project_root.txt', "w") as outfile :
                workdir = path = os.path.abspath(path)
                outfile.write(workdir)
                outfile.close()

    def setKey(cfg_file, key, value) :
        """set nebula key"""
        if not os.path.isfile(cfg_file):
            folder = expanduser("~") + "/.config/nebula/"
            os.makedirs(folder, exist_ok=True)
            # create dummy data
            jsn = {}
            jsn["ToolkitShared"] = {}
            jsn["ToolkitShared"]["workdir"] = ""
            jsn["ToolkitShared"]["path"] = ""
            with open(cfg_file, 'w') as json_out:
                json.dump(jsn, json_out)
            
        with open(cfg_file) as old_json_file:
            old_json = json.load(old_json_file)
            path = os.path.abspath(value)
            old_json["ToolkitShared"][key] = path
            with open(cfg_file, 'w') as new_json_file:
                json.dump(old_json, new_json_file)
        
    def getKey(cfg_file, key) :
         with open(cfg_file) as json_file:
            cfg = json.load(json_file)
            toolkit_shared = cfg['ToolkitShared']
            return toolkit_shared[key]

    def run(fips_dir,proj_dir,args):
        """run the 'nebula' verb"""
        try:
            cfg_file = expanduser("~") + "/.config/nebula/gscept.cfg"
            if len(args) > 0 :
                noun = args[0]
                if noun == 'set' :
                    if len(args) == 3 :
                        key = argToKey(args[1])
                        setKey(cfg_file, key, args[2])
                        if key == 'workdir' :
                            storeProjectPath(fips_dir, proj_dir, args[2])
                    else :
                        log.error("Expected setting and value")            
                elif noun == 'get' :
                    if len(args) == 2 :
                        key = argToKey(args[1])
                        value = getKey(cfg_file, key)
                        log.info(value)
            else:
                workval = getKey(cfg_file, "workdir")
                rootval = getKey(cfg_file, "path")
                log.colored(log.YELLOW, "Current settings")
                log.optional("Project directory", workval)
                log.optional("Nebula root director", rootval)
        except (RuntimeError, KeyError):
            log.error("failed manipulating settings")

def help():
    """print 'nebula' help"""
    log.info(log.YELLOW +
             "fips nebula [set|get]\n"
             "  work [working directory]\n"
             "  toolkit [nebula root/toolkit directory]\n"
             "fips nebula\n"
             "  prints current configuration\n"
             "fips cleannidl\n"
             "  cleans all nidl files which forces a regeneration upon compilation\n")
