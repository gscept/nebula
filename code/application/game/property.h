#pragma once
//------------------------------------------------------------------------------
/**
    Property

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "attr/attrid.h"

namespace Game
{

template<typename T>
using PropertyData = Game::ColumnData<T>;

class Property : public Core::RefCounted
{
    __DeclareClass(Property);
public:
    Property();
    virtual ~Property() = 0;

    virtual void SetupExternalAttributes() {};

    virtual void Init() {};

    virtual void OnActivate(Game::InstanceId instance) {};
    virtual void OnDeactivate(Game::InstanceId instance) {};
    virtual void OnBeginFrame() {};
    virtual void OnRender() {};
    virtual void OnEndFrame() {};
    virtual void OnRenderDebug() {};
    virtual void OnLoad() {};
    virtual void OnSave() {};
    virtual void OnInstanceMoved() {};

protected:
    friend class CategoryManager;

    /// which category does this property belong to
    /// @note   This is const_cast and changed by the category manager upon being attached to the category
    const Game::CategoryId category;
};


/*
struct PropertyFunctionBundle
{
    /// Called upon activation of component instance.
    /// When loading an entity from file, this is called after
    /// all components have been loaded.
    void(*OnActivate)(IndexT instance);

    /// Called upon deactivation of component instance
    void(*OnDeactivate)(IndexT instance);

    /// called at beginning of frame
    void(*OnBeginFrame)();

    /// called before rendering happens
    void(*OnRender)();

    /// called at the end of a frame
    void(*OnEndFrame)();

    /// called when game debug visualization is on
    void(*OnRenderDebug)();

    /// called when the component has been loaded from file.
    /// this does not guarantee that all components have been loaded.
    void(*OnLoad)(IndexT instance);

    /// called after an entity has been save to a file.
    void(*OnSave)(IndexT instance);

    /// Called after an instance has been moved from one index to another.
    /// Used in very special cases when you rely on for example instance id relations
    /// within your components.
    void(*OnInstanceMoved)(IndexT instance, IndexT oldIndex);
} functions;
*/
} // namespace Game
