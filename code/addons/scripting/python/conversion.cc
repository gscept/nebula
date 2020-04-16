//------------------------------------------------------------------------------
//  conversion.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "conversion.h"
#include "pybind11/operators.h"
#include "pybind11/cast.h"
#include "util/random.h"

namespace Python
{

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(nmath, m)
{
    py::class_<Math::float4>(m, "Float4", py::buffer_protocol())
        .def(py::init([](){return Math::float4();}))
		.def(py::init<float, float, float, float>())
		.def(py::init([](py::array_t<float> b)
    	{
    	    py::buffer_info info = b.request();
    	    if (info.size != 4)
				throw pybind11::value_error("Invalid number of elements!");
			Math::float4 f;
    	    f.loadu((float*)info.ptr);
    	    return f;
    	}))
		.def("__getitem__", [](Math::float4&f, ssize_t i)
    	{
    	    if (i > 3 || i < 0)
				throw pybind11::index_error("Index out of range!");
    	    return f[i];
    	})
    	.def("__setitem__", [](Math::float4&f, ssize_t i, float v)
    	{
    	    if (i > 3 || i < 0)
				throw pybind11::index_error("Index out of range!");
    	    f[i] = v;
    	})
		.def("__repr__",
            [](Math::float4 const& val)
            {
                return Util::String::FromFloat4(val);
            }
        )
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def(py::self * float())
		.def(py::self - py::self)
		.def(py::self + py::self)
		.def(py::self += py::self)
		.def(py::self -= py::self)
		.def(py::self *= py::self)
		.def(py::self /= py::self)
		.def(-py::self)
		.def_property("x", (Math::scalar (Math::float4::*)() const)&Math::float4::x, (Math::scalar&(Math::float4::*)())&Math::float4::x)
		.def_property("y", (Math::scalar(Math::float4::*)() const)&Math::float4::y, (Math::scalar&(Math::float4::*)())&Math::float4::y)
		.def_property("z", (Math::scalar (Math::float4::*)() const)&Math::float4::z, (Math::scalar&(Math::float4::*)())&Math::float4::z)
		.def_property("w", (Math::scalar (Math::float4::*)() const)&Math::float4::w, (Math::scalar&(Math::float4::*)())&Math::float4::w)
		.def("length", &Math::float4::length)
		.def("length3", &Math::float4::length3)
		.def("length_sq", &Math::float4::lengthsq)
		.def("length3_sq", &Math::float4::lengthsq3)
		.def("abs", &Math::float4::abs)
		.def_static("reciprocal", &Math::float4::reciprocal)
		.def_static("multiply", &Math::float4::multiply)
		.def_static("multiply_add", &Math::float4::multiplyadd)
		.def_static("divide", &Math::float4::divide)
		.def_static("cross3", &Math::float4::cross3)
        .def_static("dot3", &Math::float4::dot3)
		.def_static("barycentric", &Math::float4::barycentric)
		.def_static("catmullrom", &Math::float4::catmullrom)
		.def_static("hermite", &Math::float4::hermite)
		.def_static("lerp", &Math::float4::lerp)
		.def_static("maximize", &Math::float4::maximize)
		.def_static("minimize", &Math::float4::minimize)
		.def_static("normalize", &Math::float4::normalize)
		.def_static("reflect", &Math::float4::reflect)
		.def_static("clamp", &Math::float4::clamp)
		.def_static("angle", &Math::float4::angle)
		.def_static("zerovector", &Math::float4::zerovector)
		.def_static("perspective_div", &Math::float4::perspective_div)
		.def_static("sum", &Math::float4::sum)
		.def_static("floor", &Math::float4::floor)
		.def_static("ceil", &Math::float4::ceiling)
		.def_buffer([](Math::float4 &m) -> py::buffer_info {
			return py::buffer_info(
				&m[0],				/* Pointer to buffer */
				{ 1, 4 },           /* Buffer dimensions */
				{ 4 * sizeof(float),/* Strides (in bytes) for each index */
				sizeof(float) }
			);
		});
	py::class_<Math::point, Math::float4>(m, "Point", py::buffer_protocol())
        .def(py::init<float, float, float>())
		.def(py::init([](py::array_t<float> b)
    	{
    	    py::buffer_info info = b.request();
    	    if (info.size != 4)
				throw pybind11::value_error("Invalid number of elements!");
			Math::float4 f;
    	    f.loadu((float*)info.ptr);
    	    return Math::point(f);
    	}));
	
	py::class_<Math::vector, Math::float4>(m, "Vector", py::buffer_protocol())
        .def(py::init<float, float, float>())
		.def(py::init([](py::array_t<float> b)
    	{
    	    py::buffer_info info = b.request();
    	    if (info.size != 4)
				throw pybind11::value_error("Invalid number of elements!");
			Math::float4 f;
    	    f.loadu((float*)info.ptr);
    	    return Math::vector(f);
    	}));

	py::class_<Math::matrix44>(m, "Matrix44", py::buffer_protocol())
        .def(py::init([](){return Math::matrix44();}))
		.def(py::init<Math::float4 const&, Math::float4 const&, Math::float4 const&, Math::float4 const&>())
		.def(py::init([](py::array_t<float> b)
    	{
    	    py::buffer_info info = b.request();
    	    if (info.size != 16)
				throw pybind11::value_error("Invalid number of elements!");
			Math::matrix44 m;
    	    m.loadu((float*)info.ptr);
    	    return m;
    	}))
		.def(py::init([](float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float m, float n, float o, float p)
    	{
    	    Math::matrix44 mat({a,b,c,d},{e,f,g,h},{i,j,k,l},{m,n,o,p});
    	    return mat;
    	}))
		.def("__getitem__", [](Math::matrix44& m, ssize_t i)
    	{
    	    switch (i)
			{
			case 0:
				return m.getrow0();
			case 1:
				return m.getrow1();
			case 2:
				return m.getrow2();
			case 3:
				return m.getrow3();
			default:
				throw pybind11::index_error("Index out of range!");
			}
    	})
    	.def("__setitem__", [](Math::matrix44& m, ssize_t i, Math::float4 const& v)
    	{
    	    switch (i)
			{
			case 0:
				m.setrow0(v);
				return;
			case 1:
				m.setrow1(v);
				return;
			case 2:
				m.setrow2(v);
				return;
			case 3:
				m.setrow3(v);
				return;
			default:
				throw pybind11::index_error("Index out of range!");
			}
    	})
		.def("__repr__",
            [](Math::matrix44 const& val)
            {
                return Util::String::FromMatrix44(val);
            }
        )
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def_property("row0", &Math::matrix44::getrow0, &Math::matrix44::setrow0)
		.def_property("row1", &Math::matrix44::getrow1, &Math::matrix44::setrow1)
		.def_property("row2", &Math::matrix44::getrow2, &Math::matrix44::setrow2)
		.def_property("row3", &Math::matrix44::getrow3, &Math::matrix44::setrow3)
		.def_property("right", 		&Math::matrix44::get_xaxis, 	&Math::matrix44::set_xaxis)
		.def_property("up", 		&Math::matrix44::get_yaxis, 	&Math::matrix44::set_yaxis)
		.def_property("forward", 	&Math::matrix44::get_zaxis, 	&Math::matrix44::set_zaxis)
		.def_property("position", 	&Math::matrix44::get_position, 	&Math::matrix44::set_position)
		.def("is_identity", &Math::matrix44::isidentity)
		.def("determinant", &Math::matrix44::determinant)
		.def("get_determinant", &Math::matrix44::determinant)
		.def_static("identity", &Math::matrix44::identity)
		.def_static("affine_transformation", &Math::matrix44::affinetransformation, "Build matrix from affine transformation")
		.def_static("inverse", &Math::matrix44::inverse)
		.def_static("look_at_rh", &Math::matrix44::lookatrh)
		.def_static("look_at_lh", &Math::matrix44::lookatlh)
		.def_static("multiply", &Math::matrix44::multiply)
		.def_static("ortho_lh", &Math::matrix44::ortholh)
		.def_static("ortho_rh", &Math::matrix44::orthorh)
		.def_static("ortho_off_center_lh", &Math::matrix44::orthooffcenterlh)
		.def_static("ortho_off_center_rh", &Math::matrix44::orthooffcenterrh)
		.def_static("persp_fov_lh", &Math::matrix44::perspfovlh)
		.def_static("persp_fov_rh", &Math::matrix44::perspfovrh)
		.def_static("persp_lh", &Math::matrix44::persplh)
		.def_static("persp_rh", &Math::matrix44::persprh)
		.def_static("persp_off_center_lh", &Math::matrix44::perspoffcenterlh)
		.def_static("persp_off_center_rh", &Math::matrix44::perspoffcenterrh)
		.def_static("reflect", &Math::matrix44::reflect)
		.def_static("rotation_axis", &Math::matrix44::rotationaxis)
		.def_static("rotation_quaternion", &Math::matrix44::rotationquaternion)
		.def_static("rotation_x", &Math::matrix44::rotationx)
		.def_static("rotation_y", &Math::matrix44::rotationy)
		.def_static("rotation_z", &Math::matrix44::rotationz)
		.def_static("rotation_yaw_pitch_roll", &Math::matrix44::rotationyawpitchroll)
		.def_static("scaling", py::overload_cast<Math::scalar,Math::scalar,Math::scalar>(&Math::matrix44::scaling))
		.def_static("scaling", py::overload_cast<Math::float4 const&>(&Math::matrix44::scaling))
		.def_static("transformation", &Math::matrix44::transformation)
		.def_static("translation", py::overload_cast<Math::scalar,Math::scalar,Math::scalar>(&Math::matrix44::translation))
		.def_static("translation", py::overload_cast<Math::float4 const&>(&Math::matrix44::translation))
		.def_static("transpose", &Math::matrix44::transpose)
		.def_static("transform", py::overload_cast<const Math::float4&, const Math::matrix44&>(&Math::matrix44::transform))
		.def_static("transform3", py::overload_cast<const Math::float4&, const Math::matrix44&>(&Math::matrix44::transform3))
		.def_static("transform3", py::overload_cast<const Math::point&, const Math::matrix44&>(&Math::matrix44::transform3))
		.def_static("transform3", py::overload_cast<const Math::vector&, const Math::matrix44&>(&Math::matrix44::transform3))
		.def_static("rotation_matrix", &Math::matrix44::rotationmatrix)
		.def_static("is_point_inside", &Math::matrix44::ispointinside)
		.def_buffer([](Math::matrix44 &m) -> py::buffer_info {
			return py::buffer_info(
				(float*)&m.row0(),			/* Pointer to buffer */
				{ 4, 4 },           /* Buffer dimensions */
				{ 4 * sizeof(float),/* Strides (in bytes) for each index */
				sizeof(float) }
			);
		});

	py::class_<Math::quaternion>(m, "Quaternion", py::buffer_protocol())
        .def(py::init([](){return Math::quaternion();}))
		.def(py::init<float, float, float, float>())
		.def(py::init([](py::array_t<float> b)
    	{
    	    py::buffer_info info = b.request();
    	    if (info.size != 4)
				throw pybind11::value_error("Invalid number of elements!");
			Math::quaternion f;
    	    f.loadu((float*)info.ptr);
    	    return f;
    	}))
		.def("__repr__",
            [](Math::quaternion const& val)
            {
                return Util::String::FromQuaternion(val);
            }
        )
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def_property("x", (Math::scalar (Math::quaternion::*)() const)&Math::quaternion::x, (Math::scalar&(Math::quaternion::*)())&Math::quaternion::x)
		.def_property("y", (Math::scalar(Math::quaternion::*)() const)&Math::quaternion::y, (Math::scalar&(Math::quaternion::*)())&Math::quaternion::y)
		.def_property("z", (Math::scalar (Math::quaternion::*)() const)&Math::quaternion::z, (Math::scalar&(Math::quaternion::*)())&Math::quaternion::z)
		.def_property("w", (Math::scalar (Math::quaternion::*)() const)&Math::quaternion::w, (Math::scalar&(Math::quaternion::*)())&Math::quaternion::w)
		.def("is_identity", &Math::quaternion::isidentity)
		.def("length", &Math::quaternion::length)
		.def("length_sq", &Math::quaternion::lengthsq)
		.def("undenormalize", &Math::quaternion::undenormalize)
		.def_static("barycentric", &Math::quaternion::barycentric)
		.def_static("conjugate", &Math::quaternion::conjugate)
		.def_static("dot", &Math::quaternion::dot)
		.def_static("exp", &Math::quaternion::exp)
		.def_static("identity", &Math::quaternion::identity)
		.def_static("inverse", &Math::quaternion::inverse)
		.def_static("ln", &Math::quaternion::ln)
		.def_static("multiply", &Math::quaternion::multiply)
		.def_static("normalize", &Math::quaternion::normalize)
		.def_static("rotation_axis", &Math::quaternion::rotationaxis)
		.def_static("rotation_matrix", &Math::quaternion::rotationmatrix)
		.def_static("rotation_yaw_pitch_roll", &Math::quaternion::rotationyawpitchroll)
		.def_static("slerp", &Math::quaternion::slerp)
		.def_static("squad_setup", &Math::quaternion::squadsetup)
		.def_static("squad", &Math::quaternion::squad)
		.def_static("to_axis_angle", &Math::quaternion::to_axisangle)		
		.def_buffer([](Math::quaternion &m) -> py::buffer_info {
			return py::buffer_info(
				&m.x(),				/* Pointer to buffer */
				{ 1, 4 },           /* Buffer dimensions */
				{ 4 * sizeof(float),/* Strides (in bytes) for each index */
				sizeof(float) }
			);
		});	
		
	py::class_<Math::float2>(m, "Float2", py::buffer_protocol())
        .def(py::init([](){return Math::float2();}))
		.def(py::init<float>())
		.def(py::init<float, float>())
		.def(py::init([](py::array_t<float> b)
    	{
    	    py::buffer_info info = b.request();
    	    if (info.size != 2)
				throw pybind11::value_error("Invalid number of elements!");
			Math::float2 f(b.at(0), b.at(1));
    	    return f;
    	}))
		.def("__repr__",
            [](Math::float2 const& val)
            {
                return Util::String::FromFloat2(val);
            }
        )
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def(py::self += py::self)
		.def(py::self -= py::self)
		.def(py::self *= float())
		.def(py::self + py::self)
		.def(py::self - py::self)
		.def(py::self * float())
		.def(-py::self)
		.def_property("x", (Math::scalar (Math::float2::*)() const)&Math::float2::x, (Math::scalar&(Math::float2::*)())&Math::float2::x)
		.def_property("y", (Math::scalar(Math::float2::*)() const)&Math::float2::y, (Math::scalar&(Math::float2::*)())&Math::float2::y)
		.def("length", &Math::float2::length)
		.def("length_sq", &Math::float2::lengthsq)
		.def("abs", &Math::float2::abs)
		.def("any", &Math::float2::any)
		.def("all", &Math::float2::all)
		.def_static("multiply", &Math::float2::multiply)
		.def_static("maximize", &Math::float2::maximize)
		.def_static("minimize", &Math::float2::minimize)
		.def_static("normalize", &Math::float2::normalize)
		.def_static("lt", &Math::float2::lt)
		.def_static("le", &Math::float2::le)
		.def_static("gt", &Math::float2::gt)
		.def_static("ge", &Math::float2::ge)
		.def_buffer([](Math::float2& f) -> py::buffer_info {
			return py::buffer_info(
				(float*)&f,				/* Pointer to buffer */
				{ 1, 4 },           /* Buffer dimensions */
				{ 4 * sizeof(float),/* Strides (in bytes) for each index */
				sizeof(float) }
			);
		});	
}

PYBIND11_EMBEDDED_MODULE(util, m)
{
	m.def("randf",
        []()->float
        {
            return Util::RandomFloat();
        },
		"Returns a random floating point number between 0 .. 1 using XorShift128."
    );

	m.def("rand",
        []()->uint
        {
            return Util::FastRandom();
        },
		"Returns an XorShift128 random 32-bit unsigned integer."
    );

    py::class_<Util::Guid>(m, "Guid")
        .def(py::init(
            []()
            {
                return Util::Guid();
            }
		))
		.def(py::init(
            [](const char* str)
            {
                return Util::Guid::FromString(str);
            }
		))
		.def("__repr__",
            [](Util::Guid const& guid)
            {
                return guid.AsString();
            }
        )
		.def("generate", &Util::Guid::Generate)
        .def("to_string", &Util::Guid::AsString);
	
	py::class_<Util::FourCC>(m, "FourCC")
        .def(py::init([](){ return Util::FourCC(); }))
		.def(py::init(
            [](const char* str)
            {
                return Util::FourCC::FromString(str);
            }
		))
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def(py::self <= py::self)
		.def(py::self >= py::self)
		.def(py::self < py::self)
		.def(py::self > py::self)
		.def("__repr__",
            [](Util::FourCC fourcc)
            {
                return fourcc.AsString();
            }
        )
		.def_static("lookup",[](Util::String const& s)->Util::FourCC
		{
			if(Core::Factory::Instance()->ClassExists(s))
			{
				return Core::Factory::Instance()->GetClassRtti(s)->GetFourCC();
			}
			else throw(std::exception("Unknown class!"));
			return Util::FourCC();
		});
}

//------------------------------------------------------------------------------
/**
*/
pybind11::handle VariantToPyType(Util::Variant src, pybind11::return_value_policy policy, pybind11::handle parent)
{
	using namespace pybind11;
	namespace py = pybind11;

	auto type = src.GetType();
    if (type == Util::Variant::Type::Void)
    {
        return Py_None;
    }
	else if (type == Util::Variant::Type::Byte)
	{
		return PyLong_FromLong(src.GetByte());
	}
	else if (type == Util::Variant::Type::Short)
	{
		return PyLong_FromLong(src.GetShort());
	}
	else if (type == Util::Variant::Type::UShort)
	{
		return PyLong_FromLong(src.GetUShort());
	}
	else if (type == Util::Variant::Type::Int)
	{
		return PyLong_FromLong(src.GetInt());
	}
	else if (type == Util::Variant::Type::UInt)
	{
		return PyLong_FromLong(src.GetUInt());
	}
	else if (type == Util::Variant::Type::Int64)
	{
		return PyLong_FromLong(src.GetInt64());
	}
	else if (type == Util::Variant::Type::UInt64)
	{
		return PyLong_FromLong(src.GetUInt64());
	}
	else if (type == Util::Variant::Type::Float)
	{
		return PyFloat_FromDouble(src.GetFloat());
	}
	else if (type == Util::Variant::Type::Double)
	{
		return PyFloat_FromDouble(src.GetDouble());
	}
	else if (type == Util::Variant::Type::Bool)
	{
		return PyBool_FromLong(src.GetBool());
	}
	else if (type == Util::Variant::Type::Float2)
	{
		return py::cast(src.GetFloat2(), policy);
	}
	else if (type == Util::Variant::Type::Float4)
	{
		return py::detail::make_caster<Math::float4>::cast(src.GetFloat4(), policy, parent);
	}
	else if (type == Util::Variant::Type::Quaternion)
	{
		return py::detail::make_caster<Math::quaternion>::cast(src.GetQuaternion(), policy, parent);
	}
	else if (type == Util::Variant::Type::String)
	{
		return py::detail::make_caster<Util::String>::cast(src.GetString(), policy, parent);
	}
	else if (type == Util::Variant::Type::Matrix44)
	{
		return py::detail::make_caster<Math::matrix44>::cast(src.GetMatrix44(), policy, parent);
	}
	else if (type == Util::Variant::Type::Guid)
	{
		return py::detail::make_caster<Util::Guid>::cast(src.GetGuid(), policy, parent);
	}
	else if (type == Util::Variant::Type::IntArray)
	{
		return py::detail::make_caster<Util::Array<int>>::cast(src.GetIntArray(), policy, parent);
	}
	else if (type == Util::Variant::Type::FloatArray)
	{
		return py::detail::make_caster<Util::Array<float>>::cast(src.GetFloatArray(), policy, parent);
	}
	else if (type == Util::Variant::Type::BoolArray)
	{
		return py::detail::make_caster<Util::Array<bool>>::cast(src.GetBoolArray(), policy, parent);
	}
	else if (type == Util::Variant::Type::Float2Array)
	{
		return py::detail::make_caster<Util::Array<Math::float2>>::cast(src.GetFloat2Array(), policy, parent);
	}
	else if (type == Util::Variant::Type::Float4Array)
	{
		return py::detail::make_caster<Util::Array<Math::float4>>::cast(src.GetFloat4Array(), policy, parent);
	}
	else if (type == Util::Variant::Type::StringArray)
	{
		return py::detail::make_caster<Util::Array<Util::String>>::cast(src.GetStringArray(), policy, parent);
	}
	else if (type == Util::Variant::Type::Matrix44Array)
	{
		return py::detail::make_caster<Util::Array<Math::matrix44>>::cast(src.GetMatrix44Array(), policy, parent);
	}
	else if (type == Util::Variant::Type::GuidArray)
	{
		return py::detail::make_caster<Util::Array<Util::Guid>>::cast(src.GetGuidArray(), policy, parent);
	}
	// TODO: the rest of these...

	// TODO: Error handling
	throw std::exception("Unimplemented variant type!\n");
	return PyBool_FromLong(0);
}
} // namespace Python
