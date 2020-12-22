#pragma once
//------------------------------------------------------------------------------
/**
    @class Attr::AttributeContainer
    
    A simple container for attributes.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/dictionary.h"
#include "attribute.h"

//------------------------------------------------------------------------------
namespace Attr
{
class AttributeContainer
{
public:
    /// constructor
    AttributeContainer();
    /// destructor
    ~AttributeContainer();
    /// check if an attribute exists in the container
    bool HasAttr(const AttrId& attrId) const;
    /// set a single attribute, new or existing
    void SetAttr(const Attribute& attr);
    /// get a single attribute
    const Attribute& GetAttr(const AttrId& attrId) const;
    /// read access to the attribute array
    const Util::Dictionary<AttrId, Attribute>& GetAttrs() const;
    /// remove an attribute
    void RemoveAttr(const AttrId& attrId);
    /// clear the attribute container
    void Clear();
    /// add a new attribute, faster then SetAttr(), but attribute may not exist!
    void AddAttr(const Attribute& attr);
    /// set bool value
    void SetBool(const BoolAttrId& attrId, bool val);
    /// get bool value
    bool GetBool(const BoolAttrId& attrId) const;
    /// get bool value with default if not exists
    bool GetBool(const BoolAttrId& attrId, bool defaultValue) const;
    /// set float value
    void SetFloat(const FloatAttrId& attrId, float val);
    /// get float value
    float GetFloat(const FloatAttrId& attrId) const;
    /// get float value with default if not exists
    float GetFloat(const FloatAttrId& attrId, float defaultValue) const;
    /// set int value
    void SetInt(const IntAttrId& attrId, int val);
    /// get int value
    int GetInt(const IntAttrId& attrId) const;
    /// get int value with default if not exists
    int GetInt(const IntAttrId& attrId, int defaultValue) const;
    /// set string value
    void SetString(const StringAttrId& attrId, const Util::String& val);
    /// get string value
    const Util::String& GetString(const StringAttrId& attrId) const;
    /// get string value with default if not exists
    const Util::String& GetString(const StringAttrId& attrId, const Util::String& defaultValue) const;
    /// set float4 value
    void SetFloat4(const Vec4AttrId& attrId, const Math::vec4& val);
    /// get float4 value
    Math::vec4 GetVec4(const Vec4AttrId& attrId) const;
    /// get float4 value with default if not exists
    Math::vec4 GetVec4(const Vec4AttrId& attrId, const Math::vec4& defaultValue) const;
    /// set matrix44 value
    void SetMatrix44(const Mat4AttrId& attrId, const Math::mat4& val);
    /// get matrix44 value
    const Math::mat4& GetMat4(const Mat4AttrId& attrId) const;
    /// get matrix44 value with default if not exists
    const Math::mat4& GetMat4(const Mat4AttrId& attrId, const Math::mat4& defaultValue) const;
    /// set guid value
    void SetGuid(const GuidAttrId& attrId, const Util::Guid& guid);
    /// get guid value
    const Util::Guid& GetGuid(const GuidAttrId& attrId) const;
    /// get guid value with default if not exists
    const Util::Guid& GetGuid(const GuidAttrId& attrId, const Util::Guid& defaultValue) const;
    /// set blob value
    void SetBlob(const BlobAttrId& attrId, const Util::Blob& blob);
    /// get blob value
    const Util::Blob& GetBlob(const BlobAttrId& attrId) const;
    /// get blob value with default if not exists
    const Util::Blob& GetBlob(const BlobAttrId& attrId, const Util::Blob& defaultValue) const;  

private:
    /// find index for attribute (SLOW!)
    int FindAttrIndex(const Attr::AttrId& attrId) const;

    Util::Dictionary<Attr::AttrId, Attribute> attrs;
};

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeContainer::SetBool(const BoolAttrId& attrId, bool val)
{
    this->SetAttr(Attribute(attrId, val));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AttributeContainer::GetBool(const BoolAttrId& attrId) const
{
    n_assert(this->attrs.Contains(attrId));
    return this->attrs[attrId].GetBool();
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AttributeContainer::GetBool(const BoolAttrId& attrId, bool defaultValue) const
{
    if (this->HasAttr(attrId))
    {
        return this->GetBool(attrId);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeContainer::SetFloat(const FloatAttrId& attrId, float val)
{
    this->SetAttr(Attribute(attrId, val));
}

//------------------------------------------------------------------------------
/**
*/
inline float
AttributeContainer::GetFloat(const FloatAttrId& attrId) const
{
    n_assert(this->attrs.Contains(attrId));
    return this->attrs[attrId].GetFloat();
}

//------------------------------------------------------------------------------
/**
*/
inline float
AttributeContainer::GetFloat(const FloatAttrId& attrId, float defaultValue) const
{
    if (this->HasAttr(attrId))
    {
        return this->GetFloat(attrId);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeContainer::SetInt(const IntAttrId& attrId, int val)
{
    this->SetAttr(Attribute(attrId, val));
}

//------------------------------------------------------------------------------
/**
*/
inline int
AttributeContainer::GetInt(const IntAttrId& attrId) const
{
    n_assert(this->attrs.Contains(attrId));
    return this->attrs[attrId].GetInt();
}

//------------------------------------------------------------------------------
/**
*/
inline int
AttributeContainer::GetInt(const IntAttrId& attrId, int defaultValue) const
{
    if (this->HasAttr(attrId))
    {
        return this->GetInt(attrId);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeContainer::SetString(const StringAttrId& attrId, const Util::String& val)
{
    this->SetAttr(Attribute(attrId, val));
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AttributeContainer::GetString(const StringAttrId& attrId) const
{
    n_assert(this->attrs.Contains(attrId));
    return this->attrs[attrId].GetString();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
AttributeContainer::GetString(const StringAttrId& attrId, const Util::String& defaultValue) const
{
    if (this->HasAttr(attrId))
    {
        return this->GetString(attrId);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeContainer::SetFloat4(const Vec4AttrId& attrId, const Math::vec4& val)
{
    this->SetAttr(Attribute(attrId, val));
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
AttributeContainer::GetVec4(const Vec4AttrId& attrId) const
{
    n_assert(this->attrs.Contains(attrId));
    return this->attrs[attrId].GetVec4();
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
AttributeContainer::GetVec4(const Vec4AttrId& attrId, const Math::vec4& defaultValue) const
{
    if (this->HasAttr(attrId))
    {
        return this->GetVec4(attrId);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeContainer::SetMatrix44(const Mat4AttrId& attrId, const Math::mat4& val)
{
    this->SetAttr(Attribute(attrId, val));
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
AttributeContainer::GetMat4(const Mat4AttrId& attrId) const
{
    n_assert(this->attrs.Contains(attrId));
    return this->attrs[attrId].GetMat4();
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
AttributeContainer::GetMat4(const Mat4AttrId& attrId, const Math::mat4& defaultValue) const
{
    if (this->HasAttr(attrId))
    {
        return this->GetMat4(attrId);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeContainer::SetGuid(const GuidAttrId& attrId, const Util::Guid& val)
{
    this->SetAttr(Attribute(attrId, val));
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Guid&
AttributeContainer::GetGuid(const GuidAttrId& attrId) const
{
    n_assert(this->attrs.Contains(attrId));
    return this->attrs[attrId].GetGuid();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Guid&
AttributeContainer::GetGuid(const GuidAttrId& attrId, const Util::Guid& defaultValue) const
{
    if (this->HasAttr(attrId))
    {
        return this->GetGuid(attrId);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AttributeContainer::SetBlob(const BlobAttrId& attrId, const Util::Blob& val)
{
    this->SetAttr(Attribute(attrId, val));
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Blob&
AttributeContainer::GetBlob(const BlobAttrId& attrId) const
{
    n_assert(this->attrs.Contains(attrId));
    return this->attrs[attrId].GetBlob();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Blob&
AttributeContainer::GetBlob(const BlobAttrId& attrId, const Util::Blob& defaultValue) const
{
    if (this->HasAttr(attrId))
    {
        return this->GetBlob(attrId);
    }
    else
    {
        return defaultValue;
    }
}

} // namespace Attr
//------------------------------------------------------------------------------
