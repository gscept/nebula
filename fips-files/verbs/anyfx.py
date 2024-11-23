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
    elif sys.platform == "darwin":
        target = "anyfxcompiler-darwin/anyfxcompiler"
    else:
        target = "anyfxcompiler-linux/anyfxcompiler"        
    if len(args) > 0 :
        noun = args[0]
        if noun == 'setup' :
            force = False
            if len(args) > 1 :
                if args[1] == 'force' :
                    force = True
            target = util.fix_path(proj_dir + "/../fips-deploy/fips-anyfx/" + target)
            if not os.path.isfile(target) or force:
                log.info(log.YELLOW + "Compiling anyfxcompiler")
                if sys.platform == "win32" :
                    ext = ".bat"
                elif sys.platform == "darwin":
                    ext = "-macos.sh"
                else:
                    ext = "-linux.sh"

                if (len(args) > 2 and args[2] == 'quiet'):
                    ret_code = subprocess.call(proj_dir + "/../fips-anyfx/anyfxcompiler/build{}".format(ext), stdout=subprocess.DEVNULL)
                else:
                    ret_code = subprocess.call(proj_dir + "/../fips-anyfx/anyfxcompiler/build{}".format(ext))
                    
                if ret_code == 0:
                    log.info(log.GREEN + "AnyFX compiler built successfully.")
                else:
                    log.info(log.RED + "AnyFX compiler failed to build!")
            else:
                log.info(log.YELLOW + "AnyFX compiler already built, skipping")

def help():
    """print 'anyfx' help"""
    log.info(log.YELLOW +
             "fips anyfx setup [force] [quiet]\n"
             "  compiles anyfxcompiler for platform\n")
