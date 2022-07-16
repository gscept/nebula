#pragma once
//------------------------------------------------------------------------------
/**
    Static UI Context

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "ultralight/ultralightrenderer.h"

namespace StaticUI
{

class StaticUIContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    StaticUIContext();
    /// destructor
    virtual ~StaticUIContext();

    /// Create context
    static void Create();
    /// Discard context
    static void Discard();

    /// Register function
    static void RegisterFunction(const char* name, std::function<void()> func);
};



} // namespace StaticUI
