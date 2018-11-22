Version = 58

import sys
if __name__ == '__main__':
	sys.path.insert(0,'../fips/generators')		

import genutil as util
import IDLC

#-------------------------------------------------------------------------------
def generate(input, out_src, out_hdr) :
    if util.isDirty(Version, [input], [out_src, out_hdr]) :
        idlc = IDLC.IDLCodeGenerator()
        
        idlc.SetVersion(Version)

        idlc.SetDocument(input)
        generateSource = idlc.GenerateHeader(out_hdr)
        # reset document
        idlc.SetDocument(input)
        idlc.GenerateSource(out_src, out_hdr)

if __name__ == '__main__':	
	globals()
	generate(sys.argv[1],sys.argv[2],sys.argv[3])