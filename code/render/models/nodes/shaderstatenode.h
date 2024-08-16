#pragma once
//------------------------------------------------------------------------------
/**
    The shader state node wraps the shader associated with a certain primitive node,
    or group of primitive nodes.
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "resources/resourceid.h"
#include "transformnode.h"
#include "coregraphics/resourcetable.h"

namespace Models
{
class ShaderStateNode : public TransformNode
{
public:
    /// constructor
    ShaderStateNode();
    /// destructor
    virtual ~ShaderStateNode();

    struct DrawPacket;
    static const uint NumMaxOffsets = 4; // object, instancing, skeleton, particles

    struct DrawPacket
    {
#ifndef PUBLIC_BUILD
        Math::bbox boundingBox;
        uint32_t nodeInstanceHash;
#endif
        Materials::MaterialInstanceId materialInstance;
        CoreGraphics::ResourceTableId table;
        uint32 offsets[NumMaxOffsets];
        uint8 numOffsets;
        uint8 slot;

        /// Apply the resource table
        void Apply(const CoreGraphics::CmdBufferId cmdBuf, IndexT batchIndex, IndexT bufferIndex);
    };

    /// get surface
    const Materials::MaterialId GetMaterial() const { return this->material; };

    /// Create resource table
    static Util::FixedArray<CoreGraphics::ResourceTableId> CreateResourceTables();

protected:
    friend class ModelContext;
    friend class ModelLoader;

    /// load shader state
    bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;
    /// unload data
    virtual void Unload() override;
    /// called when loading finished
    virtual void OnFinishedLoading(ModelStreamingData* streamingData) override;
    
    Resources::ResourceId materialRes;
    Materials::MaterialId material;
    Resources::ResourceName materialName;

    Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
};

} // namespace Models
