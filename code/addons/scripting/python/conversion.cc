//------------------------------------------------------------------------------
//  conversion.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
//#pragma warning (disable : 4267)
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
    py::class_<Math::vec4>(m, "Vec4", py::buffer_protocol())
        .def(py::init([](){return Math::vec4();}))
        .def(py::init<float, float, float, float>())
        .def(py::init([](py::array_t<float> b)
        {
            py::buffer_info info = b.request();
            if (info.size != 4)
                throw pybind11::value_error("Invalid number of elements!");
            Math::vec4 f;
            f.loadu((float*)info.ptr);
            return f;
        }))
        .def("__getitem__", [](Math::vec4&f, const int i)
        {
            if (i > 3 || i < 0)
                throw pybind11::index_error("Index out of range!");
            return f[i];
        })
        .def("__setitem__", [](Math::vec4&f, const int i, float v)
        {
            if (i > 3 || i < 0)
                throw pybind11::index_error("Index out of range!");
            f[i] = v;
        })
        .def("__repr__",
            [](Math::vec4 const& val)
            {
                return Util::String::FromVec4(val);
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
        .def_readwrite("x", &Math::vec4::x)
        .def_readwrite("y", &Math::vec4::y)
        .def_readwrite("z", &Math::vec4::z)
        .def_readwrite("w", &Math::vec4::w)
        .def("length", py::overload_cast<Math::vec4 const&>(&Math::length))
        .def("length3", &Math::length3)
        .def("length_sq", py::overload_cast<Math::vec4 const&>(&Math::lengthsq))
        .def("length3_sq", &Math::lengthsq3)
        .def("abs", py::overload_cast<Math::vec4 const&>(&Math::abs))
        .def_static("reciprocal", py::overload_cast<Math::vec4 const&>(&Math::reciprocal))
        .def_static("multiply_add", py::overload_cast<Math::vec4 const&, Math::vec4 const&, Math::vec4 const&>(&Math::multiplyadd))
        .def_static("divide", py::overload_cast<Math::vec4 const&, Math::vec4 const&>(&Math::divide))
        .def_static("cross3", &Math::cross3)
        .def_static("dot3", &Math::dot3)
        .def_static("barycentric", py::overload_cast<Math::vec4 const&, Math::vec4 const&, Math::vec4 const&, Math::scalar, Math::scalar>(&Math::barycentric))
        .def_static("catmullrom", py::overload_cast<Math::vec4 const&, Math::vec4 const&, Math::vec4 const&, Math::vec4 const&, Math::scalar>(&Math::catmullrom))
        .def_static("hermite", py::overload_cast<Math::vec4 const&, Math::vec4 const&, Math::vec4 const&, Math::vec4 const&, Math::scalar>(&Math::hermite))
        .def_static("lerp", py::overload_cast<Math::vec4 const&, Math::vec4 const&, Math::scalar>(&Math::lerp))
        .def_static("maximize", py::overload_cast<Math::vec4 const&, Math::vec4 const&>(&Math::maximize))
        .def_static("minimize", py::overload_cast<Math::vec4 const&, Math::vec4 const&>(&Math::minimize))
        .def_static("normalize", py::overload_cast<Math::vec4 const&>(&Math::normalize))
        .def_static("reflect", py::overload_cast<Math::vec4 const&, Math::vec4 const&>(&Math::reflect))
        .def_static("clamp", py::overload_cast<Math::vec4 const&, Math::vec4 const&, Math::vec4 const&>(&Math::clamp))
        .def_static("angle", py::overload_cast<Math::vec4 const&, Math::vec4 const&>(&Math::angle))
        .def_static("perspective_div", &Math::perspective_div)
        .def_static("floor", py::overload_cast<Math::vec4 const&>(&Math::floor))
        .def_static("ceil", py::overload_cast<Math::vec4 const&>(&Math::ceil))
        .def_buffer([](Math::vec4 &m) -> py::buffer_info {
            return py::buffer_info(
                &m[0],              /* Pointer to buffer */
                { 1, 4 },           /* Buffer dimensions */
                { 4 * sizeof(float),/* Strides (in bytes) for each index */
                sizeof(float) }
            );
        });

    py::class_<Math::point>(m, "Point", py::buffer_protocol())
        .def(py::init<float, float, float>())
        .def(py::init([](py::array_t<float> b)
        {
            py::buffer_info info = b.request();
            if (info.size != 3)
                throw pybind11::value_error("Invalid number of elements!");
            Math::vec4 f;
            f.loadu((float*)info.ptr);
            return Math::point(f);
        }))
        .def_readwrite("x", &Math::point::x)
        .def_readwrite("y", &Math::point::y)
        .def_readwrite("z", &Math::point::z)
        .def(py::self == py::self)
        .def(py::self + Math::vector())
        .def(py::self - Math::vector())
        .def("__repr__",
            [](Math::point const& p)
            {
                return Util::String::FromVec3(p.vec);
            }
        );
    
    py::class_<Math::vector>(m, "Vector", py::buffer_protocol())
        .def(py::init<float, float, float>())
        .def(py::init([](py::array_t<float> b)
        {
            py::buffer_info info = b.request();
            if (info.size != 3)
                throw pybind11::value_error("Invalid number of elements!");
            Math::vec3 f;
            f.loadu((float*)info.ptr);
            return Math::vector(f);
        }))
        .def_readwrite("x", &Math::vector::x)
        .def_readwrite("y", &Math::vector::y)
        .def_readwrite("z", &Math::vector::z)
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self * float())
        .def("__repr__",
            [](Math::vector const& v)
            {
                return Util::String::FromVec3(v.vec);
            }
        );

    py::class_<Math::mat4>(m, "Mat4", py::buffer_protocol())
        .def(py::init([](){return Math::mat4();}))
        .def(py::init<Math::vec4 const&, Math::vec4 const&, Math::vec4 const&, Math::vec4 const&>())
        .def(py::init([](py::array_t<float> b)
        {
            py::buffer_info info = b.request();
            if (info.size != 16)
                throw pybind11::value_error("Invalid number of elements!");
            Math::mat4 m;
            m.loadu((float*)info.ptr);
            return m;
        }))
        .def(py::init([](float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float m, float n, float o, float p)
        {
            Math::mat4 mat({a,b,c,d},{e,f,g,h},{i,j,k,l},{m,n,o,p});
            return mat;
        }))
        .def("__getitem__", [](Math::mat4& m, size_t i)
        {
            switch (i)
            {
            case 0:
                return m.row0;
            case 1:
                return m.row1;
            case 2:
                return m.row2;
            case 3:
                return m.row3;
            default:
                throw pybind11::index_error("Index out of range!");
            }
        })
        .def("__setitem__", [](Math::mat4& m, size_t i, Math::vec4 const& v)
        {
            switch (i)
            {
            case 0:
                m.row0 = v;
                return;
            case 1:
                m.row1 = v;
                return;
            case 2:
                m.row2 = v;
                return;
            case 3:
                m.row3 = v;
                return;
            default:
                throw pybind11::index_error("Index out of range!");
            }
        })
        .def("__repr__",
            [](Math::mat4 const& val)
            {
                return Util::String::FromMat4(val);
            }
        )
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self * py::self)
        .def(py::self * Math::vec4())
        .def(py::self * Math::point())
        .def("__mul__", 
                [](const Math::mat4& m, const Math::vector& v)
                {
                return Math::vector((m*Math::vec4(v.vec)).vec);
                } , py::is_operator())
        .def("determinant", &Math::determinant)
        .def("get_determinant", &Math::determinant)
        .def_static("identity", &Math::identity)
        .def_static("affine_transformation", &Math::affinetransformation, "Build matrix from affine transformation")
        .def_static("inverse", py::overload_cast<Math::mat4 const&>(&Math::inverse))
        .def_static("look_at_rh", &Math::lookatrh)
        .def_static("look_at_lh", &Math::lookatlh)
        .def_static("ortho_lh", &Math::ortholh)
        .def_static("ortho_rh", &Math::orthorh)
        .def_static("ortho_off_center_lh", &Math::orthooffcenterlh)
        .def_static("ortho_off_center_rh", &Math::orthooffcenterrh)
        .def_static("persp_fov_lh", &Math::perspfovlh)
        .def_static("persp_fov_rh", &Math::perspfovrh)
        .def_static("persp_lh", &Math::persplh)
        .def_static("persp_rh", &Math::persprh)
        .def_static("persp_off_center_lh", &Math::perspoffcenterlh)
        .def_static("persp_off_center_rh", &Math::perspoffcenterrh)
        .def_static("reflect", py::overload_cast<Math::vec4 const&>(&Math::reflect))
        .def_static("rotation_axis", py::overload_cast<Math::vec3 const&, Math::scalar>(&Math::rotationaxis))
        .def_static("rotation_quaternion", &Math::rotationquat)
        .def_static("rotation_x", &Math::rotationx)
        .def_static("rotation_y", &Math::rotationy)
        .def_static("rotation_z", &Math::rotationz)
        .def_static("rotation_yaw_pitch_roll", &Math::rotationyawpitchroll)
        .def_static("scaling", py::overload_cast<Math::scalar,Math::scalar,Math::scalar>(&Math::scaling))
        .def_static("scaling", py::overload_cast<Math::vec3 const&>(&Math::scaling))
        .def_static("transformation", &Math::transformation)
        .def_static("translation", py::overload_cast<Math::scalar,Math::scalar,Math::scalar>(&Math::translation))
        .def_static("translation", py::overload_cast<Math::vec3 const&>(&Math::translation))
        .def_static("transpose", &Math::transpose)
        .def_static("rotation_matrix", &Math::rotationmatrix)
        .def_static("is_point_inside", &Math::ispointinside)
        .def_buffer([](Math::mat4 &m) -> py::buffer_info {
            return py::buffer_info(
                (float*)&m.row0,            /* Pointer to buffer */
                { 4, 4 },           /* Buffer dimensions */
                { 4 * sizeof(float),/* Strides (in bytes) for each index */
                sizeof(float) }
            );
        });

    py::class_<Math::quat>(m, "Quaternion", py::buffer_protocol())
        .def(py::init([](){return Math::quat();}))
        .def(py::init<float, float, float, float>())
        .def(py::init([](py::array_t<float> b)
        {
            py::buffer_info info = b.request();
            if (info.size != 4)
                throw pybind11::value_error("Invalid number of elements!");
            Math::quat f;
            f.loadu((float*)info.ptr);
            return f;
        }))
        .def("__repr__",
            [](Math::quat const& val)
            {
                return Util::String::FromQuat(val);
            }
        )
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def_readwrite("x", &Math::quat::x)
        .def_readwrite("y", &Math::quat::y)
        .def_readwrite("z", &Math::quat::z)
        .def_readwrite("w", &Math::quat::w)
        .def("length", py::overload_cast<Math::quat const&>(&Math::length))
        .def("length_sq", py::overload_cast<Math::quat const&>(&Math::lengthsq))
        .def("undenormalize", &Math::quatUndenormalize)
        .def_static("barycentric", py::overload_cast<Math::quat const&, Math::quat const&, Math::quat const&, Math::scalar, Math::scalar>(&Math::barycentric))
        .def_static("conjugate", &Math::conjugate)
        .def_static("dot", py::overload_cast<Math::quat const&, Math::quat const&>(&Math::dot))
        .def_static("exp", &Math::quatExp)
        .def_static("identity", &Math::identity)
        .def_static("inverse", py::overload_cast<Math::quat const&>(&Math::inverse))
        .def_static("ln", &Math::ln)
        .def_static("normalize", py::overload_cast<Math::quat const&>(&Math::normalize))
        .def_static("rotation_axis", &Math::rotationquataxis)
        .def_static("rotation_matrix", &Math::rotationmatrix)
        .def_static("rotation_yaw_pitch_roll", &Math::rotationyawpitchroll)
        .def_static("slerp", &Math::slerp)
        .def_static("squad_setup", &Math::squadsetup)
        .def_static("squad", &Math::squad)
        .def_static("to_axis_angle", &Math::to_axisangle)       
        .def_buffer([](Math::quat &m) -> py::buffer_info {
            return py::buffer_info(
                &m.x,               /* Pointer to buffer */
                { 1, 4 },           /* Buffer dimensions */
                { 4 * sizeof(float),/* Strides (in bytes) for each index */
                sizeof(float) }
            );
        }); 
        
    py::class_<Math::vec2>(m, "Float2", py::buffer_protocol())
        .def(py::init([](){return Math::vec2();}))
        .def(py::init<float>())
        .def(py::init<float, float>())
        .def(py::init([](py::array_t<float> b)
        {
            py::buffer_info info = b.request();
            if (info.size != 2)
                throw pybind11::value_error("Invalid number of elements!");
            Math::vec2 f(b.at(0), b.at(1));
            return f;
        }))
        .def("__repr__",
            [](Math::vec2 const& val)
            {
                return Util::String::FromVec2(val);
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
        .def_readwrite("x", &Math::vec2::x)
        .def_readwrite("y", &Math::vec2::y)
        .def("length", &Math::vec2::length)
        .def("length_sq", &Math::vec2::lengthsq)
        .def("abs", &Math::vec2::abs)
        .def("any", &Math::vec2::any)
        .def("all", &Math::vec2::all)
        .def_static("multiply", &Math::vec2::multiply)
        .def_static("maximize", &Math::vec2::maximize)
        .def_static("minimize", &Math::vec2::minimize)
        .def_static("normalize", &Math::vec2::normalize)
        .def_static("lt", &Math::vec2::lt)
        .def_static("le", &Math::vec2::le)
        .def_static("gt", &Math::vec2::gt)
        .def_static("ge", &Math::vec2::ge)
        .def_buffer([](Math::vec2& f) -> py::buffer_info {
            return py::buffer_info(
                (float*)&f,             /* Pointer to buffer */
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
            else throw(std::runtime_error("Unknown class!"));
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
    else if (type == Util::Variant::Type::Vec2)
    {
        return py::cast(src.GetVec2(), policy);
    }
    else if (type == Util::Variant::Type::Vec4)
    {
        return py::detail::make_caster<Math::vec4>::cast(src.GetVec4(), policy, parent);
    }
    else if (type == Util::Variant::Type::Quaternion)
    {
        return py::detail::make_caster<Math::quat>::cast(src.GetQuat(), policy, parent);
    }
    else if (type == Util::Variant::Type::String)
    {
        return py::detail::make_caster<Util::String>::cast(src.GetString(), policy, parent);
    }
    else if (type == Util::Variant::Type::Mat4)
    {
        return py::detail::make_caster<Math::mat4>::cast(src.GetMat4(), policy, parent);
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
    else if (type == Util::Variant::Type::Vec2Array)
    {
        return py::detail::make_caster<Util::Array<Math::vec2>>::cast(src.GetVec2Array(), policy, parent);
    }
    else if (type == Util::Variant::Type::Vec4Array)
    {
        return py::detail::make_caster<Util::Array<Math::vec4>>::cast(src.GetVec4Array(), policy, parent);
    }
    else if (type == Util::Variant::Type::StringArray)
    {
        return py::detail::make_caster<Util::Array<Util::String>>::cast(src.GetStringArray(), policy, parent);
    }
    else if (type == Util::Variant::Type::Mat4Array)
    {
        return py::detail::make_caster<Util::Array<Math::mat4>>::cast(src.GetMat4Array(), policy, parent);
    }
    else if (type == Util::Variant::Type::GuidArray)
    {
        return py::detail::make_caster<Util::Array<Util::Guid>>::cast(src.GetGuidArray(), policy, parent);
    }
    // TODO: the rest of these...

    // TODO: Error handling
    throw std::runtime_error("Unimplemented variant type!\n");
    return PyBool_FromLong(0);
}
} // namespace Python
