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
#include "coregraphics/shader.h"
#include "materials/materialserver.h"
#include "transformnode.h"
#include "jobs/jobs.h"
#include "coregraphics/buffer.h"
#include "coregraphics/resourcetable.h"

namespace Visibility
{
void VisibilitySortJob(const Jobs::JobFuncContext& ctx);
}

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
    static const uint NumTables = 1;
    static const uint NumMaxOffsets = 4; // object, instancing, skeleton, particles

    struct DrawPacket
    {
        Materials::SurfaceInstanceId surfaceInstance;
        SizeT numTables;
        CoreGraphics::ResourceTableId tables[NumTables];
        uint32 numOffsets[NumTables];
        uint32 offsets[NumTables][NumMaxOffsets];
        IndexT slots[NumTables];

#ifndef PUBLIC_BUILD
        uint32_t nodeInstanceHash;
        Math::bbox boundingBox;
#endif


        /// apply the resource tables and offsets
        void Apply(Materials::MaterialType* type);
    };

    /// get surface
    const Materials::SurfaceId GetSurface() const { return this->surface; };
    /// trigger an LOD update
    void SetMaxLOD(const float lod);

protected:
    friend class ModelContext;
    friend class StreamModelCache;

    friend void Visibility::VisibilitySortJob(const Jobs::JobFuncContext& ctx);

    /// load shader state
    bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;
    /// unload data
    virtual void Unload() override;
    /// called when loading finished
    virtual void OnFinishedLoading();
    
    Materials::MaterialType* materialType;
    Materials::SurfaceId surface;
    Materials::SurfaceResourceId surRes;
    Resources::ResourceName materialName;

    uint8_t objectTransformsIndex;
    uint8_t instancingTransformsIndex;
    uint8_t skinningTransformsIndex;

    CoreGraphics::ResourceTableId resourceTable;
};

} // namespace Models
