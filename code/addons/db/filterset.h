#pragma once
#ifndef DB_FILTERSET_H
#define DB_FILTERSET_H
//------------------------------------------------------------------------------
/**
    @class Db::FilterSet

    Implements a filter for datatbase data using a condition tree. A filter
    can be compiled into a SQL WHERE statement.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "attr/attribute.h"
#include "util/simpletree.h"

//------------------------------------------------------------------------------
namespace Db
{
class Command;

class FilterSet : public Core::RefCounted
{
    __DeclareClass(FilterSet);
public:
    /// constructor
    FilterSet();
    /// destructor
    virtual ~FilterSet();
    /// clear the filter
    void Clear();
    /// clear the dirty state of the filter (every op on the filter will make it dirty)
    void ClearDirtyFlag();
    /// return true if the filter is dirty
    bool IsDirty() const;
    /// return true if the filter is empty
    bool IsEmpty() const;
    /// begin a new level in the condition tree (opens a bracket)
    void BeginBlock();
    /// end the current level in the condition tree (close current bracket)
    void EndBlock();
    /// add an equality check
    void AddEqualCheck(const Attr::Attribute& attr);
    /// add a greater check
    void AddGreaterThenCheck(const Attr::Attribute& attr);
    /// add a lesser check
    void AddLessThenCheck(const Attr::Attribute& attr);
    /// add a greater-equals check
    void AddGreaterOrEqualCheck(const Attr::Attribute& attr);
    /// add a lesser-equals check
    void AddLessOrEqualCheck(const Attr::Attribute& attr);
    /// add a boolean AND
    void AddAnd();
    /// add a boolean OR
    void AddOr();
    /// add a boolean NOT
    void AddNot();
    /// compile into an SQL WHERE statement
    virtual Util::String AsSqlWhere() const;
    /// bind filter attribute values to command
    virtual void BindValuesToCommand(const Ptr<Command>& cmd, IndexT wildcardStartIndex);

protected:
    class Token
    {
    public:
        /// token types
        enum Type
        {
            Equal,
            NotEqual,
            Greater,
            Less,
            GreaterEqual,
            LessEqual,
            And,
            Or,
            Not,
            Block,
            Root,
        };
        /// default constructor
        Token();
        /// constructor with type only (for And/Or)
        Token(Type t);
        /// constructor with type and attribute
        Token(Type t, const Attr::Attribute& attr);
        /// get type
        Type GetType() const;
        /// get attribute
        const Attr::Attribute& GetAttr() const;

    private:
        Type type;
        Attr::Attribute attr;
    };
    
    Util::SimpleTree<Token> tokens;
    Util::SimpleTree<Token>::Node* curToken;
    Util::Array<Attr::Attribute> bindAttrs; 
    bool isDirty;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FilterSet::ClearDirtyFlag()
{
    this->isDirty = false;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
FilterSet::IsDirty() const
{
    return this->isDirty;
}

//------------------------------------------------------------------------------
/**
*/
inline FilterSet::Token::Token() :
    type(Root)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline FilterSet::Token::Token(Type t) :
    type(t)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline FilterSet::Token::Token(Type t, const Attr::Attribute& a) :
    type(t),
    attr(a)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline bool
FilterSet::IsEmpty() const
{
    return (this->tokens.Root().Size() == 0);
}

//------------------------------------------------------------------------------
/**
*/
inline FilterSet::Token::Type
FilterSet::Token::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline const Attr::Attribute&
FilterSet::Token::GetAttr() const
{
    return this->attr;
}

} // namespace Db
//------------------------------------------------------------------------------
#endif
