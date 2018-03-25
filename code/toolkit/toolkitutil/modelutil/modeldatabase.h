#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ModelDatabase
    
    Holds dictionary of import options, is also responsible for loading and saving them
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/dictionary.h"
#include "util/string.h"
#include "core/singleton.h"
#include "core/refcounted.h"
#include "modelattributes.h"
#include "modelconstants.h"
#include "modelphysics.h"

namespace ToolkitUtil
{
class ModelDatabase: public Core::RefCounted
	
{
	__DeclareSingleton(ModelDatabase);
	__DeclareClass(ModelDatabase);
public:

	/// constructor
	ModelDatabase();
	/// destructor
	~ModelDatabase();

	/// opens the database
	void Open();
	/// closes the database
	void Close();
	/// returns true if database is open
	bool IsOpen() const;

	/// performs lookup on model attributes, loads from file if it doesn't exist
	Ptr<ModelAttributes> LookupAttributes(const Util::String& name, bool reload = false);
	/// checks if attributes exist
	bool AttributesExist(const Util::String& name);
	/// gets name of model attributes pointer
	const Util::String& GetAttributesName(const Ptr<ModelAttributes>& attrs);

	/// performs lookup on physics attributes, loads from file if it doesn't exist
	Ptr<ModelPhysics> LookupPhysics(const Util::String& name, bool reload = false);
	/// checks if attributes exist
	bool PhysicsExist(const Util::String& name);
	/// gets name of model attributes pointer
	const Util::String& GetPhysicsName(const Ptr<ModelPhysics>& attrs);

	/// performs lookup on model constants, loads from file if it doesn't exist
	Ptr<ModelConstants> LookupConstants(const Util::String& name, bool reload = false);
	/// checks if constants exist
	bool ConstantsExist(const Util::String& name);
	/// gets name of model constants
	const Util::String& GetConstantsName(const Ptr<ModelConstants>& constants);

private:
	bool isOpen;
	Util::Dictionary<Util::String, Ptr<ModelAttributes>> modelAttributes;
	Util::Dictionary<Util::String, Ptr<ModelConstants>> modelConstants;
	Util::Dictionary<Util::String, Ptr<ModelPhysics>> modelPhysics;
};

//------------------------------------------------------------------------------
/**
*/
inline bool 
ModelDatabase::IsOpen() const
{
	return this->isOpen;
}

}