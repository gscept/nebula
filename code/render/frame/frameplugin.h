#pragma once
//------------------------------------------------------------------------------
/**
    Runs a plugin, which is a code-callback
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "util/stringatom.h"
#include <functional>
namespace Frame
{
class FramePlugin : public FrameOp
{
public:
    /// constructor
    FramePlugin();
    /// destructor
    virtual ~FramePlugin();

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const IndexT frameIndex, const IndexT bufferIndex) override;
        void Discard();

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif

        std::function<void(IndexT, IndexT)> func;
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

private:


    void Build(
        Memory::ArenaAllocator<BIG_CHUNK>& allocator,
        Util::Array<FrameOp::Compiled*>& compiledOps,
        Util::Array<CoreGraphics::EventId>& events,
        Util::Array<CoreGraphics::BarrierId>& barriers,
        Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
        Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures,
        CoreGraphics::CommandBufferPoolId commandBufferPool) override;
};


/// add function callback to global dictionary
extern void AddCallback(const Util::StringAtom name, std::function<void(IndexT, IndexT)> func);
/// get algorithm function call
extern const std::function<void(IndexT, IndexT)>& GetCallback(const Util::StringAtom& str);
/// initialize the plugin table
extern void InitPluginTable();

extern Util::Dictionary<Util::StringAtom, std::function<void(IndexT, IndexT)>> nameToFunction;


} // namespace Frame2
