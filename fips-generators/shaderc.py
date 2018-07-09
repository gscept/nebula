"""
Compiles the .afx files into shader binaries and .json frame and material declarations to binary
"""
Version = 1

import os
import shutil
import platform
import genutil
import subprocess
from mod import log

#-------------------------------------------------------------------------------
def get_shaderc_path(path) :
    #path = os.path.dirname(os.path.abspath(__file__))
    return path + '/shaderbatcher.exe'

# if you want to enable the more complex object api add this    
#'--gen-object-api',
# it will require unpack functions for nebula types
#-------------------------------------------------------------------------------
def run_shaderc(input_file, args) :
    cmd = [
        input_file,
        '-out', args
    ]    
    print("Compiling shaders, materials and frame scripts...")
    subprocess.call(cmd)

#-------------------------------------------------------------------------------
def generate(input, out_src, out_hdr, args) :
    shaderc_path = get_shaderc_path(input)
    run_shaderc(shaderc_path, args)
