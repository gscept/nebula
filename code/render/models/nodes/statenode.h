#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::MaterialStateNode
    
    Model node that handles a material and its states for each shader
	it's rendered with. It inherits StateNode to avoid recreating the texture
	handling functions, but bypasses the StateNode tag-parsing. 
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/
#include "models/nodes/transformnode.h"
#include "util/variant.h"
#include "resources/managedtexture.h"
#include "materials/materialtype.h"
#include "materials/materialinstance.h"
#include "materials/materialvariable.h"
#include "materials/managedsurface.h"

namespace Materials
{
	class MaterialInstance;
}
//------------------------------------------------------------------------------
namespace Models
{
class StateNode : public TransformNode
{
	__DeclareClass(StateNode);
public:
	/// constructor
	StateNode();
	/// destructor
	virtual ~StateNode();

	/// create a model node instance
	virtual Ptr<ModelNodeInstance> CreateNodeInstance() const;
	/// parse data tag (called by loader code)
	virtual bool ParseDataTag(const Util::FourCC& fourCC, const Ptr<IO::BinaryReader>& reader);
	/// called when resources should be loaded
	void LoadResources(bool sync);
	/// called when resources should be unloaded
	void UnloadResources();
	/// apply state shared by all my ModelNodeInstances
	virtual void ApplySharedState(IndexT frameIndex);

    /// get name of material
    const Util::StringAtom& GetMaterialName() const;
    /// get material surface from managed resource
    const Ptr<Materials::Surface>& GetMaterial() const;

	/// returns the resource state of the node, will only return true if all textures are loaded
	Resources::Resource::State GetResourceState() const;

protected:

	Resources::Resource::State stateLoaded;
    Util::StringAtom materialName;
    Ptr<Materials::ManagedSurface> managedMaterial;
	Util::Array<Util::KeyValuePair<Util::StringAtom, Util::Variant>> shaderParams;
}; 

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Materials::Surface>&
StateNode::GetMaterial() const
{
    return this->managedMaterial->GetSurface();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom& 
StateNode::GetMaterialName() const
{
	return this->materialName;
}

} // namespace Models
//------------------------------------------------------------------------------