#pragma once
//------------------------------------------------------------------------------
/**
    A primitive node contains a mesh resource and a primitive group id.

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/bbox.h"
#include "coregraphics/primitivegroup.h"
#include "resources/resourceid.h"
#include "shaderstatenode.h"
#include "coregraphics/mesh.h"
namespace Models
{
class PrimitiveNode : public ShaderStateNode
{
public:
    /// constructor
    PrimitiveNode();
    /// destructor
    virtual ~PrimitiveNode();

    /// get the nodes primitive group index
    uint32_t GetPrimitiveGroupIndex() const { return this->primitiveGroupIndex; }
    /// get primitives mesh id
    CoreGraphics::MeshId GetMeshId() const { return this->res; }

    /// Get function to apply node 
    std::function<void(const CoreGraphics::CmdBufferId)> GetApplyFunction() override;
    /// Get function to fetch primitive group
    std::function<const CoreGraphics::PrimitiveGroup()> GetPrimitiveGroupFunction() override;

protected:
    friend class ModelLoader;

    /// load primitive
    virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;
    /// unload data
    virtual void Unload() override;

    Resources::ResourceId res;
    CoreGraphics::MeshId mesh;
    uint16_t primitiveGroupIndex;

    CoreGraphics::BufferId vbo, ibo;
    IndexT vboOffset, iboOffset;
    CoreGraphics::PrimitiveTopology::Code topology;
    CoreGraphics::PrimitiveGroup primGroup;
    CoreGraphics::VertexLayoutId vertexLayout;
};

} // namespace Models
