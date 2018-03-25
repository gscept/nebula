//------------------------------------------------------------------------------
//  levelviewerfactorymanager.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "levelviewerfactorymanager.h"


namespace LevelViewer
{
	__ImplementClass(LevelViewer::LevelViewerFactoryManager, 'LVFM', BaseGameFeature::FactoryManager);
	__ImplementSingleton(LevelViewerFactoryManager);

//------------------------------------------------------------------------------
/**
*/
LevelViewerFactoryManager::LevelViewerFactoryManager()
{
	__ConstructSingleton;
	
}
 
//------------------------------------------------------------------------------
/**
*/
LevelViewerFactoryManager::~LevelViewerFactoryManager()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelViewerFactoryManager::AddProperties( const Ptr<Game::Entity>& entity, const Util::String& categoryName, bool isMaster ) const
{
    n_assert(entity);

    IndexT index = this->FindBluePrint(categoryName);
    if (InvalidIndex != index)
    {
        const BluePrint& bluePrint = this->bluePrints[index];
        int i;
        int num = bluePrint.properties.Size();
        for (i = 0; i < num; i++)
        {
            if(Core::Factory::Instance()->ClassExists(bluePrint.properties[i].propertyName))
            {
                Ptr<Game::Property> prop = this->CreateProperty(bluePrint.properties[i].propertyName);
                entity->AttachProperty(prop);
            }
            else
            {
                n_printf(("Unkown property type: " + bluePrint.properties[i].propertyName).AsCharPtr());
            }
        }
    }
}

}
