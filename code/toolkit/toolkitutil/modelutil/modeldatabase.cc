//------------------------------------------------------------------------------
//  importdatabase.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/modelutil/modeldatabase.h"
#include "io/uri.h"
#include "util/string.h"
#include "io/ioserver.h"

using namespace IO;
using namespace Util;
namespace ToolkitUtil
{

__ImplementSingleton(ModelDatabase);
__ImplementClass(ToolkitUtil::ModelDatabase, 'IMDB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ModelDatabase::ModelDatabase() :
	isOpen(false)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ModelDatabase::~ModelDatabase()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelDatabase::Open()
{
	n_assert(!this->IsOpen());
	this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelDatabase::Close()
{
	n_assert(this->IsOpen());
    IndexT i;
    for (i = 0; i < this->modelAttributes.Size(); i++)
    {
        this->modelAttributes.ValueAtIndex(i)->Clear();
    }
	this->modelAttributes.Clear();
    for (i = 0; i < this->modelConstants.Size(); i++)
    {
        this->modelConstants.ValueAtIndex(i)->Clear();
    }
	this->modelConstants.Clear();
    for (i = 0; i < this->modelPhysics.Size(); i++)
    {
        this->modelPhysics.ValueAtIndex(i)->Clear();
    }
	this->modelPhysics.Clear();
	this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelAttributes>
ModelDatabase::LookupAttributes(const Util::String& name, bool reload)
{
	if (!this->modelAttributes.Contains(name))
	{
		// create new model attributes
		Ptr<ModelAttributes> attrs = ModelAttributes::Create();

		// set the name of the attribute
		attrs->SetName(name);

		// check if file exists, if so, load it, otherwise use the base object
		if (this->AttributesExist(name))
		{
			// create uri
			String path;
			path.Format("src:assets/%s.attributes", name.AsCharPtr());

			// create stream
			Ptr<Stream> file = IoServer::Instance()->CreateStream(path);
			attrs->Load(file);
		}

		// lastly add it to the list
		this->modelAttributes.Add(name, attrs);

		// return attributes
		return attrs;
	}
	else if (reload)
	{
		Ptr<ModelAttributes> attrs = this->modelAttributes[name];

		// clear attributes
		attrs->Clear();

		// check if file exists, if so, load it, otherwise use the base object
		if (this->AttributesExist(name))
		{
			// create uri
			String path;
			path.Format("src:assets/%s.attributes", name.AsCharPtr());

			// create stream
			Ptr<Stream> file = IoServer::Instance()->CreateStream(path);
			attrs->Load(file);
		}

		return attrs;
	}
	else
	{
		return this->modelAttributes[name];
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelDatabase::AttributesExist(const Util::String& name)
{
	// format file
	String file;
	file.Format("src:assets/%s.attributes", name.AsCharPtr());

	// basically check if the file system has the attribute requested
	if (IoServer::Instance()->FileExists(file))
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
ModelDatabase::GetAttributesName(const Ptr<ModelAttributes>& attrs)
{
	IndexT index = this->modelAttributes.ValuesAsArray().FindIndex(attrs);
	n_assert(index != InvalidIndex);
	return this->modelAttributes.KeysAsArray()[index];
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelPhysics>
ModelDatabase::LookupPhysics(const Util::String& name, bool reload)
{
	if (!this->modelPhysics.Contains(name))
	{
		// create new model attributes
		Ptr<ModelPhysics> phys = ModelPhysics::Create();

		// set the name of the attribute
		phys->SetName(name);

		// check if file exists, if so, load it, otherwise use the base object
		if (this->PhysicsExist(name))
		{
			// create uri
			String path;
			path.Format("src:assets/%s.physics", name.AsCharPtr());

			// create stream
			Ptr<Stream> file = IoServer::Instance()->CreateStream(path);
			phys->Load(file);
		}

		// lastly add it to the list
		this->modelPhysics.Add(name, phys);

		// return attributes
		return phys;
	}
	else if (reload)
	{
		Ptr<ModelPhysics> phys = this->modelPhysics[name];

		// clear attributes
		phys->Clear();

		// check if file exists, if so, load it, otherwise use the base object
		if (this->PhysicsExist(name))
		{
			// create uri
			String path;
			path.Format("src:assets/%s.physics", name.AsCharPtr());

			// create stream
			Ptr<Stream> file = IoServer::Instance()->CreateStream(path);
			phys->Load(file);
		}

		return phys;
	}
	else
	{
		return this->modelPhysics[name];
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelDatabase::PhysicsExist(const Util::String& name)
{
	// format file
	String file;
	file.Format("src:assets/%s.physics", name.AsCharPtr());

	// basically check if the file system has the attribute requested
	if (IoServer::Instance()->FileExists(file))
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
ModelDatabase::GetPhysicsName(const Ptr<ModelPhysics>& attrs)
{
	IndexT index = this->modelPhysics.ValuesAsArray().FindIndex(attrs);
	n_assert(index != InvalidIndex);
	return this->modelPhysics.KeysAsArray()[index];
}


//------------------------------------------------------------------------------
/**
*/
Ptr<ModelConstants>
ModelDatabase::LookupConstants(const Util::String& name, bool reload)
{
	if (!this->modelConstants.Contains(name))
	{
		// create new model attributes
		Ptr<ModelConstants> constants = ModelConstants::Create();

		// set the name of the attribute
		constants->SetName(name);

		// check if file exists, if so, load it, otherwise use the base object
		if (this->AttributesExist(name))
		{
			// create uri
			String path;
			path.Format("src:assets/%s.constants", name.AsCharPtr());

			// create stream
			Ptr<Stream> file = IoServer::Instance()->CreateStream(path);
			constants->Load(file);
		}

		// lastly add it to the list
		this->modelConstants.Add(name, constants);

		// return attributes
		return constants;
	}
	else if (reload)
	{
		Ptr<ModelConstants> constants = this->modelConstants[name];

		// clear attributes
		constants->Clear();

		// check if file exists, if so, load it, otherwise use the base object
		if (this->AttributesExist(name))
		{
			// create uri
			String path;
			path.Format("src:assets/%s.constants", name.AsCharPtr());

			// create stream
			Ptr<Stream> file = IoServer::Instance()->CreateStream(path);
			constants->Load(file);
		}

		return constants;
	}
	else
	{
		return this->modelConstants[name];
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelDatabase::ConstantsExist(const Util::String& name)
{
	// format file
	String file;
	file.Format("src:assets/%s.constants", name.AsCharPtr());

	// basically check if the file system has the attribute requested
	if (IoServer::Instance()->FileExists(file))
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
ModelDatabase::GetConstantsName(const Ptr<ModelConstants>& constants)
{
	IndexT index = this->modelConstants.ValuesAsArray().FindIndex(constants);
	n_assert(index != InvalidIndex);
	return this->modelConstants.KeysAsArray()[index];
}

} // namespace ToolkitUtil