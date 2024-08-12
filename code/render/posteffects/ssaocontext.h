#pragma once
//------------------------------------------------------------------------------
/**
    SSAO post effect

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace PostEffects
{

class SSAOContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    SSAOContext();
    /// destructor
    virtual ~SSAOContext();


    /// create context
    static void Create();
    /// discard context
    static void Discard();

    /// setup bloom context
    static void Setup();

    /// update view resources
    static void UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

    /// Update when window resized
    static void WindowResized(const CoreGraphics::WindowId id, SizeT width, SizeT height);
private:

    /// implement an empty alloc
    static Graphics::ContextEntityId Alloc() { return Graphics::InvalidContextEntityId; }
    /// implement a dummy dealloc
    static void Dealloc(Graphics::ContextEntityId id) {};

};

} // namespace PostEffects
