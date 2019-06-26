"""Build and deploy anyfxcompiler
setup
"""

from mod import log, util, settings
import os
import sys
import shutil
import subprocess

def run(fips_dir, proj_dir, args) :
    """run the 'anyfx' verb"""
    if sys.platform == "win32" :
        target = "anyfxcompiler-windows/anyfxcompiler.exe"        
    else :
        target = "anyfxcompiler-linux/anyfxcompiler"        
    if len(args) > 0 :
        noun = args[0]
        if noun == 'setup' :
            target = util.fix_path(proj_dir + "/../fips-deploy/fips-anyfx/" + target)
            if not os.path.isfile(target) :
                log.info(log.YELLOW + "Compiling anyfxcompiler")
                if sys.platform == "win32" :
                    subprocess.call(proj_dir + "/../fips-anyfx/anyfxcompiler/build.bat")
                else :
                    subprocess.call(proj_dir + "/../fips-anyfx/anyfxcompiler/build.sh")                
            else :
                log.info(log.YELLOW + "anyfxcompiler already built, skipping")

def help():
    """print 'anyfx' help"""
    log.info(log.YELLOW +
             "fips anyfx setup\n"
             "  compiles anyfxcompiler for platform\n")
