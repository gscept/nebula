#pragma once
//------------------------------------------------------------------------------
/**
	Python conversion functions

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "pybind11/cast.h"
#include "util/variant.h"
#include "math/quaternion.h"
#include "math/transform44.h"

namespace Python
{


//------------------------------------------------------------------------------
/**
*/
Util::Variant ConvertPyType(pybind11::handle const& value, Util::Variant::Type type);

//------------------------------------------------------------------------------
/**
*/
pybind11::handle VariantToPyType(Util::Variant src, pybind11::return_value_policy policy = pybind11::return_value_policy::automatic);

} // namespace Python
