#pragma once
//------------------------------------------------------------------------------
/**
    @class Materials::MaterialType
  
    MaterialType is a central registry for material templates.
    It is used to sort objects per material which results in objects with 
    equal shaders to render in order, meaning it's used by the visibility system
    to put model node instances into buckets depending on which material type 
    their surface material uses.

    This class converts (and maps) material template names to a zero indexed
    fixed table which is constant during the execution of the application.
    Material types are then used by models to register themselves in 
    the visibility system, which can then be accessed by retrieving 
    all visible model nodes which are supposed to be rendered with a certain
    material is encountered.

    Basically, the rendering hierarchy looks like this:

    FrameBatch
        BatchGroup 'FlatGeometryLit'
            For material in materials which is registered with BatchGroup 'FlatGeometryLit'
                Apply shader for BatchGroup 'FlatGeometryLit'
                For model in visible models for material->MaterialType                    
                    For model node in visible model nodes in model for material->MaterialType
                        Apply mesh for model node
                        For model node instance in visible model node instances in model node for material->MaterialType
                            Apply per instance variables
                            Draw


    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/stringatom.h"
#include "util/array.h"
#include "util/dictionary.h"

//------------------------------------------------------------------------------
namespace Materials
{
class MaterialType
{
public:
	/// readable name of ModelNodeMaterial
	typedef Util::StringAtom Name;
	/// binary code of ModelNodeMaterial
	typedef IndexT Code;

	/// convert from string
	static Code FromName(const Name& name);
	/// convert to string
	static Name ToName(Code c);
	/// maximum number of different ModelNodeMaterials
	static const IndexT MaxNumMaterialTypes = 1024;
	/// invalid model node material code
	static const IndexT InvalidMaterialType = InvalidIndex;

private:
	friend class MaterialServer;

	/// constructor
	MaterialType();

	Util::Dictionary<Name, IndexT> nameToCode;
	Util::Array<Name> codeToName;
};

} // namespace Models
//------------------------------------------------------------------------------
