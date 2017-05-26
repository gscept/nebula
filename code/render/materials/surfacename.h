#pragma once
//------------------------------------------------------------------------------
/**
    @class Materials::SurfaceName
    
    SurfaceName is a unique and consistent registry used for surfaces.

    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/stringatom.h"
#include "util/array.h"
#include "util/dictionary.h"

//------------------------------------------------------------------------------
namespace Materials
{
class SurfaceName
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
	static const IndexT MaxNumSurfaceNames = 1028;
	/// invalid model node material code
	static const IndexT InvalidSurfaceName = InvalidIndex;

private:
    friend class MaterialServer;

	/// constructor
    SurfaceName();

	Util::Dictionary<Name, IndexT> nameToCode;
	Util::Array<Name> codeToName;
};

} // namespace Models
//------------------------------------------------------------------------------
