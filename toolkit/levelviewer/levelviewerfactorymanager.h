#pragma once
//------------------------------------------------------------------------------
/**
    @class LevelViewer::LevelViewerFactoryManager

    Overloaded factory manager that will override addproperties in order
    to ignore unknown properties

    (C) 2015-2016 Individual contributors, see AUTHORS file
*/

#include "managers/factorymanager.h"

//------------------------------------------------------------------------------
namespace LevelViewer
{
class LevelViewerFactoryManager : public BaseGameFeature::FactoryManager
{
    __DeclareClass(LevelViewerFactoryManager);
    __DeclareSingleton(LevelViewerFactoryManager);
public:
    /// constructor
    LevelViewerFactoryManager();
    /// destructor
    virtual ~LevelViewerFactoryManager();
    /// add properties to entity according to blue print but ignore unknowns
    virtual void AddProperties(const Ptr<Game::Entity>& entity, const Util::String& categoryName, bool isMaster) const;
};
}