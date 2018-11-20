"""control nebula toolkit settings

nebula work [working directory]
nebula toolkit [toolkit directory]
"""

from mod import log, util
import os
import sys

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
             "      work [working directory]\n"
             "      toolkit [nebula root/toolkit directory]\n"
             "fips nebula\n"
             "  prints current configuration")
