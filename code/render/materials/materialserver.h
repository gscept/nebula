#pragma once
//------------------------------------------------------------------------------
/**
	The material server is the central hub for the material types

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "materialtype.h"
#include "materialpool.h"
#include "ids/id.h"
#include "io/uri.h"
namespace Materials
{

ID_32_32_NAMED_TYPE(MaterialId, typeId, instanceId);

class MaterialServer : public Core::RefCounted
{
	__DeclareClass(MaterialServer);
	__DeclareSingleton(MaterialServer);
public:

	/// constructor
	MaterialServer();
	/// destructor
	~MaterialServer();

	/// open server
	void Open();
	/// close server (frees all materials and types)
	void Close();

	/// load material types from file
	bool LoadMaterialTypes(const IO::URI& file);

	/// allocate an instance of a material
	MaterialId AllocateMaterial(const Resources::ResourceName& type);
	/// deallocate material instance
	void DeallocateMaterial(const MaterialId id);
	/// set material constant
	void SetMaterialConstant(const MaterialId id, const Util::StringAtom& name, const Util::Variant& val);
	/// set material texture
	void SetMaterialTexture(const MaterialId id, const Util::StringAtom& name, const CoreGraphics::TextureId val);

private:
	friend class MaterialPool;

	Util::Dictionary<Resources::ResourceName, MaterialType*> materialTypesByName;
	Util::Array<MaterialType> materialTypes;
	Ptr<MaterialPool> materialPool;
	bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline MaterialId
CreateMaterial(const Resources::ResourceName& type)
{
	return MaterialServer::Instance()->AllocateMaterial(type);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DeallocateMaterial(const MaterialId id)
{
	MaterialServer::Instance()->DeallocateMaterial(id);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialSetConstant(const MaterialId id, const Util::StringAtom& name, const Util::Variant& val)
{
	MaterialServer::Instance()->SetMaterialConstant(id, name, val);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialSetTexture(const MaterialId id, const Util::StringAtom& name, const CoreGraphics::TextureId val)
{
	MaterialServer::Instance()->SetMaterialTexture(id, name, val);
}
} // namespace Materials
