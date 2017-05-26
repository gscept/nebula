//------------------------------------------------------------------------------
//  mouserenderdevicebase.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/mouserenderdevicebase.h"
#include "coregraphics/streamtextureloader.h"
#include "resources/resourcemanager.h"

namespace Base
{
__ImplementClass(Base::MouseRenderDeviceBase, 'MORB', Core::RefCounted);
__ImplementSingleton(Base::MouseRenderDeviceBase);

using namespace Util;
using namespace CoreGraphics;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
MouseRenderDeviceBase::MouseRenderDeviceBase() :
    isValid(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
MouseRenderDeviceBase::~MouseRenderDeviceBase()
{
    n_assert(!this->IsValid());
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
MouseRenderDeviceBase::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void
MouseRenderDeviceBase::Discard()
{
    n_assert(this->IsValid());
    
    // discard mouse pointer textures
    const Ptr<ResourceManager>& resManager = ResourceManager::Instance();
    IndexT i;
    for (i = 0; i < this->textures.Size(); i++)
    {
        const Ptr<Texture>& tex = this->textures.ValueAtIndex(i);
        if (resManager->HasResource(tex->GetResourceId()))
        {
            resManager->UnregisterUnmanagedResource(tex->GetResourceId());
        }
    }
    this->textures.Clear();
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
    This method must be called to preload texture used by the mouse renderer.
    The method may be called at any time (also several times).
*/
void
MouseRenderDeviceBase::PreloadTextures(const Array<ResourceId>& resIds)
{
    n_assert(this->IsValid());
    
    const Ptr<ResourceManager>& resManager = ResourceManager::Instance();
    IndexT i;
    for (i = 0; i < resIds.Size(); i++)
    {
        const ResourceId& resId = resIds[i];
        if (!this->textures.Contains(resId))
        {
            Ptr<Texture> tex = resManager->CreateUnmanagedResource(resId, Texture::RTTI).downcast<Texture>();
            if (tex->IsLoaded())
            {
                // texture was already loaded
                this->textures.Add(resId, tex);
            }
            else
            {
                // load texture
                Ptr<StreamTextureLoader> loader = StreamTextureLoader::Create();
                tex->SetLoader(loader.cast<ResourceLoader>());
                tex->SetAsyncEnabled(false);
                tex->Load();
                if (tex->IsLoaded())
                {
                    this->textures.Add(resId, tex);
                }
                else
                {
                    n_error("MouseRendererBase: failed to load texture '%s'!\n", resId.Value());
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Update mouse pointers for rendering in the current frame. On some
    platforms, more then one mouse pointer exists, so this method takes
    an array of MousePointer objects. Calling this method will replace
    the previous array of MousePoiners.
*/
void
MouseRenderDeviceBase::UpdatePointers(const Array<MousePointer>& pointerArray)
{
    n_assert(this->IsValid());
    this->pointers = pointerArray;
}

//------------------------------------------------------------------------------
/**
    This method should render the pointers describes by the last 
    call to UpdatePointers(). Override this method in a derived platform-
    specific class.
*/
void
MouseRenderDeviceBase::RenderPointers()
{
    n_assert(this->IsValid());

    // do nothing in parent class
}

} // namespace Base