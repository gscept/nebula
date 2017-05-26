//------------------------------------------------------------------------------
//  surface.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "surface.h"
#include "coregraphics/shader.h"
#include "resources/resourcemanager.h"
#include "surfaceinstance.h"
#include "surfaceconstant.h"

using namespace CoreGraphics;
namespace Materials
{
__ImplementClass(Materials::Surface, 'SRMA', Resources::Resource);

//------------------------------------------------------------------------------
/**
*/
Surface::Surface()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Surface::~Surface()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
Surface::Unload()
{
    n_assert(this->instances.IsEmpty());
    this->staticValues.Clear();

    // only destroy material if its loaded
    if (this->IsLoaded())
    {
        // remove from material
        this->materialTemplate->RemoveSurface(this);
        this->materialTemplate = 0;
    }

	// run base class unload
	Resource::Unload();
}

//------------------------------------------------------------------------------
/**
*/
void
Surface::Reload()
{
    // clear static values
    this->staticValues.Clear();

    // only destroy material if its loaded
    if (this->IsLoaded())
    {
        // remove from material
        this->materialTemplate->RemoveSurface(this);
        this->materialTemplate = 0;
    }

    // perform actual load
	bool async = this->asyncEnabled;
	this->SetAsyncEnabled(false);
	Resource::Unload();
    this->loader->Reset();
    this->Load();
	this->SetAsyncEnabled(async);

    // go through instances and reset their values, which will effectively discard any per instance value
    IndexT i;
    for (i = 0; i < this->instances.Size(); i++)
    {
		// not a const ref, because this will probably be the LAST reference!
        const Ptr<SurfaceInstance>& inst = this->instances[i];

		// reset the surface instance
		inst->Cleanup();
		inst->Setup(this);
    }
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SurfaceInstance>
Surface::CreateInstance()
{
    Ptr<SurfaceInstance> newInst = SurfaceInstance::Create();
    Ptr<Surface> thisPtr(this);
    newInst->Setup(thisPtr.downcast<Surface>());
    this->instances.Append(newInst);
    return newInst;
}

//------------------------------------------~------------------------------------
/**
*/
void
Surface::DiscardInstance(const Ptr<SurfaceInstance>& instance)
{
    instance->Cleanup();
    IndexT i = this->instances.FindIndex(instance);
    n_assert(InvalidIndex != i);
    this->instances.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
void
Surface::Setup(const Ptr<Material>& material)
{
    n_assert(material.isvalid());
    this->materialTemplate = material;

    // get parameters from material
    const Util::Dictionary<Util::StringAtom, Material::MaterialParameter>& parameters = this->materialTemplate->GetParameters();
    this->code = SurfaceName::FromName(this->resourceId);

    IndexT paramIndex;
    for (paramIndex = 0; paramIndex < parameters.Size(); paramIndex++)
    {
        // get parameter name
        const Util::StringAtom& paramName = parameters.KeyAtIndex(paramIndex);
        const Material::MaterialParameter& param = parameters.ValueAtIndex(paramIndex);

        // value can either by material template default, or the value defined in the surface
        Util::Variant val;
        if (this->staticValues.Contains(paramName)) val = this->staticValues[paramName].value;
        else
        {
            SurfaceValueBinding obj;
            obj.value = param.defaultVal;
            obj.system = param.system;
            this->staticValues.Add(paramName, obj);
        }
    }

    // add to material template
    this->materialTemplate->AddSurface(this);
}

} // namespace Materials