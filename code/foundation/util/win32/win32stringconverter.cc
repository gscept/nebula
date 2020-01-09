//------------------------------------------------------------------------------
//  win32stringconverter.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "win32stringconverter.h"

namespace Win32
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
SizeT
Win32StringConverter::UTF8ToWide(const String& src, ushort* dst, SizeT dstMaxBytes)
{
    return UTF8ToWide(src.AsCharPtr(), dst, dstMaxBytes);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
Win32StringConverter::UTF8ToWide(const char* src, ushort* dst, SizeT dstMaxBytes)
{
    n_assert((0 != src) && (0 != dst) && (dstMaxBytes > 2) && ((dstMaxBytes & 1) == 0));
    SizeT numConv = MultiByteToWideChar(CP_UTF8, 0, src, -1, (LPWSTR) dst, (dstMaxBytes / 2) - 1);
    if (numConv > 0)
    {
        dst[numConv] = 0;
        return numConv;
    }
    else
    {
        DWORD errCode = GetLastError();
        String errMessage;
        switch (errCode)
        {
            case ERROR_INSUFFICIENT_BUFFER: errMessage.Format("ERROR_INSUFFICIENT_BUFFER dstMaxBytes: %d", dstMaxBytes); break;
            case ERROR_INVALID_FLAGS: errMessage.Format("ERROR_INVALID_FLAGS"); break;
            case ERROR_INVALID_PARAMETER: errMessage.Format("ERROR_INVALID_PARAMETER: May occur if src and dst are the same pointer."); break;
            case ERROR_NO_UNICODE_TRANSLATION: errMessage.Format("ERROR_NO_UNICODE_TRANSLATION... should never happen."); break;
            default: errMessage = "Unknown Error";
        }
        n_error("Win32StringConverter::UTF8ToWide() failed to convert string '%s' to wide char! Error '%s'", src, errMessage.AsCharPtr());
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
Win32StringConverter::WideToUTF8(ushort* src, SizeT length)
{
    n_assert(0 != src);
    String returnString;
    char dstBuf[1024];
    int numBytesWritten = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) src, length, dstBuf, sizeof(dstBuf), 0, 0);
    if (numBytesWritten > 0)
    {
        n_assert(numBytesWritten < sizeof(dstBuf));
        returnString.Set(dstBuf, numBytesWritten);
        return returnString;
    }
    else
    {
        n_error("Win32StringConverter::WideToUTF8(): failed to convert string!");
        return 0;
    }
}

} // namespace Win32