//------------------------------------------------------------------------------
//  meshbuildervertex.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "toolkitutil/meshutil/meshbuildervertex.h"

namespace ToolkitUtil
{
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
MeshBuilderVertex::MeshBuilderVertex() :
    compMask(0),
    flagMask(0)
{
    IndexT i;
    for (i = 0; i < compsSize; i++)
    {
        this->comps[i].set(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderVertex::SetComponent(ComponentIndex compIndex, const vec4& value)
{
    n_assert((compIndex >= 0) && (compIndex < NumComponents));
    
    // both versions of a component maps to the same index within the array.
    const int i = (compIndex + 1) / 2;
    this->comps[i] = value;
    this->compMask |= (1<<compIndex);
}

//------------------------------------------------------------------------------
/**
*/
const vec4&
MeshBuilderVertex::GetComponent(ComponentIndex compIndex) const
{
    n_assert((compIndex >= 0) && (compIndex < NumComponents));
    const int i = (compIndex + 1) / 2;
    return this->comps[i];
}

//------------------------------------------------------------------------------
/**
    Returns +1 if this vertex is greater then other vertex, -1 if 
    this vertex is less then other vertex, 0 if vertices
    are equal.
*/
int
MeshBuilderVertex::Compare(const MeshBuilderVertex& rhs) const
{
    IndexT i;
    for (i = 0; i < NumComponents; i++)
    {
        ComponentMask mask = (1<<i);
        if ((this->compMask & mask) && (rhs.compMask & mask))
        {
            const int idx = (i + 1) / 2;
            const vec4& f0 = this->comps[idx];
            const vec4& f1 = rhs.comps[idx];
            if (greater_any(f0, f1))
            {
                return 1;
            }
            else if (less_any(f0, f1))
            {
                return -1;
            }
        }
    }
    // fallthrough: all components equal
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderVertex::operator==(const MeshBuilderVertex& rhs) const
{
    return (0 == this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderVertex::operator!=(const MeshBuilderVertex& rhs) const
{
    return (0 != this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderVertex::operator<(const MeshBuilderVertex& rhs) const
{
    return (-1 == this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderVertex::operator>(const MeshBuilderVertex& rhs) const
{
    return (+1 == this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderVertex::Transform(const mat4& m)
{
    if (this->compMask & CoordBit)
    {
        this->SetComponent(CoordIndex, m * this->GetComponent(CoordIndex));
    }
    if (this->compMask & NormalBit || this->compMask & NormalB4NBit)
    {
        this->SetComponent(NormalIndex, m * this->GetComponent(NormalIndex));
    }
    if (this->compMask & TangentBit || this->compMask & TangentB4NBit)
    {
        this->SetComponent(TangentIndex, m * this->GetComponent(TangentIndex));
    }
    if (this->compMask & BinormalBit || this->compMask & BinormalB4NBit)
    {
        this->SetComponent(BinormalIndex, m * this->GetComponent(BinormalIndex));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderVertex::InitComponents(ComponentMask mask)
{
    this->compMask |= mask;
    IndexT i;
    for (i = 0; i < NumComponents; i++)
    {
        if (mask & (1<<i))
        {
            const int idx = (i + 1) / 2;
            switch (i)
            {
                case CoordIndex:    
                    this->comps[idx].set(0.0f, 0.0f, 0.0f, 1.0f);
                    break;

                case NormalIndex:
                case NormalB4NIndex:
                case TangentIndex:
                case TangentB4NIndex:
                case BinormalIndex:
                case BinormalB4NIndex:
                    this->comps[idx].set(0.0f, 1.0f, 0.0f, 0.0f);
                    break;

                case Uv0Index:
                case Uv0S2Index:
                case Uv1Index:
                case Uv1S2Index:
                case Uv2Index:
                case Uv2S2Index:
                case Uv3Index:
                case Uv3S2Index:
                case WeightsIndex:
                case WeightsUB4NIndex:
                case JIndicesIndex:
                case JIndicesUB4Index:
                    this->comps[idx].set(0.0f, 0.0f, 0.0f, 0.0f);
                    break;

                case ColorIndex:
                case ColorUB4NIndex:
                    this->comps[idx].set(1.0f, 1.0f, 1.0f, 1.0f);
                    break;
            }
        }
    }
}

} // namespace ToolkitUtil