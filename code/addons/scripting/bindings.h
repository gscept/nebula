#pragma once
// wrapper for including python binding generators for easier access
#include "pybind11/embed.h"
#include "pybind11/numpy.h"
#include "pybind11/stl.h"
#include "util/string.h"
#include "util/dictionary.h"
#include "util/variant.h"

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

        // template <typename Key, typename Value> struct type_caster<Util::Dictionary<Key, Value>>
        //     : map_caster<Util::Dictionary<Key, Value>, Key, Value> { };
        // template <typename Type> struct type_caster<Util::Array<Type>>
        //     : array_caster<Util::Array<Type>, Type, true> { };   
    }
}

