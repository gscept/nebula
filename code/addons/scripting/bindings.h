#pragma once
// wrapper for including python binding generators for easier access
#include "pybind11/embed.h"
#include "pybind11/numpy.h"
#include "pybind11/stl.h"
#include "util/string.h"
#include "util/dictionary.h"
#include "util/variant.h"

namespace Util
{

//------------------------------------------------------------------------------
/**
	TYPE should be a python object or similar
*/
template<typename TYPE>
Util::Variant ConvertPyType(TYPE const& value, Util::Variant::Type type)
{
	try
	{
		if (type == Util::Variant::Type::Byte)
		{
			return Util::Variant(py::cast<byte>(value));
		}
		else if (type == Util::Variant::Type::Short)
		{
			return Util::Variant(py::cast<short>(value));
		}
		else if (type == Util::Variant::Type::UShort)
		{
			return Util::Variant(py::cast<ushort>(value));
		}
		else if (type == Util::Variant::Type::Int)
		{
			return Util::Variant(py::cast<int>(value));
		}
		else if (type == Util::Variant::Type::UInt)
		{
			return Util::Variant(py::cast<uint>(value));
		}
		else if (type == Util::Variant::Type::Int64)
		{
			return Util::Variant(py::cast<int64>(value));
		}
		else if (type == Util::Variant::Type::UInt64)
		{
			return Util::Variant(py::cast<uint64>(value));
		}
		else if (type == Util::Variant::Type::Float)
		{
			return Util::Variant(py::cast<float>(value));
		}
		else if (type == Util::Variant::Type::Double)
		{
			return Util::Variant(py::cast<double>(value));
		}
		else if (type == Util::Variant::Type::Bool)
		{
			return Util::Variant(py::cast<bool>(value));
		}
		else if (type == Util::Variant::Type::Float2)
		{
			return Util::Variant(py::cast<Math::float2>(value));
		}
		else if (type == Util::Variant::Type::Float4)
		{
			return Util::Variant(py::cast<Math::float4>(value));
		}
		else if (type == Util::Variant::Type::Quaternion)
		{
			return Util::Variant(py::cast<Math::quaternion>(value));
		}
		else if (type == Util::Variant::Type::String)
		{
			return Util::Variant(py::cast<Util::String>(value));
		}
		else if (type == Util::Variant::Type::Matrix44)
		{
			return Util::Variant(py::cast<Math::matrix44>(value));
		}
		else if (type == Util::Variant::Type::Transform44)
		{
			return Util::Variant(py::cast<Math::transform44>(value));
		}
		else if (type == Util::Variant::Type::Guid)
		{
			return Util::Variant(py::cast<Util::Guid>(value));
		}
		else if (type == Util::Variant::Type::IntArray)
		{
			return Util::Variant(py::cast<Util::Array<int>>(value));
		}
		else if (type == Util::Variant::Type::FloatArray)
		{
			return Util::Variant(py::cast<Util::Array<int>>(value));
		}
		else if (type == Util::Variant::Type::BoolArray)
		{
			return Util::Variant(py::cast<Util::Array<bool>>(value));
		}
		else if (type == Util::Variant::Type::Float2Array)
		{
			return Util::Variant(py::cast<Util::Array<Math::float2>>(value));
		}
		else if (type == Util::Variant::Type::Float4Array)
		{
			return Util::Variant(py::cast<Util::Array<Math::float4>>(value));
		}
		else if (type == Util::Variant::Type::StringArray)
		{
			return Util::Variant(py::cast<Util::Array<Util::String>>(value));
		}
		else if (type == Util::Variant::Type::Matrix44Array)
		{
			return Util::Variant(py::cast<Util::Array<Math::matrix44>>(value));
		}
		else if (type == Util::Variant::Type::GuidArray)
		{
			return Util::Variant(py::cast<Util::Array<Util::Guid>>(value));
		}
	}
	catch (py::cast_error e)
	{
		n_warning("Could not convert python type to correct variant type.");
	}

	return Util::Variant();
}

} // namespace Util

namespace pybind11
{
    namespace detail
    {
        template <> class type_caster<Util::String>
        {
        public:
            bool load(handle src, bool convert)
            {
                handle load_src = src;
                if (!src)
                {
                    return false;
                }
                else if (!PyUnicode_Check(load_src.ptr()))
                {
                    return load_bytes(load_src);
                }
                object utfNbytes = reinterpret_steal<object>(PyUnicode_AsEncodedString(load_src.ptr(), "utf-8", nullptr));
                if (!utfNbytes)
                {
                    PyErr_Clear();
                    return false;
                }

                const char *buffer = reinterpret_cast<const char *>(PYBIND11_BYTES_AS_STRING(utfNbytes.ptr()));
                size_t length = (size_t)PYBIND11_BYTES_SIZE(utfNbytes.ptr());
                value.Set(buffer, length);
                
                return true;
            }
            static handle cast(const Util::String& src, return_value_policy /* policy */, handle /* parent */) {
                const char *buffer = src.AsCharPtr();
                ssize_t nbytes = ssize_t(src.Length());
                handle s = PyUnicode_DecodeUTF8(buffer, nbytes, nullptr);
                return s;
            }
            PYBIND11_TYPE_CASTER(Util::String, _(PYBIND11_STRING_NAME));

        private:
            bool load_bytes(handle src)
            {
                if (PYBIND11_BYTES_CHECK(src.ptr()))
                {
                    // We were passed a Python 3 raw bytes; accept it into a std::string or char*
                    // without any encoding attempt.
                    const char *bytes = PYBIND11_BYTES_AS_STRING(src.ptr());
                    if (bytes)
                    {
                        value.Set(bytes, (size_t)PYBIND11_BYTES_SIZE(src.ptr()));
                        return true;
                    }
                }
                return false;
            }
        };

        template <typename Key, typename Value> struct type_caster<Util::Dictionary<Key, Value>>
            : map_caster<Util::Dictionary<Key, Value>, Key, Value> { };
        template <typename Type> struct type_caster<Util::Array<Type>>
            : array_caster<Util::Array<Type>, Type, true> { };

        
    };
    class nstr : public str
    {
    public:
        nstr(const Util::String& s) : str(s.AsCharPtr()) {}
    };
}

