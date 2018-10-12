//------------------------------------------------------------------------------
//  crc.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/crc.h"

namespace Util
{
unsigned int Crc::Table[NumByteValues] = { 0 };
bool Crc::TableInitialized = false;

//------------------------------------------------------------------------------
/**
*/
Crc::Crc() :
    inBegin(false),
    resultValid(false),
    checksum(0)
{
    // first-time byte table initialization
    if (!TableInitialized)
    {
        unsigned int i;
        for (i = 0; i < NumByteValues; ++i)
        {
            unsigned int reg = i << 24;
            int j;
            for (j = 0; j < 8; ++j)
            {
                bool topBit = (reg & 0x80000000) != 0;
                reg <<= 1;
                if (topBit)
                {
                    reg ^= Key;
                }
            }
            Table[i] = reg;
        }
        TableInitialized = true;
    }
}

//------------------------------------------------------------------------------
/**
    Begin computing a CRC checksum over several chunks of data. This can be
    done in multiple runs, which is the only useful way to compute
    checksum for large files.
*/
void
Crc::Begin()
{
    n_assert(!this->inBegin);    
    this->resultValid = false;
    this->inBegin = true;
    this->checksum = 0;
}

//------------------------------------------------------------------------------
/**
    Do one run of checksum computation for a chunk of data. Must be 
    executed inside Begin()/End().
*/
void
Crc::Compute(unsigned char* buf, unsigned int numBytes)
{
    n_assert(this->inBegin);
    unsigned int i;
    for (i = 0; i < numBytes; i++)
    {
        uint top = this->checksum >> 24;
        top ^= buf[i];
        this->checksum = (this->checksum << 8) ^ Table[top];
    }
}

//------------------------------------------------------------------------------
/**
    End checksum computation. This validates the result, so that
    it can be accessed with GetResult().
*/
void
Crc::End()
{
    n_assert(this->inBegin);
    this->inBegin = false;
    this->resultValid = true;
}

//------------------------------------------------------------------------------
/**
    Get the result of the checksum computation. Must be executed
    after End().
*/
unsigned int
Crc::GetResult() const
{
    n_assert(!this->inBegin);
    n_assert(this->resultValid);
    return this->checksum;
}

} // namespace Util
