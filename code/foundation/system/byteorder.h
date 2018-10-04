#pragma once
//------------------------------------------------------------------------------
/**
    @class System::ByteOrder
    
    Provides information and methods to help with byte ordering issues.

    The following byte orders are defined:

    - ByteOrder::LittleEndian:  Intel byte order
    - ByteOrder::BigEndian:     Motorola byte order
    - ByteOrder::Network        network byte order (always big endian)
    - ByteOrder::Host           hardwired to little or big endian depending on host cpu

    NOTE: on console-platforms it is strongly recommended to store binary data
    in console-byte-order during asset processing on the development PC,
    so that the console doesn't waste loading time with byte conversions!
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#if !__OSX__
#include "math/float4.h"
#include "math/matrix44.h"
#endif

//------------------------------------------------------------------------------
namespace System
{
class ByteOrder
{
public:
    // byte orders
    enum Type
    {
        LittleEndian = 0,           // e.g. x86
        BigEndian,                  // e.g. PowerPC        
        Network = BigEndian,        // network byte order

        //FIXME, this is wrong
        #if __WIN32__ || __LINUX__
        Host = LittleEndian,                
        #else
        Host = BigEndian,
        #endif
    };

    /// default constructor
    ByteOrder();
    /// constructor: set byte order conversion rule
    ByteOrder(Type fromByteOrder, Type toByteOrder);
    /// set from-byte-order
    void SetFromByteOrder(Type fromByteOrder);
    /// get from-byte-order
    Type GetFromByteOrder() const;
    /// set to-byte-order
    void SetToByteOrder(Type toByteOrder);
    /// get to-byte-order
    Type GetToByteOrder() const;
    /// endian-convert in place
    template<class TYPE> void ConvertInPlace(TYPE& val) const;
    /// endian-convert by copy
    template<class TYPE> TYPE Convert(TYPE val) const;
    /// endian-convert in place
    template<class TYPE> static void ConvertInPlace(Type fromByteOrder, Type toByteOrder, TYPE& val);
    /// endian-convert by copy
    template<class TYPE> static TYPE Convert(Type fromByteOrder, Type toByteOrder, TYPE val);    

private:
    Type from;
    Type to;

    union PunFloatUL
    {
        float f;
        ulong u;
    };
    union PunDoubleULL
    {
        double d;
        unsigned long long u;
    };
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
ByteOrder::ByteOrder() :
    from(Host),
    to(Host)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
ByteOrder::ByteOrder(ByteOrder::Type fromByteOrder, ByteOrder::Type toByteOrder) :
    from(fromByteOrder),
    to(toByteOrder)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
ByteOrder::SetFromByteOrder(Type fromByteOrder)
{
    this->from = fromByteOrder;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline ByteOrder::Type
ByteOrder::GetFromByteOrder() const
{
    return this->from;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
ByteOrder::SetToByteOrder(Type toByteOrder)
{
    this->to = toByteOrder;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline ByteOrder::Type
ByteOrder::GetToByteOrder() const
{
    return this->to;
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<short>(Type fromByteOrder, Type toByteOrder, short& val)
{
    if (fromByteOrder != toByteOrder)
    {
        ushort res = _byteswap_ushort((ushort)val);
        val = (short)res;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<short>(short& val) const
{
    if (this->from != this->to)
    {
        ushort res = _byteswap_ushort((ushort)val);
        val = (short)res;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline short
ByteOrder::Convert<short>(Type fromByteOrder, Type toByteOrder, short val)
{
    if (fromByteOrder != toByteOrder)
    {
        return (short)_byteswap_ushort((ushort)val);        
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline short
ByteOrder::Convert<short>(short val) const
{
    if (this->from != this->to)
    {
        return (short)_byteswap_ushort((ushort)val);        
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<ushort>(Type fromByteOrder, Type toByteOrder, ushort& val)
{
    if (fromByteOrder != toByteOrder)
    {
        val = _byteswap_ushort(val);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<ushort>(ushort& val) const
{
    if (this->from != this->to)
    {
        val = _byteswap_ushort(val);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline ushort
ByteOrder::Convert<ushort>(Type fromByteOrder, Type toByteOrder, ushort val)
{
    if (fromByteOrder != toByteOrder)
    {
        return _byteswap_ushort(val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline ushort
ByteOrder::Convert<ushort>(ushort val) const
{
    if (this->from != this->to)
    {
        return _byteswap_ushort(val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<int>(Type fromByteOrder, Type toByteOrder, int& val)
{
    if (fromByteOrder != toByteOrder)
    {
        uint res = _byteswap_ulong((uint)val);
        val = (int)res;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<int>(int& val) const
{
    if (this->from != this->to)
    {
        uint res = _byteswap_ulong((uint)val);
        val = (int)res;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline int
ByteOrder::Convert<int>(Type fromByteOrder, Type toByteOrder, int val)
{
    if (fromByteOrder != toByteOrder)
    {
        return (int) _byteswap_ulong((uint)val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline int
ByteOrder::Convert<int>(int val) const
{
    if (this->from != this->to)
    {
        return (int) _byteswap_ulong((uint)val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<uint>(Type fromByteOrder, Type toByteOrder, uint& val)
{
    if (fromByteOrder != toByteOrder)
    {
        val = _byteswap_ulong(val);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<uint>(uint& val) const
{
    if (this->from != this->to)
    {
        val = _byteswap_ulong(val);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline uint
ByteOrder::Convert<uint>(Type fromByteOrder, Type toByteOrder, uint val)
{
    if (fromByteOrder != toByteOrder)
    {
        return _byteswap_ulong(val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline uint
ByteOrder::Convert<uint>(uint val) const
{
    if (this->from != this->to)
    {
        return _byteswap_ulong(val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<float>(Type fromByteOrder, Type toByteOrder, float& val)
{
    if (fromByteOrder != toByteOrder)
    {
        PunFloatUL pun;
        pun.f = val;
        pun.u = _byteswap_ulong(pun.u);
        val = pun.f;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<float>(float& val) const
{
    if (this->from != this->to)
    {
        PunFloatUL pun;
        pun.f = val;
        pun.u = _byteswap_ulong(pun.u);
        val = pun.f;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline float
ByteOrder::Convert<float>(Type fromByteOrder, Type toByteOrder, float val)
{
    if (fromByteOrder != toByteOrder)
    {
        PunFloatUL pun;
        pun.f = val;
        pun.u = _byteswap_ulong(pun.u);
        return pun.f;
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline float
ByteOrder::Convert<float>(float val) const
{
    if (this->from != this->to)
    {
        PunFloatUL pun;
        pun.f = val;
        pun.u = _byteswap_ulong(pun.u);
        return pun.f;
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<double>(Type fromByteOrder, Type toByteOrder, double& val)
{
    if (fromByteOrder != toByteOrder)
    {
        PunDoubleULL pun;
        pun.d = val;
        pun.u = _byteswap_uint64(pun.u);
        val = pun.d;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<double>(double& val) const
{
    if (this->from != this->to)
    {
        PunDoubleULL pun;
        pun.d = val;
        pun.u = _byteswap_uint64(pun.u);
        val = pun.d;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline double
ByteOrder::Convert<double>(Type fromByteOrder, Type toByteOrder, double val)
{
    if (fromByteOrder != toByteOrder)
    {
        PunDoubleULL pun;
        pun.d = val;
        pun.u = _byteswap_uint64(pun.u);
        return pun.d;
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline uint64_t
ByteOrder::Convert<uint64_t>(Type fromByteOrder, Type toByteOrder, uint64_t val)
{
    if (fromByteOrder != toByteOrder)
    {        
        return  _byteswap_uint64(val);     
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline int64_t
ByteOrder::Convert<int64_t>(Type fromByteOrder, Type toByteOrder, int64_t val)
{
    if (fromByteOrder != toByteOrder)
    {
        return  _byteswap_uint64(val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline int64_t
ByteOrder::Convert<int64_t>(int64_t val) const
{
    if (this->from != this->to)
    {
        return  _byteswap_uint64(val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline uint64_t
ByteOrder::Convert<uint64_t>(uint64_t val) const
{
    if (this->from != this->to)
    {
        return  _byteswap_uint64(val);
    }
    else
    {
        return val;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline double
ByteOrder::Convert<double>(double val) const
{
    if (this->from != this->to)
    {
        PunDoubleULL pun;
        pun.d = val;
        pun.u = _byteswap_uint64(pun.u);
        return pun.d;
    }
    else
    {
        return val;
    }
}

#if !__OSX__    
//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<Math::float4>(Type fromByteOrder, Type toByteOrder, Math::float4& val)
{
    if (fromByteOrder != toByteOrder)
    {
        ConvertInPlace<float>(fromByteOrder, toByteOrder, val.x());
        ConvertInPlace<float>(fromByteOrder, toByteOrder, val.y());
        ConvertInPlace<float>(fromByteOrder, toByteOrder, val.z());
        ConvertInPlace<float>(fromByteOrder, toByteOrder, val.w());
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<Math::float4>(Math::float4& val) const
{
    if (this->from != this->to)
    {
        ConvertInPlace<float>(val.x());
        ConvertInPlace<float>(val.y());
        ConvertInPlace<float>(val.z());
        ConvertInPlace<float>(val.w());
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<Math::matrix44>(Type fromByteOrder, Type toByteOrder, Math::matrix44& val)
{
    if (fromByteOrder != toByteOrder)
    {
        Math::float4 row0 = val.getrow0();
        Math::float4 row1 = val.getrow1();
        Math::float4 row2 = val.getrow2();
        Math::float4 row3 = val.getrow3();
        ConvertInPlace<Math::float4>(fromByteOrder, toByteOrder, row0);
        ConvertInPlace<Math::float4>(fromByteOrder, toByteOrder, row1);
        ConvertInPlace<Math::float4>(fromByteOrder, toByteOrder, row2);
        ConvertInPlace<Math::float4>(fromByteOrder, toByteOrder, row3);
        val.setrow0(row0);
        val.setrow1(row1);
        val.setrow2(row2);
        val.setrow3(row3);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> __forceinline void
ByteOrder::ConvertInPlace<Math::matrix44>(Math::matrix44& val) const
{
    if (this->from != this->to)
    {
        Math::float4 row0 = val.getrow0();
        Math::float4 row1 = val.getrow1();
        Math::float4 row2 = val.getrow2();
        Math::float4 row3 = val.getrow3();
        ConvertInPlace<Math::float4>(row0);
        ConvertInPlace<Math::float4>(row1);
        ConvertInPlace<Math::float4>(row2);
        ConvertInPlace<Math::float4>(row3);
        val.setrow0(row0);
        val.setrow1(row1);
        val.setrow2(row2);
        val.setrow3(row3);
    }
}
#endif // __OSX__
    
} // namespace System
//------------------------------------------------------------------------------
    