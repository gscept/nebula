#------------------------------------------------------------------------------
##
#   Generates a .vscode/anyfx_properties.json file with provided arguments
#
import sys
import os
import json
import pathlib

if len(sys.argv) > 2:
    filename = sys.argv[1]

    data = dict()
    if os.path.isfile(filename):
        f = open(filename) 
        data = json.load(f)
        f.close()

    if 'includeDirs' not in data:
        data['includeDirs'] = []

    for i in range(2, len(sys.argv)):
        includeDir = sys.argv[i]

        # Make sure we only same directory once
        if includeDir not in data['includeDirs']:
            data['includeDirs'].append(includeDir)
    
    folder = os.path.dirname(filename)
    pathlib.Path(folder).mkdir(parents=True, exist_ok=True)
    with open(filename, 'w') as outfile:
        json.dump(data, outfile)

else:
    print("Too few arguments!")