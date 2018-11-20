#pragma once
// wrapper for including python binding generators for easier access
#include "pybind11/embed.h"
#include "pybind11/numpy.h"
#include "util/string.h"

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
        /*
        template <> struct type_caster<Game::Entity>
        {
        public:            
            PYBIND11_TYPE_CASTER(Game::Entity, _("game.entity"));
            
            bool load(handle src, bool) {                
                PyObject *source = src.ptr();                
                PyObject *tmp = PyNumber_Long(source);
                if (!tmp)
                    return false;                
                value = PyLong_AsLong(tmp);
                Py_DECREF(tmp);                
                return true;
            }
            
            static handle cast(Game::Entity src, return_value_policy policy , handle  parent ) {
                return PyLong_FromLong(src.id);
            }
        };
    */
    };
    class nstr : public str
    {
    public:
        nstr(const Util::String& s) : str(s.AsCharPtr()) {}
    };
}

