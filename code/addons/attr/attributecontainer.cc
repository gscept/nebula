//------------------------------------------------------------------------------
//  attr/attributecontainer.h
//  (C) 2005 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "attributecontainer.h"

namespace Attr
{

//------------------------------------------------------------------------------
/**
*/
AttributeContainer::AttributeContainer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AttributeContainer::~AttributeContainer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Return true if an attribute exists.
*/
bool
AttributeContainer::HasAttr(const Attr::AttrId& id) const
{
    return this->attrs.Contains(id);       
}

//------------------------------------------------------------------------------
/**
    Set a generic attribute. If the attribute exists, its
    value will be overwritten and a type check will be made (you can't
    overwrite an attribute with a different types one). If the attribute
    doesn't exist, a new attribute will be created.
*/
void
AttributeContainer::SetAttr(const Attribute& attr)
{
    if (!this->attrs.Contains(attr.GetAttrId()))
    {
        // add
        this->attrs.Add(attr.GetAttrId(), attr);
    }
    else
    {
        // set  
        this->attrs[attr.GetAttrId()].SetValue(attr.GetValue());
    }
}

//------------------------------------------------------------------------------
/**
    Add a new attribute. The attribute may not exist yet in the container,
    otherwise the result is undefined. This is faster then SetAttr() when
    many attributes are added.
*/
void
AttributeContainer::AddAttr(const Attribute& attr)
{
    this->attrs.Add(attr.GetAttrId(), attr);
}

//------------------------------------------------------------------------------
/**
    Get generic attribute. Throws a hard error if the
    attribute doesn't exist.
*/
const Attribute&
AttributeContainer::GetAttr(const Attr::AttrId& attrId) const
{
    n_assert(attrId.IsValid());
    if (this->attrs.Contains(attrId))
    {
        return this->attrs[attrId];
    }
    else
    {
        n_error("Attr::AttributeContainer::GetAttr(): attr '%s' not found!", attrId.GetName().AsCharPtr());
        return this->attrs[0]; // silence the compiler
    }
}

//------------------------------------------------------------------------------
/**
    This method provides direct read access to the attributes.
*/
const Util::Dictionary<Attr::AttrId, Attribute>&
AttributeContainer::GetAttrs() const
{
    return this->attrs;
}

//------------------------------------------------------------------------------
/**
    This method clears the attributes in the attribute container.
*/
void
AttributeContainer::Clear()
{
    this->attrs.Clear();
}

//------------------------------------------------------------------------------
/**
    This method removes an attribute from the container
*/
void
AttributeContainer::RemoveAttr(const AttrId& attrId)
{
    IndexT id = this->attrs.FindIndex(attrId);
	n_assert(id != InvalidIndex);
	this->attrs.EraseAtIndex(id);
}



};
