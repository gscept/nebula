//------------------------------------------------------------------------------
//  conversion.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "conversion.h"

namespace Python
{

//------------------------------------------------------------------------------
/**
*/
Util::Variant ConvertPyType(pybind11::handle const & value, Util::Variant::Type type)
{
	namespace py = pybind11;
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

//------------------------------------------------------------------------------
/**
*/
pybind11::handle VariantToPyType(Util::Variant src, pybind11::return_value_policy policy)
{
	using namespace pybind11;
	namespace py = pybind11;

	auto type = src.GetType();
	if (type == Util::Variant::Type::Byte)
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
		return py::cast(src.GetFloat2());
	}
	else if (type == Util::Variant::Type::Float4)
	{
		return py::cast(src.GetFloat4());
	}
	else if (type == Util::Variant::Type::Quaternion)
	{
		return py::cast(src.GetQuaternion());
	}
	else if (type == Util::Variant::Type::String)
	{
		Util::String str = src.GetString();
		const char *buffer = str.AsCharPtr();
		ssize_t nbytes = ssize_t(str.Length());
		handle s = PyUnicode_DecodeUTF8(buffer, nbytes, nullptr);
		return s;
	}
	else if (type == Util::Variant::Type::Matrix44)
	{
		return py::cast(src.GetMatrix44(), py::return_value_policy::copy);
	}
	else if (type == Util::Variant::Type::Transform44)
	{
		return py::cast(src.GetTransform44());
	}
	else if (type == Util::Variant::Type::Guid)
	{
		return py::cast(src.GetGuid());
	}
	else if (type == Util::Variant::Type::IntArray)
	{
		return py::cast(src.GetIntArray());
	}
	else if (type == Util::Variant::Type::FloatArray)
	{
		return py::cast(src.GetFloatArray());
	}
	else if (type == Util::Variant::Type::BoolArray)
	{
		return pybind11::cast(src.GetBoolArray());
	}
	else if (type == Util::Variant::Type::Float2Array)
	{
		return pybind11::cast(src.GetFloat2Array());
	}
	else if (type == Util::Variant::Type::Float4Array)
	{
		return pybind11::cast(src.GetFloat4Array());
	}
	else if (type == Util::Variant::Type::StringArray)
	{
		return pybind11::cast(src.GetStringArray());
	}
	else if (type == Util::Variant::Type::Matrix44Array)
	{
		return pybind11::cast(src.GetMatrix44Array());
	}
	else if (type == Util::Variant::Type::GuidArray)
	{
		return pybind11::cast(src.GetGuidArray());
	}
	// TODO: the rest of these...

	// TODO: Error handling
	return PyBool_FromLong(0);
}
} // namespace Python
