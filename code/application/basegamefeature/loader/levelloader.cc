//------------------------------------------------------------------------------
//  levelloader.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "levelloader.h"

namespace BaseGameFeature
{

//void RegisterNodes(int parent, Node node, Scene& scene, HashTable<Entity, uint>& entities)
//{
//	// Add entity to index map entry
//	entities.Add(node.entity, scene.numEntities);
//	scene.numEntities++;
//	scene.parentIndices.Append(parent)
//	for (auto child : node.children)
//		RegisterNodes(scene.numEntities - 1, child, scene);
//}


//------------------------------------------------------------------------------
/**
*/
bool
LevelLoader::Save(const Util::String& levelName)
{
	//const uint ROOT = UINT_MAX;
	//Scene scene;
	//HashTable<Entity, uint> entities;

	//// Fill numEntities and parentIndices
	//RegisterNodes(ROOT, scene.Root(), scene, entities);

	//// Fill components
	//for (auto component : manager.registeredComponents)
	//{
	//	Component c;

	//	c.fourcc = component.FourCC();
	//	c.numInstances = component.NumRegistered();
	//	c.data = component.InstanceDataBlob();

	//	// Update update each entity attribute
	//	for (Entity& entity : component.GetEntityAttributes())
	//	{
	//		// Update all entity ids to the indices theyre located at.
	//		// When loading we can then switch back to real entity ids.
	//		entity = entities[entity];
	//	}

	//	scene.numComponentTypes++;
	//	scene.components.Append(c);
	//}
	return false;
}


//------------------------------------------------------------------------------
/**
*/
bool
LevelLoader::Load(const Util::String& levelName)
{
	//auto scene = LevelManager::GetLevel(levelName);
	//Array<Entity> entities = EntityManager.CreateEntities(scene.numEntities);
	//for (auto component : scene.components)
	//{
	//	Ptr<BaseComponent> c = ComponentManager::Instance()->ComponentByFourCC(component.fourcc);
	//	if (c)
	//	{
	//		// Needs to create entirely new instances, not reuse old.
	//		uint start = c->Size();
	//		c->Alloc(component.numInstances);
	//		uint end = c->Size();
	//		c->SetData(start, end, component.data.unsafe_ptr());
	//		auto entityAttributes = c->GetEntityAttributes();
	//		for (uint i = start; i < end; i++)
	//		{
	//			// Update all entity ids to the newly generated.
	//			entityAttributes[i] = entities[entityAttributes[i]];
	//		}

	//		c->SetParents(start, end, entities, scene.parentIndices);
	//	}
	//}
	return false;
}

} // namespace BaseGameFeature
