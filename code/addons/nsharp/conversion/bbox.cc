//------------------------------------------------------------------------------
//  bbox.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "bbox.h"
#include "math/bbox.h"
#include "mono/jit/jit.h"
#include "mono/metadata/image.h"
#include "mono/metadata/class.h"
#include "mono/metadata/debug-helpers.h"

namespace Mono
{

static MonoClass* bboxClass;
static MonoMethod* bboxConstructor;

void BoundingBox::Setup(MonoImage * image)
{
	bboxClass = mono_class_from_name(image, MATH_NAMESPACE, "BoundingBox");
	MonoMethodDesc* desc = mono_method_desc_new(":.ctor (single,single,single,single,single,single)", false);
	bboxConstructor = mono_method_desc_search_in_class(desc, bboxClass);
	mono_method_desc_free(desc);

	if (!bboxConstructor)
		n_error("Could not find BoundingBox constructor!");
}

//------------------------------------------------------------------------------
/**
*/
MonoObject*
BoundingBox::Convert(Math::bbox const & bbox)
{
	MonoObject* retval = mono_object_new(mono_domain_get(), bboxClass);
	void* args[] = {
		(void*)&bbox.pmin.x,
		(void*)&bbox.pmin.y,
		(void*)&bbox.pmin.z,
		(void*)&bbox.pmax.x,
		(void*)&bbox.pmax.y,
		(void*)&bbox.pmax.z,
	};
	mono_runtime_invoke(bboxConstructor, retval, args, NULL);
	return retval;
}

} // namespace Mono
