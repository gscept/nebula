"""
Bootstrap nebula build environment
"""

from mod import log, util, config, settings
import os
import sys
import shutil
import subprocess

def run(fips_dir, proj_dir, args) :
    """run the 'bootstrap' verb"""

    log.info("[BOOTSTRAP]: " + log.BLUE + "-- AnyFX --" + log.DEF)
    subprocess.call(proj_dir + "/fips anyfx setup force quiet", shell=True)

    # Get config build type
    # cfg = config.load(fips_dir, proj_dir, settings.get(proj_dir, 'config'))
    # build_type = cfg[0]['build_type'].lower()

    version = "linux"
    if sys.platform == "win32" :
        vswhere = os.path.expandvars("%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe")
        # catalog_productLine currently gives us Dev17 for example, which is currently the latest version.
        # Just replace the Dev with vc and were good. Note that this might not hold true forever!
        version = subprocess.check_output(vswhere + " -property catalog_productLine").decode("utf-8").rstrip()
        version = version.replace("Dev", "vc")

    log.info("[BOOTSTRAP]: " + log.BLUE + "-- PhysX --" + log.DEF)
    subprocess.call(proj_dir + '/fips physx build {} debug'.format(version), shell=True)
    subprocess.call(proj_dir + '/fips physx build {} release'.format(version), shell=True)

    log.info("[BOOTSTRAP]: " + log.BLUE + "-- UltraLight --" + log.DEF)
    subprocess.call(proj_dir + "/fips ultralight", shell=True)

    log.info("[BOOTSTRAP]: " + log.BLUE + "-- Python Deps --" + log.DEF)
    log.info(log.YELLOW + "Installing required python dependencies..." + log.DEF)

    if sys.platform == "win32":
        subprocess.call("pip install py7zr", stdout=subprocess.DEVNULL)
    elif sys.platform == "darwin":
        subprocess.call("brew install p7zip",  shell=True)
    
    # TODO: we should build the assetbatcher as well, without support for fbx unless we have the SDK installed.

    log.info("[BOOTSTRAP]: " + log.YELLOW + "Bootstrap finished.")

def help():
    """print 'bootstrap' help"""
    log.info(log.YELLOW +
             "fips bootstrap\n"
             "  Runs bootstrap for the Nebula dev environment\n")
