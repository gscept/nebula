#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Crc
    
    Compute CRC checksums over a range of memory.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
class Crc
{
public:
    /// constructor
    Crc();
    /// begin computing a checksum
    void Begin();
    /// continue computing checksum
    void Compute(unsigned char* buf, unsigned int numBytes);
    /// finish computing the checksum
    void End();
    /// get result
    unsigned int GetResult() const;

private:
    static const unsigned int Key = 0x04c11db7;
    static const unsigned int NumByteValues = 256;
    static unsigned int Table[NumByteValues];
    static bool TableInitialized;

    bool inBegin;
    bool resultValid;
    unsigned int checksum;
};

} // namespace Util
//------------------------------------------------------------------------------
