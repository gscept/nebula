#pragma once
//------------------------------------------------------------------------------
/**
    Handles callbacks for downscaling passes

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
namespace PostEffects
{

class DownsamplingContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext()
public:
    /// Constructor
    DownsamplingContext();
    /// Destructor
    ~DownsamplingContext();

    /// create context
    static void Create();
    /// discard context
    static void Discard();

    /// setup bloom context
    static void Setup();

    /// Handle window resize
    static void Resize(const uint framescriptHash, SizeT width, SizeT height);

private:

    /// implement an empty alloc
    static Graphics::ContextEntityId Alloc() { return Graphics::InvalidContextEntityId; }
    /// implement a dummy dealloc
    static void Dealloc(Graphics::ContextEntityId id) {};

};

} // namespace PostEffects
