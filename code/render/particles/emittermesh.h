#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::EmitterMesh
    
    An emitter mesh holds positions and normals for particle emission.
    The actual format of a vertex in the emitter mesh is:

    vec4 position;
    vec4 normal;
    vec4 tangent;
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "coregraphics/mesh.h"
#include "util/fixedarray.h"

//------------------------------------------------------------------------------
namespace Particles
{
class EmitterMesh
{
public:
    /// constructor
    EmitterMesh();
    /// destructor
    ~EmitterMesh();
    
    /// setup the emitter mesh
    void Setup(const CoreGraphics::MeshId mesh, IndexT primGroupIndex);
    /// discard the emitter mesh
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;
    
    struct EmitterPoint
    {
        Math::vec4 position;
        Math::vec4 normal;
        Math::vec4 tangent;
    };

    /// get emitter point
    const EmitterPoint& GetEmitterPoint(IndexT key) const;

private:
    // width in number of vec4's...
    static const SizeT VertexWidth = sizeof(EmitterPoint) / sizeof(Math::vec4);
    SizeT numPoints;
    EmitterPoint* points;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
EmitterMesh::IsValid() const
{
    return (0 != this->points);
}

} // namespace Particles
//------------------------------------------------------------------------------
