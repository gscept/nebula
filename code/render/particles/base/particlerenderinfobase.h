#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleRenderInfo
    
    ParticleRenderInfo objects are returned by the ParticleRenderer singleton
    when a visible particle system is attached. The caller needs to store
    this object and needs to hand it back to the ParticleRenderer when
    actually rendering of the particle system needs to happen.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "particles/particle.h"

//------------------------------------------------------------------------------
namespace Base
{
class ParticleRenderInfoBase
{
public:
    /// default constructor
    ParticleRenderInfoBase();
    /// clear the object
    void Clear();
    /// empty?
    bool IsEmpty() const;

    /// constructor
    ParticleRenderInfoBase(IndexT baseVertexIndex, SizeT numVertices);
    /// get base vertex index
    IndexT GetBaseVertexIndex() const;
    /// get number of vertices
    SizeT GetNumVertices() const;

private:
    IndexT baseVertexIndex;
    SizeT numVertices;
};

//------------------------------------------------------------------------------
/**
*/
inline
ParticleRenderInfoBase::ParticleRenderInfoBase() 
:   baseVertexIndex(0),
    numVertices(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
ParticleRenderInfoBase::ParticleRenderInfoBase(IndexT base, SizeT num) :
    baseVertexIndex(base),
    numVertices(num)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
ParticleRenderInfoBase::Clear()
{
    this->baseVertexIndex = 0;
    this->numVertices = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
ParticleRenderInfoBase::GetBaseVertexIndex() const
{
    return this->baseVertexIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
ParticleRenderInfoBase::GetNumVertices() const
{
    return this->numVertices;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
ParticleRenderInfoBase::IsEmpty() const
{
    return !this->numVertices;
}

} // namespace Base
//------------------------------------------------------------------------------
    
