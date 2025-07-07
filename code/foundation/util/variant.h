#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Variant

    An "any type" variable.

    Since the Variant class has a rich set of assignment and cast operators,
    a variant variable can most of the time be used like a normal C++ variable.
    
    @copyright
    (C) 2006 RadonLabs GmbH.
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math/vec2.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/transform44.h"
#include "util/string.h"
#include "util/guid.h"
#include "util/blob.h"
#include "memory/memory.h"
#include "core/refcounted.h"
#include "util/color.h"

//------------------------------------------------------------------------------
namespace Util
{
class Variant
{
public:
    /// variant types
    enum Type
    {
        Void,
        Byte,
        Short,
        UShort,
        Int,
        UInt,
        Int64,
        UInt64,
        Float,
        Double,
        Bool,
        Vec2,
        Vec3,
        Vec4,
        Quaternion,
        String,
        Mat4,
        Transform44,
        Blob,
        Guid,
        Object,
        VoidPtr,
        IntArray,
        FloatArray,
        BoolArray,
        Vec2Array,
        Vec3Array,
        Vec4Array,
        StringArray,
        Mat4Array,
        BlobArray,
        GuidArray,
        NumTypes,
    };

    /// default constructor
    Variant();
    /// byte constructor
    Variant(byte rhs);
    /// short constructor
    Variant(short rhs);
    /// ushort constructor
    Variant(ushort rhs);
    /// int constructor
    Variant(int rhs);
    /// uint constructor
    Variant(uint rhs);
    /// int64 constructor
    Variant(int64_t rhs);
    /// uint64_t constructor
    Variant(uint64_t rhs);
    /// float constructor
    Variant(float rhs);
    /// double constructor
    Variant(double rhs);
    /// bool constructor
    Variant(bool rhs);
    /// vec2 constructor
    Variant(const Math::vec2& v);
    /// vec4 constructor
    Variant(const Math::vec3& v);
    /// vec4 constructor
    Variant(const Math::vec4& v);
    /// quaternion constructor
    Variant(const Math::quat& q);
    /// mat4 constructor
    Variant(const Math::mat4& m);
    /// transform44 constructor
    Variant(const Math::transform44& m);
    /// string constructor
    Variant(const Util::String& rhs);
    /// blob constructor
    Variant(const Util::Blob& blob);
    /// guid constructor
    Variant(const Util::Guid& guid);
    /// const char constructor
    Variant(const char* chrPtr);
    /// object constructor
    Variant(Core::RefCounted* ptr);
    /// void pointer constructor
    Variant(void* ptr);
    /// null pointer construction
    Variant(std::nullptr_t);
    /// int array constructor
    Variant(const Util::Array<int>& rhs);
    /// float array constructor
    Variant(const Util::Array<float>& rhs);
    /// bool array constructor
    Variant(const Util::Array<bool>& rhs);
    /// vec2 array constructor
    Variant(const Util::Array<Math::vec2>& rhs);
    /// vec3 array constructor
    Variant(const Util::Array<Math::vec3>& rhs);
    /// vec4 array constructor
    Variant(const Util::Array<Math::vec4>& rhs);
    /// mat4 array constructor
    Variant(const Util::Array<Math::mat4>& rhs);
    /// string array constructor
    Variant(const Util::Array<Util::String>& rhs);
    /// blob array constructor
    Variant(const Util::Array<Util::Blob>& rhs);
    /// guid array constructor
    Variant(const Util::Array<Util::Guid>& rhs);
    /// copy constructor
    Variant(const Variant& rhs);

    /// destructor
    ~Variant();
    /// set type of attribute
    void SetType(Type t);
    /// get type
    Type GetType() const;
    /// clear content, resets type to void
    void Clear();
    
    /// assignment operator
    void operator=(const Variant& rhs);
    /// byte assignment operator
    void operator=(byte val);
    /// short assignment operator
    void operator=(short val);
    /// ushort assignment operator
    void operator=(ushort val);
    /// int assignment operator
    void operator=(int val);
    /// uint assignment operator
    void operator=(uint val);
    /// int assignment operator
    void operator=(int64_t val);
    /// uint assignment operator
    void operator=(uint64_t val);
    /// float assignment operator
    void operator=(float val);
    /// double assignment operator
    void operator=(double val);
    /// bool assigment operator
    void operator=(bool val);
    /// vec2 assignment operator
    void operator=(const Math::vec2& val);
    /// vec3 assignment operator
    void operator=(const Math::vec3& val);
    /// vec4 assignment operator
    void operator=(const Math::vec4& val);
    /// quat assignment operator
    void operator=(const Math::quat& val);
    /// mat4 assignment operator
    void operator=(const Math::mat4& val);
    /// transform44 assignment operator
    void operator=(const Math::transform44& val);
    /// string assignment operator
    void operator=(const Util::String& s);
    /// blob assignment operator
    void operator=(const Util::Blob& val);
    /// guid assignment operator
    void operator=(const Util::Guid& val);
    /// char pointer assignment
    void operator=(const char* chrPtr);
    /// object assignment
    void operator=(Core::RefCounted* ptr);
    /// object assignment
    void operator=(void* ptr);
    /// int array assignment
    void operator=(const Util::Array<int>& rhs);
    /// float array assignment
    void operator=(const Util::Array<float>& rhs);
    /// bool array assignment
    void operator=(const Util::Array<bool>& rhs);
    /// vec2 array assignment
    void operator=(const Util::Array<Math::vec2>& rhs);
    /// vec3 array assignment
    void operator=(const Util::Array<Math::vec3>& rhs);
    /// vec4 array assignment
    void operator=(const Util::Array<Math::vec4>& rhs);
    /// mat4 array assignment
    void operator=(const Util::Array<Math::mat4>& rhs);
    /// string array assignment
    void operator=(const Util::Array<Util::String>& rhs);
    /// blob array assignment
    void operator=(const Util::Array<Util::Blob>& rhs);
    /// guid array assignment
    void operator=(const Util::Array<Util::Guid>& rhs);

    /// equality operator
    bool operator==(const Variant& rhs) const;
    /// byte equality operator
    bool operator==(byte rhs) const;
    /// short equality operator
    bool operator==(short rhs) const;
    /// ushort equality operator
    bool operator==(ushort rhs) const;
    /// int equality operator
    bool operator==(int rhs) const;
    /// uint equality operator
    bool operator==(uint rhs) const;
    /// int equality operator
    bool operator==(int64_t rhs) const;
    /// uint equality operator
    bool operator==(uint64_t rhs) const;
    /// float equality operator
    bool operator==(float rhs) const;
    /// double equality operator
    bool operator==(double rhs) const;
    /// bool equality operator
    bool operator==(bool rhs) const;
    /// vec2 equality operator
    bool operator==(const Math::vec2& rhs) const;
    /// vec4 equality operator
    bool operator==(const Math::vec4& rhs) const;
    /// vec4 equality operator
    bool operator==(const Math::quat& rhs) const;
    /// string equality operator
    bool operator==(const Util::String& rhs) const;
    /// guid equality operator
    bool operator==(const Util::Guid& rhs) const;
    /// char ptr equality operator
    bool operator==(const char* chrPtr) const;
    /// pointer equality operator
    bool operator==(Core::RefCounted* ptr) const;
    /// pointer equality operator
    bool operator==(void* ptr) const;

    /// inequality operator
    bool operator!=(const Variant& rhs) const;
    /// byte inequality operator
    bool operator!=(byte rhs) const;
    /// short inequality operator
    bool operator!=(short rhs) const;
    /// ushort inequality operator
    bool operator!=(ushort rhs) const;
    /// int inequality operator
    bool operator!=(int rhs) const;
    /// uint inequality operator
    bool operator!=(uint rhs) const;
    /// int inequality operator
    bool operator!=(int64_t rhs) const;
    /// uint inequality operator
    bool operator!=(uint64_t rhs) const;
    /// float inequality operator
    bool operator!=(float rhs) const;
    /// double inequality operator
    bool operator!=(double rhs) const;
    /// bool inequality operator
    bool operator!=(bool rhs) const;
    /// vec2 inequality operator
    bool operator!=(const Math::vec2& rhs) const;
    /// vec4 inequality operator
    bool operator!=(const Math::vec4& rhs) const;
    /// quaternion inequality operator
    bool operator!=(const Math::quat& rhs) const;
    /// string inequality operator
    bool operator!=(const Util::String& rhs) const;
    /// guid inequality operator
    bool operator!=(const Util::Guid& rhs) const;
    /// char ptr inequality operator
    bool operator!=(const char* chrPtr) const;
    /// pointer equality operator
    bool operator!=(Core::RefCounted* ptr) const;
    /// pointer equality operator
    bool operator!=(void* ptr) const;

    /// greater operator
    bool operator>(const Variant& rhs) const;
    /// less operator
    bool operator<(const Variant& rhs) const;
    /// greater equal operator
    bool operator>=(const Variant& rhs) const;
    /// less equal operator
    bool operator<=(const Variant& rhs) const;

    /// set byte content
    void SetByte(byte val);
    /// get byte content
    byte GetByte() const;
    /// set short content
    void SetShort(short val);
    /// get short content
    short GetShort() const;
    /// set ushort content
    void SetUShort(ushort val);
    /// get short content
    ushort GetUShort() const;
    /// set integer content
    void SetInt(int val);
    /// get integer content
    int GetInt() const;
    /// set unsigned integer content
    void SetUInt(uint val);
    /// get unsigned integer content
    uint GetUInt() const;
    /// set integer content
    void SetInt64(int64_t val);
    /// get integer content
    int64_t GetInt64() const;
    /// set unsigned integer content
    void SetUInt64(uint64_t val);
    /// get unsigned integer content
    uint64_t GetUInt64() const;
    /// set float content
    void SetFloat(float val);
    /// get float content
    float GetFloat() const;
    /// set double content
    void SetDouble(double val);
    /// get double content
    double GetDouble() const;
    /// set bool content
    void SetBool(bool val);
    /// get bool content
    bool GetBool() const;
    /// set string content
    void SetString(const Util::String& val);
    /// get string content
    const Util::String& GetString() const;
    /// set vec2 content
    void SetVec2(const Math::vec2& val);
    /// get vec2 content
    Math::vec2 GetVec2() const;
    /// set vec4 content
    void SetVec3(const Math::vec3& val);
    /// get vec4 content
    Math::vec3 GetVec3() const;
    /// set vec4 content
    void SetVec4(const Math::vec4& val);
    /// get vec4 content
    Math::vec4 GetVec4() const;
    /// set quaternion content
    void SetQuat(const Math::quat& val);
    /// get quaternion content
    Math::quat GetQuat() const;
    /// set mat4 content
    void SetMat4(const Math::mat4& val);
    /// get mat4 content
    const Math::mat4& GetMat4() const;
    /// set transform44 content
    void SetTransform44(const Math::transform44& val);
    /// get transform44 content
    const Math::transform44& GetTransform44() const;
    /// set blob 
    void SetBlob(const Util::Blob& val);
    /// get blob
    const Util::Blob& GetBlob() const;
    /// set guid content
    void SetGuid(const Util::Guid& val);
    /// get guid content
    const Util::Guid& GetGuid() const;
    /// set object pointer
    void SetObject(Core::RefCounted* ptr);
    /// get object pointer
    Core::RefCounted* GetObject() const;
    /// set opaque void pointer
    void SetVoidPtr(void* ptr);
    /// get void pointer
    void* GetVoidPtr() const;
    /// set int array content
    void SetIntArray(const Util::Array<int>& val);
    /// get int array content
    const Util::Array<int>& GetIntArray() const;
    /// set float array content
    void SetFloatArray(const Util::Array<float>& val);
    /// get float array content
    const Util::Array<float>& GetFloatArray() const;
    /// set bool array content
    void SetBoolArray(const Util::Array<bool>& val);
    /// get bool array content
    const Util::Array<bool>& GetBoolArray() const;
    /// set vec2 array content
    void SetVec2Array(const Util::Array<Math::vec2>& val);
    /// get vec2 array content
    const Util::Array<Math::vec2>& GetVec2Array() const;
    /// set vec4 array content
    void SetVec3Array(const Util::Array<Math::vec3>& val);
    /// get vec4 array content
    const Util::Array<Math::vec3>& GetVec3Array() const;
    /// set vec4 array content
    void SetVec4Array(const Util::Array<Math::vec4>& val);
    /// get vec4 array content
    const Util::Array<Math::vec4>& GetVec4Array() const;
    /// set mat4 array content
    void SetMat4Array(const Util::Array<Math::mat4>& val);
    /// get mat4 array content
    const Util::Array<Math::mat4>& GetMat4Array() const;
    /// set string array content
    void SetStringArray(const Util::Array<Util::String>& val);
    /// get string array content
    const Util::Array<Util::String>& GetStringArray() const;
    /// set guid array content
    void SetGuidArray(const Util::Array<Util::Guid>& val);
    /// get guid array content
    const Util::Array<Util::Guid>& GetGuidArray() const;
    /// set blob array content
    void SetBlobArray(const Util::Array<Util::Blob>& val);
    /// get blob array content
    const Util::Array<Util::Blob>& GetBlobArray() const;

    /// Templated get method.
    template <typename TYPE>
    TYPE Get() const;

    /// convert value to string
    Util::String ToString() const;
    /// set value from string, if type doesn't match, returns false
    bool SetParseString(const Util::String& string);
    
    /// get size of data
    const SizeT Size() const;
    /// get pointer to data
    const void* AsVoidPtr() const;

    /// create from string
    static Variant FromString(const Util::String& string);
    /// convert type to string
    static Util::String TypeToString(Type t);
    /// convert string to type
    static Type StringToType(const Util::String& str);

private:
    /// delete current content
    void Delete();
    /// copy current content
    void Copy(const Variant& rhs);

    Type type;
    union
    {
        byte i8;
        short i16;
        ushort ui16;
        int i;
        uint u;
        int64_t i64;
        uint64_t u64;
        bool b;
        double d;
        float f[4];
        Math::mat4* m;
        Math::transform44* t;
        Util::String* string;
        Util::Guid* guid;
        Util::Blob* blob;
        void* voidPtr;
        Core::RefCounted* object;
        Util::Array<int>* intArray;
        Util::Array<float>* floatArray;
        Util::Array<bool>* boolArray;
        Util::Array<Math::vec2>* vec2Array;
        Util::Array<Math::vec3>* float3Array;
        Util::Array<Math::vec4>* vec4Array;
        Util::Array<Math::mat4>* mat4Array; 
        Util::Array<Util::String>* stringArray;
        Util::Array<Util::Guid>* guidArray;
        Util::Array<Util::Blob>* blobArray;
    };
};

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant() :
    type(Void),
    string(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::Delete()
{
    if (String == this->type)
    {
        n_assert(this->string);
        delete this->string;
        this->string = 0;
    }
    else if (Mat4 == this->type)
    {
        n_assert(this->m);
        delete this->m;
        this->m = 0;
    }
    else if (Guid == this->type)
    {
        n_assert(this->guid);
        delete this->guid;
        this->guid = 0;
    }
    else if (Blob == this->type)
    {
        n_assert(this->blob);
        delete this->blob;
        this->blob = 0;
    }
    else if (Object == this->type)
    {
        if (this->object)
        {
            this->object->Release();
            this->object = 0;
        }
    }
    else if (VoidPtr == this->type)
    {
        this->voidPtr = 0;
    }
    else if (IntArray == this->type)
    {
        n_assert(this->intArray);
        delete this->intArray;
        this->intArray = 0;
    }
    else if (FloatArray == this->type)
    {
        n_assert(this->floatArray);
        delete this->floatArray;
        this->floatArray = 0;
    }
    else if (BoolArray == this->type)
    {
        n_assert(this->boolArray);
        delete this->boolArray;
        this->boolArray = 0;
    }
    else if (Vec2Array == this->type)
    {
        n_assert(this->vec2Array);
        delete this->vec2Array;
        this->vec2Array = 0;
    }
    else if (Vec3Array == this->type)
    {
        n_assert(this->float3Array);
        delete this->float3Array;
        this->float3Array = 0;
    }
    else if (Vec4Array == this->type)
    {
        n_assert(this->vec4Array);
        delete this->vec4Array;
        this->vec4Array = 0;
    }
    else if (Mat4Array == this->type)
    {
        n_assert(this->mat4Array);
        delete this->mat4Array;
        this->mat4Array = 0;
    }
    else if (StringArray == this->type)
    {
        n_assert(this->stringArray);
        delete this->stringArray;
        this->stringArray = 0;
    }
    else if (GuidArray == this->type)
    {
        n_assert(this->guidArray);
        delete this->guidArray;
        this->guidArray = 0;
    }
    else if (BlobArray == this->type)
    {
        n_assert(this->blobArray);
        delete this->blobArray;
        this->blobArray = 0;
    }
    this->type = Void;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::Clear()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::Copy(const Variant& rhs)
{
    n_assert(Void == this->type);
    this->type = rhs.type;
    switch (rhs.type)
    {
        case Void:
            break;
        case Byte:
            this->i8 = rhs.i8;
            break;
        case Short:
            this->i16 = rhs.i16;
            break;
        case UShort:
            this->ui16 = rhs.ui16;
            break;
        case Int:
            this->i = rhs.i;
            break;
        case UInt:
            this->u = rhs.u;
            break;
        case Int64:
            this->i64 = rhs.i64;
            break;
        case UInt64:
            this->u64 = rhs.u64;
            break;
        case Float:
            this->f[0] = rhs.f[0];
            break;
        case Double:
            this->d = rhs.d;
            break;
        case Bool:
            this->b = rhs.b;
            break;
        case Vec2:
            this->f[0] = rhs.f[0];
            this->f[1] = rhs.f[1];
            break;
        case Vec3:
            this->f[0] = rhs.f[0];
            this->f[1] = rhs.f[1];
            this->f[2] = rhs.f[2];
            break;
        case Vec4:
            this->f[0] = rhs.f[0];
            this->f[1] = rhs.f[1];
            this->f[2] = rhs.f[2];
            this->f[3] = rhs.f[3];
            break;
        case Quaternion:
            this->f[0] = rhs.f[0];
            this->f[1] = rhs.f[1];
            this->f[2] = rhs.f[2];
            this->f[3] = rhs.f[3];
            break;
        case String:
            this->string = new Util::String(*rhs.string);
            break;
        case Mat4:
            this->m = new Math::mat4(*rhs.m);
            break;
        case Transform44:
            this->t = new Math::transform44(*rhs.t);
            break;
        case Blob:
            this->blob = new Util::Blob(*rhs.blob);
            break;
        case Guid:
            this->guid = new Util::Guid(*rhs.guid);
            break;
        case Object:
            this->object = rhs.object;
            if (this->object)
            {
                this->object->AddRef();
            }
            break;
        case VoidPtr:
            this->voidPtr = rhs.voidPtr;
            break;
        case IntArray:
            this->intArray = new Util::Array<int>(*rhs.intArray);
            break;
        case FloatArray:
            this->floatArray = new Util::Array<float>(*rhs.floatArray);
            break;
        case BoolArray:
            this->boolArray = new Util::Array<bool>(*rhs.boolArray);
            break;
        case Vec2Array:
            this->vec2Array = new Util::Array<Math::vec2>(*rhs.vec2Array);
            break;
        case Vec3Array:
            this->float3Array = new Util::Array<Math::vec3>(*rhs.float3Array);
            break;
        case Vec4Array:
            this->vec4Array = new Util::Array<Math::vec4>(*rhs.vec4Array);
            break;
        case Mat4Array:
            this->mat4Array = new Util::Array<Math::mat4>(*rhs.mat4Array);
            break;
        case StringArray:
            this->stringArray = new Util::Array<Util::String>(*rhs.stringArray);
            break;
        case GuidArray:
            this->guidArray = new Util::Array<Util::Guid>(*rhs.guidArray);
            break;
        case BlobArray:
            this->blobArray = new Util::Array<Util::Blob>(*rhs.blobArray);
            break;
        default:
            n_error("Variant::Copy(): invalid type!");
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Variant& rhs) :
    type(Void)
{
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(byte rhs) :
    type(Byte),
    i8(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(short rhs) :
    type(Short),
    i16(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(ushort rhs) :
    type(UShort),
    ui16(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(int rhs) :
    type(Int),
    i(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(uint rhs) :
type(UInt),
    u(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(int64_t rhs) :
    type(Int64),
    i64(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(uint64_t rhs) :
    type(UInt64),
    u64(rhs)
{
    // empty
}
//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(float rhs) :
    type(Float)
{
    this->f[0] = rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(double rhs) :
    type(Double),
    d(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(bool rhs) :
    type(Bool),
    b(rhs)
{
    // empty
}

    
//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant( const Math::vec2& rhs ) :
    type(Vec2)
{
    this->f[0] = rhs.x;
    this->f[1] = rhs.y;
}

//------------------------------------------------------------------------------
/**
*/
inline 
Variant::Variant(const Math::vec3& rhs) :
    type(Vec3)
{
    rhs.storeu(this->f);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Math::vec4& rhs) :
    type(Vec4)
{
    rhs.storeu(this->f);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Math::quat& rhs) :
    type(Quaternion)
{
    rhs.storeu(this->f);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::String& rhs) :
    type(String)
{
    this->string = new Util::String(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const char* chrPtr) :
    type(String)
{
    this->string = new Util::String(chrPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(Core::RefCounted* ptr) :
    type(Object)
{
    this->object = ptr;
    if (this->object)
    {
        this->object->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline 
Variant::Variant(void* ptr) :
    type(VoidPtr)
{
    this->voidPtr = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline 
Variant::Variant(std::nullptr_t)    :
    type(VoidPtr)
{
    this->voidPtr = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Math::mat4& rhs) :
    type(Mat4)
{
    this->m = new Math::mat4(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Math::transform44& rhs) :
    type(Transform44)
{
    this->t = new Math::transform44(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Blob& rhs) :
    type(Blob)
{
    this->blob = new Util::Blob(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Guid& rhs) :
    type(Guid)
{
    this->guid = new Util::Guid(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<int>& rhs) :
    type(IntArray)
{
    this->intArray = new Util::Array<int>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<float>& rhs) :
    type(FloatArray)
{
    this->floatArray = new Util::Array<float>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<bool>& rhs) :
    type(BoolArray)
{
    this->boolArray = new Util::Array<bool>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<Math::vec2>& rhs) : 
    type(Vec2Array) 
{
    this->vec2Array = new Util::Array<Math::vec2>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<Math::vec3>& rhs) :
    type(Vec3Array)
{
    this->float3Array = new Util::Array<Math::vec3>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<Math::vec4>& rhs) :
    type(Vec4Array)
{
    this->vec4Array = new Util::Array<Math::vec4>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<Math::mat4>& rhs) :
    type(Mat4Array)
{
    this->mat4Array = new Util::Array<Math::mat4>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<Util::String>& rhs) :
    type(StringArray)
{
    this->stringArray = new Util::Array<Util::String>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<Util::Guid>& rhs) :
    type(GuidArray)
{
    this->guidArray = new Util::Array<Util::Guid>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::Variant(const Util::Array<Util::Blob>& rhs) :
    type(BlobArray)
{
    this->blobArray = new Util::Array<Util::Blob>(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline
Variant::~Variant()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetType(Type t)
{
    this->Delete();
    this->type = t;
    switch (t)
    {
        case String:
            this->string = new Util::String;
            break;
        case Mat4:
            this->m = new Math::mat4;
            break;
        case Transform44:
            this->t = new Math::transform44;
            break;
        case Blob:
            this->blob = new Util::Blob;
            break;
        case Guid:
            this->guid = new Util::Guid;
            break;
        case Object:
            this->object = 0;
            break;
        case VoidPtr:
            this->voidPtr = 0;
            break;
        case IntArray:
            this->intArray = new Util::Array<int>;
            break;
        case FloatArray:
            this->floatArray = new Util::Array<float>;
            break;
        case BoolArray:
            this->boolArray = new Util::Array<bool>;
            break;
        case Vec2Array:
            this->vec2Array = new Util::Array<Math::vec2>;
            break;
        case Vec3Array:
            this->float3Array = new Util::Array<Math::vec3>;
            break;
        case Vec4Array:
            this->vec4Array = new Util::Array<Math::vec4>;
            break;
        case Mat4Array:
            this->mat4Array = new Util::Array<Math::mat4>;
            break;
        case StringArray:
            this->stringArray = new Util::Array<Util::String>;
            break;
        case GuidArray:
            this->guidArray = new Util::Array<Util::Guid>;
            break;
        case BlobArray:
            this->blobArray = new Util::Array<Util::Blob>;
            break;
        default:
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Variant::Type
Variant::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Variant& rhs)
{
    this->Delete();
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(byte val)
{
    this->Delete();
    this->type = Byte;
    this->i8 = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(short val)
{
    this->Delete();
    this->type = Short;
    this->i16 = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(ushort val)
{
    this->Delete();
    this->type = UShort;
    this->ui16 = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(int val)
{
    this->Delete();
    this->type = Int;
    this->i = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(uint val)
{
    this->Delete();
    this->type = UInt;
    this->u = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(int64_t val)
{
    this->Delete();
    this->type = Int64;
    this->i64 = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(uint64_t val)
{
    this->Delete();
    this->type = UInt64;
    this->u64 = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(float val)
{
    this->Delete();
    this->type = Float;
    this->f[0] = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(double val)
{
    this->Delete();
    this->type = Double;
    this->d = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(bool val)
{
    this->Delete();
    this->type = Bool;
    this->b = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Variant::operator=(const Math::vec2& val)
{
    this->Delete();
    this->type = Vec2;
    this->f[0] = val.x;
    this->f[1] = val.y;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Math::vec3& val)
{
    this->Delete();
    this->type = Vec3;
    val.storeu(this->f);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Math::vec4& val)
{
    this->Delete();
    this->type = Vec4;
    val.storeu(this->f);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Math::quat& val)
{
    this->Delete();
    this->type = Quaternion;
    val.storeu(this->f);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::String& s)
{
    if (String == this->type)
    {
        *this->string = s;
    }
    else
    {
        this->Delete();
        this->string = new Util::String(s);
    }
    this->type = String;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const char* chrPtr)
{
    *this = Util::String(chrPtr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Math::mat4& val)
{
    if (Mat4 == this->type)
    {
        *this->m = val;
    }
    else
    {
        this->Delete();
        this->m = new Math::mat4(val);
    }
    this->type = Mat4;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Math::transform44& val)
{
    if (Transform44 == this->type)
    {
        *this->t = val;
    }
    else
    {
        this->Delete();
        this->t = new Math::transform44(val);
    }
    this->type = Transform44;
}
//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Guid& val)
{
    if (Guid == this->type)
    {
        *this->guid = val;
    }
    else
    {
        this->Delete();
        this->guid = new Util::Guid(val);
    }
    this->type = Guid;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Blob& val)
{
    if (Blob == this->type)
    {
        *this->blob = val;
    }
    else
    {
        this->Delete();
        this->blob = new Util::Blob(val);
    }
    this->type = Blob;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(Core::RefCounted* ptr)
{
    this->Delete();
    this->type = Object;
    this->object = ptr;
    if (this->object)
    {
        this->object->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(void* ptr)
{
    this->Delete();
    this->type = VoidPtr;
    this->voidPtr = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<int>& val)
{
    if (IntArray == this->type)
    {
        *this->intArray = val;
    }
    else
    {
        this->Delete();
        this->intArray = new Util::Array<int>(val);
    }
    this->type = IntArray;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<float>& val)
{
    if (FloatArray == this->type)
    {
        *this->floatArray = val;
    }
    else
    {
        this->Delete();
        this->floatArray = new Util::Array<float>(val);
    }
    this->type = FloatArray;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<bool>& val)
{
    if (BoolArray == this->type)
    {
        *this->boolArray = val;
    }
    else
    {
        this->Delete();
        this->boolArray = new Util::Array<bool>(val);
    }
    this->type = BoolArray;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
Variant::operator=( const Util::Array<Math::vec2>& rhs )
{
    if (Vec2Array == this->type)
    {
        *this->vec2Array = rhs;
    }
    else
    {
        this->Delete();
        this->vec2Array = new Util::Array<Math::vec2>(rhs);
    }
    this->type = Vec2Array;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<Math::vec3>& val)
{
    if (Vec3Array == this->type)
    {
        *this->float3Array = val;
    }
    else
    {
        this->Delete();
        this->float3Array = new Util::Array<Math::vec3>(val);
    }
    this->type = Vec3Array;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<Math::vec4>& val)
{
    if (Vec4Array == this->type)
    {
        *this->vec4Array = val;
    }
    else
    {
        this->Delete();
        this->vec4Array = new Util::Array<Math::vec4>(val);
    }
    this->type = Vec4Array;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<Math::mat4>& val)
{
    if (Mat4Array == this->type)
    {
        *this->mat4Array = val;
    }
    else
    {
        this->Delete();
        this->mat4Array = new Util::Array<Math::mat4>(val);
    }
    this->type = Mat4Array;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<Util::String>& val)
{
    if (StringArray == this->type)
    {
        *this->stringArray = val;
    }
    else
    {
        this->Delete();
        this->stringArray = new Util::Array<Util::String>(val);
    }
    this->type = StringArray;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<Util::Guid>& val)
{
    if (GuidArray == this->type)
    {
        *this->guidArray = val;
    }
    else
    {
        this->Delete();
        this->guidArray = new Util::Array<Util::Guid>(val);
    }
    this->type = GuidArray;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::operator=(const Util::Array<Util::Blob>& val)
{
    if (BlobArray == this->type)
    {
        *this->blobArray = val;
    }
    else
    {
        this->Delete();
        this->blobArray = new Util::Array<Util::Blob>(val);
    }
    this->type = BlobArray;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(const Variant& rhs) const
{
    if (rhs.type == this->type)
    {
        switch (rhs.type)
        {
            case Void:
                return true;
            case Byte:
                return (this->i8 == rhs.i8);
            case Short:
                return (this->i16 == rhs.i16);
            case UShort:
                return (this->ui16 == rhs.ui16);
            case Int:
                return (this->i == rhs.i);
            case UInt:
                return (this->u == rhs.u);
            case Int64:
                return (this->i64 == rhs.i64);
            case UInt64:
                return (this->u64 == rhs.u64);
            case Bool:
                return (this->b == rhs.b);
            case Float:
                return (this->f[0] == rhs.f[0]);
            case Double:
                return (this->d == rhs.d);
            case String:
                return ((*this->string) == (*rhs.string));
            case Vec2:
                return ((this->f[0] == rhs.f[0]) && (this->f[1] == rhs.f[1]));
            case Vec4:
                return ((this->f[0] == rhs.f[0]) &&
                        (this->f[1] == rhs.f[1]) &&
                        (this->f[2] == rhs.f[2]) &&
                        (this->f[3] == rhs.f[3]));
            case Quaternion:
                return ((this->f[0] == rhs.f[0]) &&
                        (this->f[1] == rhs.f[1]) &&
                        (this->f[2] == rhs.f[2]) &&
                        (this->f[3] == rhs.f[3]));
            case Guid:
                return ((*this->guid) == (*rhs.guid));
            case Blob:
                return ((*this->blob) == (*rhs.blob));
            case Object:
                return (this->object == rhs.object);
            case VoidPtr:
                return (this->voidPtr == rhs.voidPtr);
            case Mat4:
                return ((*this->m) == (*rhs.m));
            case Transform44:
                return ((*this->t) == (*rhs.t));
            default:
                n_error("Variant::operator==(): invalid variant type!");
                return false;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator>(const Variant& rhs) const
{
    if (rhs.type == this->type)
    {
        switch (rhs.type)
        {
        case Void:
            return true;
        case Byte:
            return (this->i8 > rhs.i8);
        case Short:
            return (this->i16 > rhs.i16);
        case UShort:
            return (this->ui16 > rhs.ui16);
        case Int:
            return (this->i > rhs.i);
        case UInt:
            return (this->u > rhs.u);
        case Int64:
            return (this->i64 > rhs.i64);
        case UInt64:
            return (this->u64 > rhs.u64);
        case Bool:
            return (this->b > rhs.b);
        case Float:
            return (this->f[0] > rhs.f[0]);
        case Double:
            return (this->d > rhs.d);
        case String:
            return ((*this->string) > (*rhs.string));
        case Vec2:
            return ((this->f[0] > rhs.f[0]) && (this->f[1] > rhs.f[1]));
        case Vec4:
            return ((this->f[0] > rhs.f[0]) &&
                    (this->f[1] > rhs.f[1]) &&
                    (this->f[2] > rhs.f[2]) &&
                    (this->f[3] > rhs.f[3]));
        case Quaternion:
            return ((this->f[0] > rhs.f[0]) &&
                    (this->f[1] > rhs.f[1]) &&
                    (this->f[2] > rhs.f[2]) &&
                    (this->f[3] > rhs.f[3]));
        case Guid:
            return ((*this->guid) > (*rhs.guid));
        case Blob:
            return ((*this->blob) > (*rhs.blob));
        case Object:
            return (this->object > rhs.object);
        case VoidPtr:
            return (this->voidPtr > rhs.voidPtr);
        default:
            n_error("Variant::operator>(): invalid variant type!");
            return false;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator<(const Variant& rhs) const
{
    if (rhs.type == this->type)
    {
        switch (rhs.type)
        {
        case Void:
            return true;
        case Byte:
            return (this->i8 < rhs.i8);
        case Short:
            return (this->i16 < rhs.i16);
        case UShort:
            return (this->ui16 < rhs.ui16);
        case Int:
            return (this->i < rhs.i);
        case UInt:
            return (this->u < rhs.u);
        case Int64:
            return (this->i64 < rhs.i64);
        case UInt64:
            return (this->u64 < rhs.u64);
        case Bool:
            return (this->b < rhs.b);
        case Float:
            return (this->f[0] < rhs.f[0]);
        case Double:
            return (this->d > rhs.d);
        case String:
            return ((*this->string) < (*rhs.string));
        case Vec2:
            return ((this->f[0] < rhs.f[0]) && (this->f[1] < rhs.f[1]));
        case Vec4:
            return ((this->f[0] < rhs.f[0]) &&
                    (this->f[1] < rhs.f[1]) &&
                    (this->f[2] < rhs.f[2]) &&
                    (this->f[3] < rhs.f[3]));
        case Quaternion:
            return ((this->f[0] < rhs.f[0]) &&
                    (this->f[1] < rhs.f[1]) &&
                    (this->f[2] < rhs.f[2]) &&
                    (this->f[3] < rhs.f[3]));
        case Guid:
            return ((*this->guid) < (*rhs.guid));
        case Blob:
            return ((*this->blob) < (*rhs.blob));
        case Object:
            return (this->object < rhs.object);
        case VoidPtr:
            return (this->voidPtr < rhs.voidPtr);
        default:
            n_error("Variant::operator<(): invalid variant type!");
            return false;
        }
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator>=(const Variant& rhs) const
{
    if (rhs.type == this->type)
    {
        switch (rhs.type)
        {
        case Void:
            return true;
        case Byte:
            return (this->i8 >= rhs.i8);
        case Short:
            return (this->i16 >= rhs.i16);
        case UShort:
            return (this->ui16 >= rhs.ui16);
        case Int:
            return (this->i >= rhs.i);
        case UInt:
            return (this->u >= rhs.u);
        case Int64:
            return (this->i64 >= rhs.i64);
        case UInt64:
            return (this->u64 >= rhs.u64);
        case Bool:
            return (this->b >= rhs.b);
        case Float:
            return (this->f[0] >= rhs.f[0]);
        case Double:
            return (this->d >= rhs.d);
        case String:
            return ((*this->string) >= (*rhs.string));
        case Vec2:
            return ((this->f[0] >= rhs.f[0]) && (this->f[1] >= rhs.f[1]));
        case Vec4:
            return ((this->f[0] >= rhs.f[0]) &&
                    (this->f[1] >= rhs.f[1]) &&
                    (this->f[2] >= rhs.f[2]) &&
                    (this->f[3] >= rhs.f[3]));
        case Quaternion:
            return ((this->f[0] >= rhs.f[0]) &&
                    (this->f[1] >= rhs.f[1]) &&
                    (this->f[2] >= rhs.f[2]) &&
                    (this->f[3] >= rhs.f[3]));
        case Guid:
            return ((*this->guid) >= (*rhs.guid));
        case Blob:
            return ((*this->blob) >= (*rhs.blob));
        case Object:
            return (this->object >= rhs.object);
        case VoidPtr:
            return (this->voidPtr >= rhs.voidPtr);
        default:
            n_error("Variant::operator>(): invalid variant type!");
            return false;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator<=(const Variant& rhs) const
{
    if (rhs.type == this->type)
    {
        switch (rhs.type)
        {
        case Void:
            return true;
        case Byte:
            return (this->i8 <= rhs.i8);
        case Short:
            return (this->i16 <= rhs.i16);
        case UShort:
            return (this->ui16 <= rhs.ui16);
        case Int:
            return (this->i <= rhs.i);
        case UInt:
            return (this->u <= rhs.u);
        case Int64:
            return (this->i64 <= rhs.i64);
        case UInt64:
            return (this->u64 <= rhs.u64);
        case Bool:
            return (this->b <= rhs.b);
        case Float:
            return (this->f[0] <= rhs.f[0]);
        case Double:
            return (this->d >= rhs.d);
        case String:
            return ((*this->string) <= (*rhs.string));
        case Vec2:
            return ((this->f[0] <= rhs.f[0]) && (this->f[1] <= rhs.f[1]));
        case Vec4:
            return ((this->f[0] <= rhs.f[0]) &&
                    (this->f[1] <= rhs.f[1]) &&
                    (this->f[2] <= rhs.f[2]) &&
                    (this->f[3] <= rhs.f[3]));
        case Quaternion:
            return ((this->f[0] <= rhs.f[0]) &&
                    (this->f[1] <= rhs.f[1]) &&
                    (this->f[2] <= rhs.f[2]) &&
                    (this->f[3] <= rhs.f[3]));
        case Guid:
            return ((*this->guid) <= (*rhs.guid));
        case Blob:
            return ((*this->blob) <= (*rhs.blob));
        case Object:
            return (this->object <= rhs.object);
        case VoidPtr:
            return (this->voidPtr <= rhs.voidPtr);
        default:
            n_error("Variant::operator<(): invalid variant type!");
            return false;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(const Variant& rhs) const
{
    return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(byte rhs) const
{
    n_assert(Byte == this->type);
    return (this->i8 == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(short rhs) const
{
    n_assert(Short == this->type);
    return (this->i16 == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(ushort rhs) const
{
    n_assert(UShort == this->type);
    return (this->ui16 == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(int rhs) const
{
    n_assert(Int == this->type);
    return (this->i == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(uint rhs) const
{
    n_assert(UInt == this->type);
    return (this->u == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(int64_t rhs) const
{
    n_assert(Int64 == this->type);
    return (this->i64 == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(uint64_t rhs) const
{
    n_assert(UInt64 == this->type);
    return (this->u64 == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(float rhs) const
{
    n_assert(Float == this->type);
    return (this->f[0] == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(double rhs) const
{
    n_assert(Double == this->type);
    return (this->d == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(bool rhs) const
{
    n_assert(Bool == this->type);
    return (this->b == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(const Util::String& rhs) const
{
    n_assert(String == this->type);
    return ((*this->string) == rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(const char* chrPtr) const
{
    return *this == Util::String(chrPtr);
}


//------------------------------------------------------------------------------
/**
*/
inline bool 
Variant::operator==(const Math::vec2& rhs) const
{
    n_assert(Vec2 == this->type);
    return (this->f[0] == rhs.x && this->f[1] == rhs.y);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(const Math::vec4& rhs) const
{
    n_assert(Vec4 == this->type);
    return ((this->f[0] == rhs.x) &&
            (this->f[1] == rhs.y) &&
            (this->f[2] == rhs.z) &&
            (this->f[3] == rhs.w));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(const Math::quat& rhs) const
{
    n_assert(Quaternion == this->type);
    return ((this->f[0] == rhs.x) &&
            (this->f[1] == rhs.y) &&
            (this->f[2] == rhs.z) &&
            (this->f[3] == rhs.w));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(const Util::Guid& rhs) const
{
    n_assert(Guid == this->type);
    return (*this->guid) == rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(Core::RefCounted* ptr) const
{
    n_assert(Object == this->type);
    return this->object == ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator==(void* ptr) const
{
    n_assert(VoidPtr == this->type);
    return this->voidPtr == ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(byte rhs) const
{
    n_assert(Byte == this->type);
    return (this->i8 != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(short rhs) const
{
    n_assert(Short == this->type);
    return (this->i16 != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(ushort rhs) const
{
    n_assert(UShort == this->type);
    return (this->ui16 != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(int rhs) const
{
    n_assert(Int == this->type);
    return (this->i != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(uint rhs) const
{
    n_assert(UInt == this->type);
    return (this->u != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(int64_t rhs) const
{
    n_assert(Int64 == this->type);
    return (this->i64 != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(uint64_t rhs) const
{
    n_assert(UInt64 == this->type);
    return (this->u64 != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(float rhs) const
{
    n_assert(Float == this->type);
    return (this->f[0] != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(double rhs) const
{
    n_assert(Double == this->type);
    return (this->d != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(bool rhs) const
{
    n_assert(Bool == this->type);
    return (this->b != rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(const Util::String& rhs) const
{
    n_assert(String == this->type);
    return (*this->string) != rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(const char* chrPtr) const
{
    return *this != Util::String(chrPtr);
}


//------------------------------------------------------------------------------
/**
*/
inline bool 
Variant::operator!=( const Math::vec2& rhs ) const
{
    n_assert(Vec2 == this->type);
    return (this->f[0] != rhs.x || this->f[1] != rhs.y);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(const Math::vec4& rhs) const
{
    n_assert(Vec4 == this->type);
    return ((this->f[0] != rhs.x) ||
            (this->f[1] != rhs.y) ||
            (this->f[2] != rhs.z) ||
            (this->f[3] != rhs.w));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(const Math::quat& rhs) const
{
    n_assert(Quaternion == this->type);
    return ((this->f[0] != rhs.x) ||
            (this->f[1] != rhs.y) ||
            (this->f[2] != rhs.z) ||
            (this->f[3] != rhs.w));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(const Util::Guid& rhs) const
{
    n_assert(Guid == this->type);
    return (*this->guid) != rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(Core::RefCounted* ptr) const
{
    n_assert(Object == this->type);
    return (this->object == ptr);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::operator!=(void* ptr) const
{
    n_assert(VoidPtr == this->type);
    return (this->voidPtr == ptr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetByte(byte val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline byte
Variant::GetByte() const
{
    n_assert(Byte == this->type);
    return this->i8;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetShort(short val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetUShort(ushort val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline short
Variant::GetShort() const
{
    n_assert(Short == this->type);
    return this->i16;
}

//------------------------------------------------------------------------------
/**
*/
inline ushort
Variant::GetUShort() const
{
    n_assert(UShort == this->type);
    return this->ui16;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetInt(int val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetUInt(uint val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline int
Variant::GetInt() const
{
    n_assert(Int == this->type);
    return this->i;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
Variant::GetUInt() const
{
    n_assert(UInt == this->type);
    return this->u;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetInt64(int64_t val)
{
    n_assert(Int64 == this->type);
    this->i64 = val;
}

//------------------------------------------------------------------------------
/**
*/
inline int64_t
Variant::GetInt64() const
{
    n_assert(Int64 == this->type);
    return this->i64;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetUInt64(uint64_t val)
{
    n_assert(UInt64 == this->type);
    this->u64 = val;
}

//------------------------------------------------------------------------------
/**
*/
inline uint64_t
Variant::GetUInt64() const
{
    n_assert(UInt64 == this->type);
    return this->u64;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetFloat(float val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline float
Variant::GetFloat() const
{
    n_assert(Float == this->type);
    return this->f[0];
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetDouble(double val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline double
Variant::GetDouble() const
{
    n_assert(Double == this->type);
    return this->d;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetBool(bool val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Variant::GetBool() const
{
    n_assert(Bool == this->type);
    return this->b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetString(const Util::String& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
Variant::GetString() const
{
    n_assert(String == this->type);
    return *(this->string);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetVec2(const Math::vec2& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec2 
Variant::GetVec2() const
{
    n_assert(Vec2 == this->type);
    return Math::vec2(this->f[0], this->f[1]);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Variant::SetVec3(const Math::vec3& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec3 
Variant::GetVec3() const
{
    return Math::vec3();
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetVec4(const Math::vec4& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
Variant::GetVec4() const
{
    n_assert(Vec4 == this->type);
    return Math::vec4(this->f[0], this->f[1], this->f[2], this->f[3]);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetQuat(const Math::quat& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline Math::quat
Variant::GetQuat() const
{
    n_assert(Quaternion == this->type);
    return Math::quat(this->f[0], this->f[1], this->f[2], this->f[3]);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetMat4(const Math::mat4& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
Variant::GetMat4() const
{
    n_assert(Mat4 == this->type);
    return *(this->m);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetTransform44(const Math::transform44& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::transform44&
Variant::GetTransform44() const
{
    n_assert(Transform44 == this->type);
    return *(this->t);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetGuid(const Util::Guid& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Guid&
Variant::GetGuid() const
{
    n_assert(Guid == this->type);
    return *(this->guid);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetBlob(const Util::Blob& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Blob&
Variant::GetBlob() const
{
    n_assert(Blob == this->type);
    return *(this->blob);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetObject(Core::RefCounted* ptr)
{
    *this = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline Core::RefCounted*
Variant::GetObject() const
{
    n_assert(Object == this->type);
    return this->object;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Util::Variant::SetVoidPtr(void* ptr)
{
    *this = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline void* 
Util::Variant::GetVoidPtr() const
{
    n_assert(VoidPtr == this->type);
    return this->voidPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetIntArray(const Util::Array<int>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<int>&
Variant::GetIntArray() const
{
    n_assert(IntArray == this->type);
    return *(this->intArray);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetFloatArray(const Util::Array<float>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<float>&
Variant::GetFloatArray() const
{
    n_assert(FloatArray == this->type);
    return *(this->floatArray);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetBoolArray(const Util::Array<bool>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<bool>&
Variant::GetBoolArray() const
{
    n_assert(BoolArray == this->type);
    return *(this->boolArray);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Variant::SetVec2Array( const Util::Array<Math::vec2>& val )
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Math::vec2>& 
Variant::GetVec2Array() const
{
    n_assert(Vec2Array == this->type);
    return *(this->vec2Array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetVec3Array(const Util::Array<Math::vec3>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Math::vec3>&
Variant::GetVec3Array() const
{
    n_assert(Vec3Array == this->type);
    return *(this->float3Array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetVec4Array(const Util::Array<Math::vec4>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Math::vec4>&
Variant::GetVec4Array() const
{
    n_assert(Vec4Array == this->type);
    return *(this->vec4Array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetMat4Array(const Util::Array<Math::mat4>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Math::mat4>&
Variant::GetMat4Array() const
{
    n_assert(Mat4Array == this->type);
    return *(this->mat4Array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetStringArray(const Util::Array<Util::String>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::String>&
Variant::GetStringArray() const
{
    n_assert(StringArray == this->type);
    return *(this->stringArray);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetGuidArray(const Util::Array<Util::Guid>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::Guid>&
Variant::GetGuidArray() const
{
    n_assert(GuidArray == this->type);
    return *(this->guidArray);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Variant::SetBlobArray(const Util::Array<Util::Blob>& val)
{
    *this = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::Blob>&
Variant::GetBlobArray() const
{
    n_assert(BlobArray == this->type);
    return *(this->blobArray);
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline TYPE
Variant::Get() const
{
    static_assert(true, "Get method for TYPE is not implemented!");
    TYPE ret;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline byte
Variant::Get() const
{
    return this->GetByte();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline short
Variant::Get() const
{
    return this->GetShort();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline int
Variant::Get() const
{
    return this->GetInt();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline uint
Variant::Get() const
{
    return this->GetUInt();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline int64_t
Variant::Get() const
{
    return this->GetInt64();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline uint64_t
Variant::Get() const
{
    return this->GetUInt64();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline bool
Variant::Get() const
{
    return this->GetBool();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline double
Variant::Get() const
{
    return this->GetDouble();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline float
Variant::Get() const
{
    return this->GetFloat();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Math::vec2
Variant::Get() const
{
    return this->GetVec2();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Math::vec3
Variant::Get() const
{
    return this->GetVec3();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Math::vec4
Variant::Get() const
{
    return this->GetVec4();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Math::quat
Variant::Get() const
{
    return this->GetQuat();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Math::mat4
Variant::Get() const
{
    return this->GetMat4();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Math::transform44
Variant::Get() const
{
    return this->GetTransform44();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::String
Variant::Get() const
{
    return this->GetString();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Guid
Variant::Get() const
{
    return this->GetGuid();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Blob
Variant::Get() const
{
    return this->GetBlob();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<int>
Variant::Get() const
{
    return this->GetIntArray();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<float>
Variant::Get() const
{
    return this->GetFloatArray();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<bool>
Variant::Get() const
{
    return this->GetBoolArray();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<Math::vec2>
Variant::Get() const
{
    return this->GetVec2Array();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<Math::vec3>
Variant::Get() const
{
    return this->GetVec3Array();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<Math::vec4>
Variant::Get() const
{
    return this->GetVec4Array();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<Math::mat4>
Variant::Get() const
{
    return this->GetMat4Array();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<Util::String>
Variant::Get() const
{
    return this->GetStringArray();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<Util::Guid>
Variant::Get() const
{
    return this->GetGuidArray();
}

//------------------------------------------------------------------------------
/**
*/
template <>
inline Util::Array<Util::Blob>
Variant::Get() const
{
    return this->GetBlobArray();
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String 
Variant::ToString() const
{
    Util::String retval = "";
    switch (this->type)
    {
        case Void:          break;
        case Byte:          { retval = Util::String::FromByte(this->GetByte()); break; }
        case Short:         { retval = Util::String::FromShort(this->GetShort()); break; }
        case UShort:        { retval = Util::String::FromUShort(this->GetUShort()); break; }
        case Int:           { retval = Util::String::FromInt(this->GetInt()); break; }
        case UInt:          { retval = Util::String::FromUInt(this->GetUInt()); break; }
        case Float:         { retval = Util::String::FromFloat(this->GetFloat()); break; }
        case Double:        { retval = Util::String::FromDouble(this->GetDouble()); break; }
        case Bool:          { retval = Util::String::FromBool(this->GetBool()); break; }
        case Vec2:          { retval = Util::String::FromVec2(this->GetVec2()); break; }
        case Vec3:          { retval = Util::String::FromVec3(this->GetVec3()); break; }
        case Vec4:          { retval = Util::String::FromVec4(this->GetVec4()); break; }
        case Quaternion:    { retval = Util::String::FromQuat(this->GetQuat()); break; }
        case String:        { retval = this->GetString(); break; }
        case Mat4:          { retval = Util::String::FromMat4(this->GetMat4()); break; }
        case Transform44:   { retval = Util::String::FromTransform44(this->GetTransform44()); break; }
        case Blob:          { retval = Util::String::FromBlob(this->GetBlob()); break; }
        case Guid:          { retval = this->GetGuid().AsString(); break; }
        case IntArray:      { const Util::Array<int>& arr = this->GetIntArray(); for (int i = 0; i < arr.Size(); i++) { retval.AppendInt(arr[i]); retval += (i < arr.Size() - 1 ? "," : ""); } break; }
        case FloatArray:    { const Util::Array<float>& arr = this->GetFloatArray(); for (int i = 0; i < arr.Size(); i++) { retval.AppendFloat(arr[i]); retval += (i < arr.Size() - 1 ? "," : ""); } break; }
        case BoolArray:     { const Util::Array<bool>& arr = this->GetBoolArray(); for (int i = 0; i < arr.Size(); i++) { retval.AppendBool(arr[i]); retval += (i < arr.Size() - 1 ? "," : ""); } break; }
        case StringArray:   { const Util::Array<Util::String>& arr = this->GetStringArray(); for (int i = 0; i < arr.Size(); i++) { retval.Append(arr[i]); retval += (i < arr.Size() - 1 ? "," : ""); } break; }
        default:
            n_error("Variant::ToString(): invalid type enum '%d'", this->type);

    }
    return retval;
}

//------------------------------------------------------------------------------
/**
    Note:
        Will treat vec2, vec4 and mat4 as float arrays
*/
inline bool 
Util::Variant::SetParseString(const Util::String& string)
{
    bool retval = false;
    switch (this->type)
    {
        case Int:
        case UInt:              { if(string.IsValidInt())          { this->SetUInt(string.AsInt()); retval = true; } break; };
        case Float:             { if(string.IsValidFloat())        { this->SetFloat(string.AsFloat()); retval = true; } break; }
        case Bool:              { if(string.IsValidBool())         { this->SetBool(string.AsBool()); retval = true; } break; }
        case Vec2:              { if(string.IsValidVec2())         { this->SetVec2(string.AsVec2()); retval = true; } break; }
        case Vec4:              { if(string.IsValidVec4())         { this->SetVec4(string.AsVec4()); retval = true; } break; }
        case String:            {                                    this->SetString(string); retval = true; break; }
        case Mat4:              { if(string.IsValidMat4())         { this->SetMat4(string.AsMat4()); retval = true; } break; }
        case Transform44:       { if (string.IsValidTransform44()) { this->SetTransform44(string.AsTransform44()); retval = true; } break; }

        case IntArray:          
            { 
                Util::Array<Util::String> tokens = string.Tokenize(",");
                Util::Array<int> result;
                IndexT i;
                for (i = 0; i < tokens.Size(); i++)
                {
                    if (tokens[i].IsValidInt())
                    {
                        result.Append(tokens[i].AsInt());
                    }
                }
                this->SetIntArray(result);
                retval = true;
                break;
            }
        case FloatArray:        
            { 
                Util::Array<Util::String> tokens = string.Tokenize(",");
                Util::Array<float> result;
                IndexT i;
                for (i = 0; i < tokens.Size(); i++)
                {
                    if (tokens[i].IsValidFloat())
                    {
                        result.Append(tokens[i].AsFloat());
                    }
                }
                this->SetFloatArray(result);
                retval = true;
                break;
            }
        case BoolArray:         
            {
                Util::Array<Util::String> tokens = string.Tokenize(",");
                Util::Array<bool> result;
                IndexT i;
                for (i = 0; i < tokens.Size(); i++)
                {
                    if (tokens[i].IsValidBool())
                    {
                        result.Append(tokens[i].AsBool());
                    }
                }
                this->SetBoolArray(result);
                retval = true;
                break;
            }
        case StringArray:       
            {
                Util::Array<Util::String> tokens = string.Tokenize(",");
                this->SetStringArray(tokens);
                retval = true;
                break;
            }
        default:
        break;
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
inline 
const SizeT 
Variant::Size() const
{
    switch (this->type)
    {
    case Void:          return 0;
    case Byte:          return sizeof(uint8_t);
    case Short:         return sizeof(uint16_t);
    case UShort:        return sizeof(uint16_t);
    case Int:           return sizeof(uint32_t);
    case UInt:          return sizeof(uint32_t);
    case Int64:         return sizeof(uint64_t);
    case UInt64:        return sizeof(uint64_t);
    case Float:         return sizeof(float);
    case Double:        return sizeof(double);
    case Bool:          return sizeof(bool);
    case Vec2:      return sizeof(float) * 2;
    case Vec4:          return sizeof(float) * 4;
    case Quaternion:    return sizeof(float) * 4;
    case String:        return sizeof(Util::String);
    case Mat4:          return sizeof(Math::mat4);
    case Transform44:   return sizeof(Math::transform44);
    case Blob:          return sizeof(Util::Blob);
    case Guid:          return sizeof(Util::Guid);
    case Object:        return sizeof(Core::RefCounted*);
    case VoidPtr:       return sizeof(void*);
    case IntArray:      return sizeof(Util::Array<int>);
    case FloatArray:    return sizeof(Util::Array<float>);
    case BoolArray:     return sizeof(Util::Array<bool>);
    case Vec2Array:     return sizeof(Util::Array<Math::vec2>);
    case Vec4Array:     return sizeof(Util::Array<Math::vec4>);
    case Mat4Array:     return sizeof(Util::Array<Math::mat4>);
    case StringArray:   return sizeof(Util::Array<Util::String>);
    case GuidArray:     return sizeof(Util::Array<Util::Guid>);
    case BlobArray:     return sizeof(Util::Array<Util::Blob>);
    default:
        n_error("Variant::Size(): invalid type enum '%d'!", this->type);
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline 
const void*
Variant::AsVoidPtr() const
{
    switch (this->type)
    {
    case Void:
        return nullptr;
    case Byte:
    case Short:
    case UShort:
    case Int:
    case UInt:
    case Int64:
    case UInt64:
    case Float:
    case Double:
    case Bool:
    case Vec2:
    case Vec4:
    case Quaternion:
    case Object: // object and void ptr will be returned as pointers, not "as content"
    case VoidPtr:
        return &this->i8;
    case String:
    case Mat4:
    case Transform44:
    case Blob:
    case Guid:
    case IntArray:
    case FloatArray:
    case BoolArray:
    case Vec2Array:
    case Vec4Array:
    case Mat4Array:
    case StringArray:
    case GuidArray:
    case BlobArray:
        return this->voidPtr;
    default:
        n_error("Variant::AsVoidPtr(): invalid type enum '%d'!", this->type);
        return nullptr;
    }
}

//------------------------------------------------------------------------------
/**
    Only some types can be inferred from string, everything else will be treated as a raw string
*/
inline Variant 
Variant::FromString(const Util::String& string)
{
    Variant val;
    if      (string.IsValidInt())       val.SetInt(string.AsInt());
    else if (string.IsValidFloat())     val.SetFloat(string.AsFloat());
    else if (string.IsValidVec2())    val.SetVec2(string.AsVec2());
    else if (string.IsValidVec4())    val.SetVec4(string.AsVec4());
    else if (string.IsValidBool())      val.SetBool(string.AsBool());
    else if (string.IsValidMat4())  val.SetMat4(string.AsMat4());
    else                                val.SetString(string);                  // raw string
    return val;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
Variant::TypeToString(Type t)
{
    switch (t)
    {
        case Void:          return "void";
        case Byte:          return "byte";
        case Short:         return "short";
        case UShort:        return "ushort";
        case Int:           return "int";
        case UInt:          return "uint";
        case Int64:         return "int64";
        case UInt64:        return "uint64_t";
        case Float:         return "float";
        case Double:        return "double";
        case Bool:          return "bool";
        case Vec2:          return "vec2";
        case Vec4:          return "vec4";
        case Quaternion:    return "quaternion";
        case String:        return "string";
        case Mat4:          return "mat4";
        case Transform44:   return "transform44";
        case Blob:          return "blob";
        case Guid:          return "guid";
        case Object:        return "object";
        case VoidPtr:       return "voidptr";
        case IntArray:      return "intarray";
        case FloatArray:    return "floatarray";
        case BoolArray:     return "boolarray";
        case Vec2Array:     return "vec2array";
        case Vec4Array:     return "vec4array";
        case Mat4Array:     return "mat4array";
        case StringArray:   return "stringarray";
        case GuidArray:     return "guidarray";
        case BlobArray:     return "blobarray";
        default:
            n_error("Variant::TypeToString(): invalid type enum '%d'!", t);
            return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Variant::Type
Variant::StringToType(const Util::String& str)
{
    if      ("void" == str)             return Void;
    else if ("byte" == str)             return Byte;
    else if ("short" == str)            return Short;
    else if ("ushort" == str)           return UShort;
    else if ("int" == str)              return Int;
    else if ("uint" == str)             return UInt;
    else if ("int64" == str)            return Int64;
    else if ("uint64_t" == str)           return UInt64;
    else if ("float" == str)            return Float;
    else if ("double" == str)           return Double;
    else if ("bool" == str)             return Bool;
    else if ("vec2" == str)             return Vec2;
    else if ("vec4" == str)             return Vec4;
    else if ("color" == str)            return Vec4; // NOT A BUG!
    else if ("quaternion" == str)       return Quaternion;
    else if ("string" == str)           return String;
    else if ("mat4" == str)             return Mat4;
    else if ("transform44" == str)      return Transform44;
    else if ("blob" == str)             return Blob;
    else if ("guid" == str)             return Guid;
    else if ("object" == str)           return Object;
    else if ("voidptr" == str)          return VoidPtr;
    else if ("intarray" == str)         return IntArray;
    else if ("floatarray" == str)       return FloatArray;
    else if ("boolarray" == str)        return BoolArray;
    else if ("vec2array" == str)        return Vec2Array;
    else if ("vec4array" == str)        return Vec4Array;
    else if ("mat4array" == str)        return Mat4Array;
    else if ("stringarray" == str)      return StringArray;
    else if ("guidarray" == str)        return GuidArray;
    else if ("blobarray" == str)        return BlobArray;
    else
    {
        n_error("Variant::StringToType(): invalid type string '%s'!", str.AsCharPtr());
        return Void;
    }
}


} // namespace Util
//------------------------------------------------------------------------------
