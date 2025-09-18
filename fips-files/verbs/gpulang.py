"""Build and deploy anyfxcompiler
setup
"""

from mod import log, util, settings
import os
import sys
import shutil
import subprocess

def run(fips_dir, proj_dir, args) :
    """run the 'gpulang' verb"""
    if sys.platform == "win32" :
        target = "gpulangc-windows/gpulangc.exe"        
    else :
        target = "gpulangc-linux/gpulangc"        
    if len(args) > 0 :
        noun = args[0]
        if noun == 'setup' :
            force = False
            if len(args) > 1 :
                if args[1] == 'force' :
                    force = True
            target = util.fix_path(proj_dir + "/../fips-deploy/gpulang/" + target)
            if not os.path.isfile(target) or force:
                log.info(log.YELLOW + "Compiling GPULang Compiler")
                if sys.platform == "win32" :
                    ext = ".bat"
                else:
                    ext = ".sh"

                if (len(args) > 2 and args[2] == 'quiet'):
                    ret_code = subprocess.call(proj_dir + "/../gpulang/gpulangcompiler/build{}".format(ext), stdout=subprocess.DEVNULL)
                else:
                    ret_code = subprocess.call(proj_dir + "/../gpulang/gpulangcompiler/build{}".format(ext))

                if ret_code == 0:
                    log.info(log.GREEN + "GPULang compiler built successfully.")
                else:
                    log.info(log.RED + "GPULang compiler failed to build!")
            else:
                log.info(log.YELLOW + "GPULang compiler already built, skipping")

def help():
    """print 'gpulang' help"""
    log.info(log.YELLOW +
             "fips gpulang setup [force] [quiet]\n"
             "  compiles gpulang for platform\n")
