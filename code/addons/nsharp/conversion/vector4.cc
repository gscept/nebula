//------------------------------------------------------------------------------
//  vector4.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "vector4.h"
#include "math/vec4.h"
#include "mono/jit/jit.h"
#include "mono/metadata/image.h"
#include "mono/metadata/class.h"
#include "mono/metadata/debug-helpers.h"

namespace Mono
{

static MonoClass* vector4Class;
static MonoMethod* vector4Constructor;

void Vector4::Setup(MonoImage * image)
{
	vector4Class = mono_class_from_name(image, MATH_NAMESPACE, "Vector4");
	MonoMethodDesc* desc = mono_method_desc_new(":.ctor (single,single,single,single)", false);
	vector4Constructor = mono_method_desc_search_in_class(desc, vector4Class);
	mono_method_desc_free(desc);

	if (!vector4Constructor)
		n_error("Could not find Vector4 constructor!");
}

//------------------------------------------------------------------------------
/**
*/
MonoObject*
Vector4::Convert(Math::vec4 const & vec)
{
	MonoObject* retval = mono_object_new(mono_domain_get(), vector4Class);
	void* args[] = {
		(void*)&vec.x,
		(void*)&vec.y,
		(void*)&vec.z,
		(void*)&vec.w
	};
	mono_runtime_invoke(vector4Constructor, retval, args, NULL);
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4
Vector4::Convert(MonoObject* object)
{
    Math::vec4 vec;
    //void* unboxed = mono_object_unbox(object);
    void* unboxed = (void*)object;
    vec.loadu((Math::scalar*)unboxed);
    return vec;
}

} // namespace Mono
