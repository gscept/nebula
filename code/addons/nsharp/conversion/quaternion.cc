//------------------------------------------------------------------------------
//  quaternion.cc
//  (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "quaternion.h"
#include "math/quat.h"
#include "mono/jit/jit.h"
#include "mono/metadata/image.h"
#include "mono/metadata/class.h"
#include "mono/metadata/debug-helpers.h"

namespace Mono
{

static MonoClass* quaternionClass;
static MonoMethod* quaternionConstructor;

void Quaternion::Setup(MonoImage * image)
{
	quaternionClass = mono_class_from_name(image, MATH_NAMESPACE, "Quaternion");
	MonoMethodDesc* desc = mono_method_desc_new(":.ctor (single,single,single,single)", false);
	quaternionConstructor = mono_method_desc_search_in_class(desc, quaternionClass);
	mono_method_desc_free(desc);

	if (!quaternionConstructor)
		n_error("Could not find Quaternion constructor!");
}

//------------------------------------------------------------------------------
/**
*/
MonoObject*
Quaternion::Convert(Math::quat const & quat)
{
	MonoObject* retval = mono_object_new(mono_domain_get(), quaternionClass);
	void* args[] = {
		(void*)&quat.x,
		(void*)&quat.y,
		(void*)&quat.z,
		(void*)&quat.w
	};
	mono_runtime_invoke(quaternionConstructor, retval, args, NULL);
	return retval;
}

} // namespace Mono
