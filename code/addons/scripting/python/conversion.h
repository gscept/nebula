#pragma once
//------------------------------------------------------------------------------
/**
    Python conversion functions

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "pybind11/cast.h"
#include "util/variant.h"
#include "pybind11/embed.h"
#include "pybind11/detail/class.h"
#include "pybind11/stl.h"
#include "pybind11/numpy.h"

#pragma warning(push)
#pragma warning(disable: 4267)

namespace Python
{

//------------------------------------------------------------------------------
/**
*/
pybind11::handle VariantToPyType(Util::Variant src, pybind11::return_value_policy policy = pybind11::return_value_policy::automatic, pybind11::handle parent = NULL);

} // namespace Python

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

        template <> class type_caster<Util::Variant>
        {
        public:
            template<class T>
            bool try_load(handle src, bool convert) {
                auto caster = make_caster<T>();
                if (caster.load(src, convert)) {
                    value = cast_op<T>(caster);
                    return true;
                }
                return false;
            }

            bool load(handle src, bool convert) {
                auto loaded = {
                    false,
                    try_load<int>(src, convert),
                    try_load<uint>(src, convert),
                    try_load<float>(src, convert),
                    try_load<Math::vec4>(src, convert),
                    try_load<Math::mat4>(src, convert),
                    try_load<Util::String>(src, convert),
                    try_load<Util::Guid>(src, convert),
                    try_load<Util::Array<int>>(src, convert),
                    try_load<Util::Array<float>>(src, convert),
                    try_load<Util::Array<Math::vec4>>(src, convert),
                    try_load<Util::Array<Math::mat4>>(src, convert),
                    try_load<Util::Array<Util::String>>(src, convert),
                    try_load<Util::Array<Util::Guid>>(src, convert)
                };
                return std::any_of(loaded.begin(), loaded.end(), [](bool b) { return b; });
            }

            static handle cast(const Util::Variant& src, return_value_policy policy, handle parent) {
                return Python::VariantToPyType(src, policy, parent);
            }
            PYBIND11_TYPE_CASTER(Util::Variant, _("Variant"));
        };

        template <typename Key, typename Value> struct type_caster<Util::Dictionary<Key, Value>>
            : map_caster<Util::Dictionary<Key, Value>, Key, Value> { };
        template <typename Type> struct type_caster<Util::Array<Type>>
            : array_caster<Util::Array<Type>, Type, true> { };   
    }
}
#pragma warning(pop)