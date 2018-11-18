//MIT License
//
//Copyright(c) 2017 Matthias M�ller
//https://github.com/TinyTinni/PyLogHook
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#pragma once

#include <utility>
#include <string>

#ifdef TYTI_PYLOGHOOK_USE_BOOST
#include <boost/python.hpp>
#else
#include <pybind11/pybind11.h>
#endif


namespace tyti {
    namespace pylog {
        namespace detail {

#ifdef TYTI_PYLOGHOOK_USE_BOOST
            template <typename T>
            inline boost::python::object LogHookMakeObject(T t)
            {
                return boost::python::object(boost::python::make_function(
                    std::forward<T>(t),
                    boost::python::default_call_policies(),
                    boost::mpl::vector<void, const char*>() // signature
                ));
            }
#else
            template<typename T>
            inline pybind11::object LogHookMakeObject(T t)
            {
                return pybind11::cpp_function(std::forward<T>(t));
            }

#endif
            template<typename T>
            void redirect_syspipe(T t, const char* pipename)
            {
                assert(Py_IsInitialized());

                PyObject* out = PySys_GetObject(pipename);

                // in case python couldnt create stdout/stderr
                // this happens in some gui application
                // just register a new nameless type for this

                if (out == NULL || out == Py_None)
                {
                    std::string register_read_write = std::string("import sys\n\
sys.") + pipename + std::string(" = type(\"\",(object,),{\"write\":lambda self, txt: None, \"flush\":lambda self: None})()\n");

                    PyRun_SimpleString(register_read_write.c_str());
                    out = PySys_GetObject(pipename);
                }
                // overwrite write function
                PyObject_SetAttrString(out, "write",
                    detail::LogHookMakeObject(std::forward<T>(t)).ptr());

            }
        } // namespace detail

        /** \brief redirects sys.stdout

        Whenever sys.stdout.write is called, call the given function instead.
        Given function has to be the signature "void (const char*)"

        Preconditions:
        Python has to be initialized.
        Calling thread has to hold the GIL. (in case of multi threading)

        Exceptions:
        Maybe a PyErr occurs. This must be handled by the user.

        @param t callbacl function of type "void (const char*)"

        */
        template<typename T>
        inline void redirect_stdout(T t)
        {
            detail::redirect_syspipe(std::forward<T>(t), "stdout");
        }

        template<typename T>
        inline void redirect_stderr(T t)
        {
            detail::redirect_syspipe(std::forward<T>(t), "stderr");
        }
    }
}
