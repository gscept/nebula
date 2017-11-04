//------------------------------------------------------------------------------
//  materialserver.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shader.h"
#include "materials/materialserver.h"
#include "materials/materialloader.h"
#include "models/nodes/statenode.h"
#include "resources/resourceid.h"
#include "util/keyvaluepair.h"
#include "util/string.h"
#include "util/array.h"
#include "graphics/batchgroup.h"
#include "io/ioserver.h"
#include "resources/resourcemanager.h"
#include "streamsurfaceloader.h"

namespace Materials
{
__ImplementClass(Materials::MaterialServer, 'MSRV', Core::RefCounted);
__ImplementSingleton(Materials::MaterialServer);

using namespace Resources;
using namespace Util;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
MaterialServer::MaterialServer() :
	isOpen(false)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
MaterialServer::~MaterialServer()
{
	if (this->IsOpen())
	{
		this->Close();
	}
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool 
MaterialServer::Open()
{
	n_assert(!this->IsOpen());
	this->isOpen = true;

    // load base materials first
    this->LoadMaterialPalette("base.xml");

    // get materials folder and load all
    Util::Array<Util::String> files = IO::IoServer::Instance()->ListFiles("mat:", "*.xml");
	files.EraseIndex(files.FindIndex("base.xml"));

    // load other materials
    IndexT i;
    for (i = 0; i < files.Size(); i++)
    {
        this->LoadMaterialPalette(files[i]);
    }

	Resources::ResourceManager::Instance()->RegisterStreamPool("sur", StreamSurfaceLoader::RTTI);
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialServer::Close()
{
	n_assert(this->IsOpen());
	for (int i = 0; i < this->materialPalettes.Size(); i++)
	{
		this->materialPalettes.ValueAtIndex(i)->Discard();
	}
	this->materialPalettes.Clear();
	this->materials.Clear();
}

//------------------------------------------------------------------------------
/**
*/
bool 
MaterialServer::IsOpen() const
{
	return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialServer::AddMaterial(const Ptr<Material>& material)
{
	n_assert(0 != material);
	this->materials.Add(material->GetName(), material);

	SizeT numPasses = material->GetNumPasses();
	IndexT passIndex;
	for (passIndex = 0; passIndex < numPasses; passIndex++)
	{
		// get node type
		const Material::MaterialPass& pass = material->GetPassByIndex(passIndex);

		if (this->materialsByBatchGroup.Contains(pass.code))
		{
			this->materialsByBatchGroup[pass.code].Append(material);
		}
		else
		{
			this->materialsByBatchGroup.Add(pass.code, Util::Array<Ptr<Material>>());
			this->materialsByBatchGroup[pass.code].Append(material);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<MaterialPalette>&
MaterialServer::LookupMaterialPalette(const Resources::ResourceName& name)
{
	if (!this->materialPalettes.Contains(name))
	{
		this->LoadMaterialPalette(name);
	}
	return this->materialPalettes[name];
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialServer::LoadMaterialPalette(const Resources::ResourceName& name)
{
	n_assert(!this->materialPalettes.Contains(name));
	Util::String path("mat:");
	path.Append(name.AsString());
	Ptr<MaterialPalette> materialPalette = MaterialLoader::LoadMaterialPalette(name, path);
	this->materialPalettes.Add(name, materialPalette);
}

//------------------------------------------------------------------------------
/**
*/
Materials::MaterialFeature::Mask
MaterialServer::FeatureStringToMask(const Util::String& str)
{
	return this->materialFeature.StringToMask(str);
}

//------------------------------------------------------------------------------
/**
*/
Util::String
MaterialServer::FeatureMaskToString(Materials::MaterialFeature::Mask mask)
{
	return this->materialFeature.MaskToString(mask);
}

} // namespace Material



