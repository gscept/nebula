#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshBuilderVertex
    
    Contains per-vertex data in a mesh builder object.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/vec4.h"
#include "math/mat4.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class MeshBuilderVertex
{
public:
    /// vertex component indices
    /// @note   Normalized version is (and should) always be right after the normal version. (ex. NormalIndex + 1 = NormalB4NIndex)
    enum ComponentIndex
    {
        CoordIndex = 0,
        NormalIndex,
		NormalB4NIndex,
        Uv0Index,
		Uv0S2Index,
        Uv1Index,
        Uv1S2Index,
        Uv2Index,
        Uv2S2Index,
        Uv3Index,
        Uv3S2Index,
		TangentIndex,
		TangentB4NIndex,
		BinormalIndex,
		BinormalB4NIndex,
        ColorIndex,
        ColorUB4NIndex,
        WeightsIndex,
		WeightsUB4NIndex,
        JIndicesIndex,
		JIndicesUB4Index,
        NumComponents,
        InvalidComponentIndex,
    };

    /// vertex component bits
    enum ComponentBit
    {
        CoordBit = (1<<CoordIndex),
        NormalBit = (1<<NormalIndex),
		NormalB4NBit = (1<<NormalB4NIndex),
        Uv0Bit = (1<<Uv0Index),
		Uv0S2Bit = (1<<Uv0S2Index),
        Uv1Bit = (1 << Uv1Index),
        Uv1S2Bit = (1 << Uv1S2Index),
        Uv2Bit = (1 << Uv2Index),
        Uv2S2Bit = (1 << Uv2S2Index),
        Uv3Bit = (1 << Uv3Index),
        Uv3S2Bit = (1 << Uv3S2Index),
		TangentBit = (1<<TangentIndex),
		TangentB4NBit = (1<<TangentB4NIndex),
		BinormalBit = (1<<BinormalIndex),
		BinormalB4NBit = (1<<BinormalB4NIndex),
        ColorBit = (1 << ColorIndex),
        ColorUB4NBit = (1 << ColorUB4NIndex),
        WeightsBit = (1<<WeightsIndex),
		WeightsUB4NBit = (1<<WeightsUB4NIndex),
        JIndicesBit = (1<<JIndicesIndex),
		JIndicesUB4Bit = (1<<JIndicesUB4Index),
    };
    /// a mask of ComponentBits
    typedef uint ComponentMask;

    /// vertex flags
    enum Flag
    {
        Redundant = (1<<0),
    };
    /// a mask of Flags
    typedef uint FlagMask;

    /// constructor
    MeshBuilderVertex();

    /// equality operator
    bool operator==(const MeshBuilderVertex& rhs) const;
    /// inequality operator
    bool operator!=(const MeshBuilderVertex& rhs) const;
    /// less-then operator
    bool operator<(const MeshBuilderVertex& rhs) const;
    /// greather-then operator
    bool operator>(const MeshBuilderVertex& rhs) const;

    /// set a vertex component
    void SetComponent(ComponentIndex compIndex, const Math::vec4& value);
    /// get a vertex component index
    const Math::vec4& GetComponent(ComponentIndex compIndex) const;
    /// check if all components are valid
    bool HasComponents(ComponentMask compMask) const;
	/// check if component exists.
	bool HasComponent(ComponentBit c) const;
    /// set components to their default values
    void InitComponents(ComponentMask compMask);
    /// delete component
    void DeleteComponents(ComponentMask compMask);
    /// get component mask
    ComponentMask GetComponentMask() const;
	/// get number of valid floats
	int GetWidth() const;

    /// set vertex flag
    void SetFlag(Flag f);
    /// unset vertex flag
    void UnsetFlag(Flag f);
    /// check vertex flag
    bool CheckFlag(Flag f) const;

    /// compare vertex against other, return -1, 0 or +1
    int Compare(const MeshBuilderVertex& rhs) const;
    /// transform the vertex
    void Transform(const Math::mat4& m);

private:
    ComponentMask compMask;
    FlagMask flagMask;

    /// we only allow one component of each type. That means, ex. the normal and normalb4n maps to the same index in the comps array.
    static const int compsSize = NumComponents / 2 + 1;
    Math::vec4 comps[compsSize];
};

//------------------------------------------------------------------------------
/**
*/
inline bool
MeshBuilderVertex::HasComponents(ComponentMask mask) const
{
    return (mask == (this->compMask & mask));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MeshBuilderVertex::HasComponent(ComponentBit c) const
{
	return ((this->compMask & c) == c);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBuilderVertex::DeleteComponents(ComponentMask mask)
{
    this->compMask &= ~mask;
}

//------------------------------------------------------------------------------
/**
*/
inline MeshBuilderVertex::ComponentMask
MeshBuilderVertex::GetComponentMask() const
{
    return this->compMask;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBuilderVertex::SetFlag(Flag f)
{
    this->flagMask |= f;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBuilderVertex::UnsetFlag(Flag f)
{
    this->flagMask &= ~f;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MeshBuilderVertex::CheckFlag(Flag f) const
{
    return (0 != (this->flagMask & f));
}

//------------------------------------------------------------------------------
/**
*/
inline int
MeshBuilderVertex::GetWidth() const
{
	int w=0;
	if(this->HasComponent(CoordBit))
		w+=3;

	if(this->HasComponent(NormalBit))
		w+=3;

	if(this->HasComponent(NormalB4NBit))
		w+=1;

	if(this->HasComponent(TangentBit))
		w+=3;

	if(this->HasComponent(TangentB4NBit))
		w+=1;

	if(this->HasComponent(BinormalBit))
		w+=3;

	if(this->HasComponent(BinormalB4NBit))
		w+=1;

	if(this->HasComponent(ColorBit))
		w+=4;
	
	if(this->HasComponent(ColorUB4NBit))
		w+=1;

	if(this->HasComponent(Uv0Bit))
		w+=2;

	if(this->HasComponent(Uv0S2Bit))
		w+=1;

	if(this->HasComponent(Uv1Bit))
		w+=2;

	if(this->HasComponent(Uv1S2Bit))
		w+=1;

	if(this->HasComponent(Uv2Bit))
		w+=2;

	if(this->HasComponent(Uv2S2Bit))
		w+=1;

	if(this->HasComponent(Uv3Bit))
		w+=2;

	if(this->HasComponent(Uv3S2Bit))
		w+=1;

	if(this->HasComponent(WeightsBit))
		w+=4;

	if(this->HasComponent(WeightsUB4NBit))
		w+=1;

	if(this->HasComponent(JIndicesBit))
		w+=4;

	if(this->HasComponent(JIndicesUB4Bit))
		w+=1;

	return w;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    