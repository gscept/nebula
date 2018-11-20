// NIDL #version:57#
#ifdef _WIN32
#define NOMINMAX
#endif
//------------------------------------------------------------------------------
//  basegameprotocol.cc
//  (C) Individual contributors, see AUTHORS file
//
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "basegameprotocol.h"
#include "scripting/bindings.h"
//------------------------------------------------------------------------------
namespace Msg
{
PYBIND11_EMBEDDED_MODULE(basegameprotocol, m)
{
    m.doc() = "namespace Msg";
    m.def("SetLocalTransform", &SetLocalTransform::Send, "Set the local transform of an entity.");
    m.def("SetWorldTransform", &SetWorldTransform::Send, "Set the world transform on an entity");
}
} // namespace Msg
