#pragma once
//------------------------------------------------------------------------------
/**
    Python conversion functions

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/variant.h"
#include "nanobind/nanobind.h"
#include "nanobind/stl/detail/nb_dict.h"
#include "nanobind/stl/detail/nb_array.h"
#include "nanobind/stl/detail/nb_list.h"

#pragma warning(push)
#pragma warning(disable: 4267)

namespace Python
{
//------------------------------------------------------------------------------
/**
*/
void RegisterNebulaModules();
//------------------------------------------------------------------------------
/**
*/
nanobind::handle VariantToPyType(Util::Variant src, nanobind::rv_policy policy = nanobind::rv_policy::automatic, nanobind::detail::cleanup_list* cleanup = nullptr);

} // namespace Python

namespace nanobind
{
    namespace detail
    {
        template <> struct type_caster<Util::String>
        {
        public:
            bool from_python(handle src, uint8_t flags, cleanup_list* ) noexcept
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
                object utfNbytes = steal<object>(PyUnicode_AsEncodedString(load_src.ptr(), "utf-8", nullptr));
                if (!utfNbytes)
                {
                    PyErr_Clear();
                    return false;
                }

                const char *buffer = reinterpret_cast<const char *>(PyBytes_AsString(utfNbytes.ptr()));
                size_t length = (size_t)PyBytes_Size(utfNbytes.ptr());
                value.Set(buffer, length);
                
                return true;
            }
            static handle from_cpp(const Util::String& src, rv_policy /* policy */, cleanup_list*) noexcept {
                const char *buffer = src.AsCharPtr();
                size_t nbytes = size_t(src.Length());
                handle s = PyUnicode_DecodeUTF8(buffer, nbytes, nullptr);
                return s;
            }
            NB_TYPE_CASTER(Util::String, const_name("str"));

        private:
            bool load_bytes(handle src)
            {
                if (PyBytes_Check(src.ptr()))
                {
                    // We were passed a Python 3 raw bytes; accept it into a std::string or char*
                    // without any encoding attempt.
                    const char *bytes = PyBytes_AsString(src.ptr());
                    if (bytes)
                    {
                        value.Set(bytes, (size_t)PyBytes_Size(src.ptr()));
                        return true;
                    }
                }
                return false;
            }
        };

#if 0
        // this needs a different approach, ownership between c++ and python is not clear atm
        template <> class type_caster<Util::Variant>
        {
            template <typename T> using Caster = make_caster<detail::intrinsic_t<T>>;
        public:
            template<class T>
            bool try_load(handle src, uint8_t flags, cleanup_list* cleanup) 
            {
                Caster<T> caster;
                if (!caster.from_python(src, flags, cleanup))
                {
                    return false;
                }
                
                if constexpr (is_pointer_v<T>) 
                {
                    static_assert(Caster<T>::IsClass,
                        "Binding 'Variant<T*,...>' requires that 'T' can also be bound by nanobind.");
                    value = caster.operator cast_t<T>();
                }
                else if constexpr (Caster<T>::IsClass) 
                {
                    // Non-pointer classes do not accept a null pointer
                    if (src.is_none())
                        return false;
                    value = caster.operator cast_t<T&>();
                }
                else 
                {
                    value = std::move(caster).operator cast_t<T&&>();
                }
                return true;
            }

            bool from_python(handle src, uint8_t flags, cleanup_list* cleanup) noexcept {
                auto loaded = {
                    false,
                    try_load<int>(src, flags, cleanup),
                    try_load<uint>(src, flags, cleanup),
                    try_load<float>(src, flags, cleanup),
                    try_load<Math::vec4>(src, flags, cleanup),
                    try_load<Math::mat4>(src, flags, cleanup),
                    try_load<Util::String>(src, flags, cleanup),
                    try_load<Util::Guid>(src, flags, cleanup),
                    try_load<Util::Array<int>>(src, flags, cleanup),
                    try_load<Util::Array<float>>(src, flags, cleanup),
                    try_load<Util::Array<Math::vec4>>(src, flags, cleanup),
                    try_load<Util::Array<Math::mat4>>(src, flags, cleanup),
                    try_load<Util::Array<Util::String>>(src, flags, cleanup),
                    try_load<Util::Array<Util::Guid>>(src, flags, cleanup)
                };
                return std::any_of(loaded.begin(), loaded.end(), [](bool b) { return b; });
            }

            static handle from_cpp(const Util::Variant& src, rv_policy policy, cleanup_list* cleanup) noexcept {
                return Python::VariantToPyType(src, policy, cleanup);
            }
            NB_TYPE_CASTER(Util::Variant, const_name("Variant"));
        };
#endif
        template <typename Key, typename Value> struct type_caster<Util::Dictionary<Key, Value>>
            : nanobind::detail::dict_caster<Util::Dictionary<Key, Value>, Key, Value> { };
        template <typename Type> struct type_caster<Util::Array<Type>>
            : list_caster<Util::Array<Type>, Type> { };
    }
}
#pragma warning(pop)
