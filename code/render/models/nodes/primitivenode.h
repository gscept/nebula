#pragma once
//------------------------------------------------------------------------------
/**
    A primitive node contains a mesh resource and a primitive group id.

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
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
    uint32_t GetPrimitiveGroupIndex() const { return this->loadContext.primIndex; }
    /// get primitives mesh id
    CoreGraphics::MeshId GetMesh() const { return this->mesh; }


protected:
    friend class ModelLoader;

    /// load primitive
    virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;
    /// unload data
    virtual void Unload() override;

    Resources::ResourceId res;
    CoreGraphics::MeshId mesh;

    struct LoadContext
    {
        uint16 meshIndex;
        uint16 primIndex;
    } loadContext;
    bool loadSuccess = false;

    CoreGraphics::BufferId vbo, ibo;
    IndexT baseVboOffset, attributesVboOffset, iboOffset;
    CoreGraphics::IndexType::Code indexType;
    CoreGraphics::PrimitiveTopology::Code topology;
    CoreGraphics::PrimitiveGroup primGroup;
    CoreGraphics::VertexLayoutId vertexLayout;
};

} // namespace Models
