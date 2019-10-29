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
#include "pybind11/embed.h"
#include "pybind11/detail/class.h"
#include "pybind11/stl.h"

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
					try_load<Math::float4>(src, convert),
					try_load<Util::String>(src, convert),
					try_load<Util::Guid>(src, convert),
					try_load<Util::Array<int>>(src, convert),
					try_load<Util::Array<float>>(src, convert),
					try_load<Util::Array<Math::float4>>(src, convert),
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
