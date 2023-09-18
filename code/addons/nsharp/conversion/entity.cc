//------------------------------------------------------------------------------
//  entity.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "entity.h"
#include "mono/jit/jit.h"
#include "mono/metadata/image.h"
#include "mono/metadata/class.h"
#include "mono/metadata/debug-helpers.h"

namespace Mono
{

static MonoClass* entityClass;
static MonoMethod* entityConstructor;

//------------------------------------------------------------------------------
/**
*/
void
Entity::Setup(MonoImage* image)
{
	entityClass = mono_class_from_name(image, GAME_NAMESPACE, "Entity");
	MonoMethodDesc* desc = mono_method_desc_new(":.ctor (uint)", false);
	entityConstructor = mono_method_desc_search_in_class(desc, entityClass);
	mono_method_desc_free(desc);

	if (!entityConstructor)
		n_error("Could not find Entity constructor!");
}

//------------------------------------------------------------------------------
/**
*/
MonoObject*
Entity::Convert(Game::Entity entity)
{
	MonoObject* retval = mono_object_new(mono_domain_get(), entityClass);
    uint32_t const hash = entity.HashCode();
	void* args[] = {
		(void*)&hash,
	};
	mono_runtime_invoke(entityConstructor, retval, args, NULL);
	return retval;
}

} // namespace Mono
