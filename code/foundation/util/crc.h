#pragma once
//------------------------------------------------------------------------------
/**
    @class  Util::Crc
    
    @brief  Compute CRC checksums over a range of memory.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
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
    template<typename sizetype> void Compute(unsigned char* buf, sizetype numBytes);
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

//template<> void Crc::Compute(unsigned char* buf, int numBytes);
//template<> void Crc::Compute(unsigned char* buf, __int64 numBytes);
} // namespace Util
//------------------------------------------------------------------------------
