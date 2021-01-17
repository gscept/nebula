#pragma once
//------------------------------------------------------------------------------
/**
    Adds a light probe component to graphics entities
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
namespace Graphics
{
class LightProbeContext : public GraphicsContext
{
    _DeclareContext();
public:
    /// constructor
    LightProbeContext();
    /// destructor
    virtual ~LightProbeContext();


    /// get transform
    static const Math::mat4& GetTransform(const Graphics::GraphicsEntityId id);
private:

    typedef Ids::IdAllocator<
        Math::mat4,             // projection
        Math::mat4              // view-transform
    > LightProbeAllocator;
    static LightProbeAllocator lightProbeAllocator;

    /// allocate a new slice for this context
    static ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(ContextEntityId id);
};
} // namespace Graphics