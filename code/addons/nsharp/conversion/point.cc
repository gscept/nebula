//------------------------------------------------------------------------------
//  matrix44.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "matrix44.h"
#include "math/matrix44.h"
#include "mono/jit/jit.h"
#include "mono/metadata/image.h"
#include "mono/metadata/class.h"
#include "mono/metadata/debug-helpers.h"

namespace Mono
{

static MonoClass* matrixClass;
static MonoMethod* matrixConstructor;

void Matrix44::Setup(MonoImage * image)
{
	matrixClass = mono_class_from_name(image, MATH_NAMESPACE, "Matrix");
	MonoMethodDesc* desc = mono_method_desc_new(":.ctor (single,single,single,single,single,single,single,single,single,single,single,single,single,single,single,single)", false);
	matrixConstructor = mono_method_desc_search_in_class(desc, matrixClass);
	mono_method_desc_free(desc);

	if (!matrixConstructor)
		n_error("Could not find matrix constructor!");
}

//------------------------------------------------------------------------------
/**
*/
MonoObject*
Matrix44::Convert(Math::matrix44 const & matrix)
{
	MonoObject* retval = mono_object_new(mono_domain_get(), matrixClass);
	void* args[16] = {
		(void*)&matrix.getrow0()[0],
		(void*)&matrix.getrow0()[1],
		(void*)&matrix.getrow0()[2],
		(void*)&matrix.getrow0()[3],
		(void*)&matrix.getrow1()[0],
		(void*)&matrix.getrow1()[1],
		(void*)&matrix.getrow1()[2],
		(void*)&matrix.getrow1()[3],
		(void*)&matrix.getrow2()[0],
		(void*)&matrix.getrow2()[1],
		(void*)&matrix.getrow2()[2],
		(void*)&matrix.getrow2()[3],
		(void*)&matrix.getrow3()[0],
		(void*)&matrix.getrow3()[1],
		(void*)&matrix.getrow3()[2],
		(void*)&matrix.getrow3()[3]
	};
	mono_runtime_invoke(matrixConstructor, retval, args, NULL);
	return retval;
}

} // namespace Mono
