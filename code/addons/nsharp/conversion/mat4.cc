//------------------------------------------------------------------------------
//  mat4.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "mat4.h"
#include "math/mat4.h"
#include "mono/jit/jit.h"
#include "mono/metadata/image.h"
#include "mono/metadata/class.h"
#include "mono/metadata/debug-helpers.h"

namespace Mono
{

static MonoClass* matrixClass;
static MonoMethod* matrixConstructor;

void Mat4::Setup(MonoImage * image)
{
	matrixClass = mono_class_from_name(image, MATH_NAMESPACE, "Matrix");
	MonoMethodDesc* desc = mono_method_desc_new(":.ctor (single,single,single,single,single,single,single,single,single,single,single,single,single,single,single,single)", false);
	matrixConstructor = mono_method_desc_search_in_class(desc, matrixClass);
	mono_method_desc_free(desc);

	if (!matrixConstructor)
		n_error("Could not find Matrix constructor!");
}

//------------------------------------------------------------------------------
/**
*/
MonoObject*
Mat4::Convert(Math::mat4 const& matrix)
{
	MonoObject* retval = mono_object_new(mono_domain_get(), matrixClass);
	
	void* args[] = {
		(void*)&matrix.get_x()[0],
		(void*)&matrix.get_x()[1],
		(void*)&matrix.get_x()[2],
		(void*)&matrix.get_x()[3],
		(void*)&matrix.get_y()[0],
		(void*)&matrix.get_y()[1],
		(void*)&matrix.get_y()[2],
		(void*)&matrix.get_y()[3],
		(void*)&matrix.get_z()[0],
		(void*)&matrix.get_z()[1],
		(void*)&matrix.get_z()[2],
		(void*)&matrix.get_z()[3],
		(void*)&matrix.get_w()[0],
		(void*)&matrix.get_w()[1],
		(void*)&matrix.get_w()[2],
		(void*)&matrix.get_w()[3]
	};
	
	mono_runtime_invoke(matrixConstructor, retval, args, NULL);
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
Mat4::Convert(MonoObject* object)
{
	Math::mat4 mat;
	// void* unboxed = mono_object_unbox(object);
	void* unboxed = (void*)object;
	Memory::Copy(unboxed, (void*)&mat.get_x()[0], sizeof(Math::mat4));
	return mat;
}

} // namespace Mono
