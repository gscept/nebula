
//------------------------------------------------------------------------------
//  callbacks.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxPhysicsAPI.h"
#include "callbacks.h"
#include "util/string.h"

using namespace physx;

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/    
void
ErrorCallback::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
{
    const char* errorCode = NULL;

    switch (code)
    {
        case PxErrorCode::eNO_ERROR:
            errorCode = "no error";
        break;
        case PxErrorCode::eINVALID_PARAMETER:
            errorCode = "invalid parameter";
        break;
        case PxErrorCode::eINVALID_OPERATION:
            errorCode = "invalid operation";
        break;
        case PxErrorCode::eOUT_OF_MEMORY:
            errorCode = "out of memory";
        break;
        case PxErrorCode::eDEBUG_INFO:
            errorCode = "info";
        break;
        case PxErrorCode::eDEBUG_WARNING:
            errorCode = "warning";
        break;
        case PxErrorCode::ePERF_WARNING:
            errorCode = "performance warning";
        break;
        case PxErrorCode::eABORT:
            errorCode = "abort";
        break;
        case PxErrorCode::eINTERNAL_ERROR:
            errorCode = "internal error";
        break;
        case PxErrorCode::eMASK_ALL:
            errorCode = "unknown error";
        break;
    }

    n_assert(errorCode);
    if(errorCode)
    {
        Util::String errorMessage = Util::String::Sprintf("%s (%d): PhysX: %s : %s\n", file, line, errorCode, message);
    
        if(code == PxErrorCode::eABORT)
        {
            n_error(errorMessage.AsCharPtr());
        }
        else
        {
            n_dbgout(errorMessage.AsCharPtr());
        }
    }
}
}
