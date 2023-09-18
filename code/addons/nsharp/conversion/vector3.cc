//------------------------------------------------------------------------------
//  vector3.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "vector3.h"
#include "math/vec3.h"
#include "mono/jit/jit.h"
#include "mono/metadata/image.h"
#include "mono/metadata/class.h"
#include "mono/metadata/debug-helpers.h"

namespace Mono
{

static MonoClass* vector3Class;
static MonoMethod* vector3Constructor;

void Vector3::Setup(MonoImage * image)
{
	vector3Class = mono_class_from_name(image, MATH_NAMESPACE, "Vector3");
	MonoMethodDesc* desc = mono_method_desc_new(":.ctor (single,single,single)", false);
	vector3Constructor = mono_method_desc_search_in_class(desc, vector3Class);
	mono_method_desc_free(desc);

	if (!vector3Constructor)
		n_error("Could not find Vector3 constructor!");
}

//------------------------------------------------------------------------------
/**
*/
MonoObject*
Vector3::Convert(Math::vec3 const & vec)
{
	MonoObject* retval = mono_object_new(mono_domain_get(), vector3Class);
	void* args[] = {
		(void*)&vec.x,
		(void*)&vec.y,
		(void*)&vec.z
	};
	mono_runtime_invoke(vector3Constructor, retval, args, NULL);
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
Vector3::Convert(MonoObject* object)
{
    Math::vec3 vec;
    //void* unboxed = mono_object_unbox(object);
    void* unboxed = (void*)object;
    vec.loadu((Math::scalar*)unboxed);
    return vec;
}

} // namespace Mono
