//------------------------------------------------------------------------------
//  filterset.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/filterset.h"

namespace Db
{
__ImplementClass(Db::FilterSet, 'DBFS', Core::RefCounted);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
FilterSet::FilterSet() :
    curToken(0),
    isDirty(false)
{
    this->curToken = &this->tokens.Root();
}

//------------------------------------------------------------------------------
/**
*/
FilterSet::~FilterSet()
{
    this->curToken = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FilterSet::Clear()
{
    this->tokens.Root().Clear();
    this->bindAttrs.Clear();
    this->curToken = &this->tokens.Root();
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This starts a new statement block (think of it as opening brackets)
    and makes it current token. All following statements will go
    below this block, until the method End() is called.
*/
void
FilterSet::BeginBlock()
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::Block));
    this->curToken = &(this->curToken->Back());
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This ends the current statement block. The method will make sure
    that there is no dangling And/Or token, and that the root
    level isn't reached yet.
*/
void
FilterSet::EndBlock()
{
    n_assert(0 != this->curToken);
    n_assert(this->curToken->HasParent());
    n_assert(this->curToken->Value().GetType() != Token::Root);
    n_assert(this->curToken->Front().Value().GetType() != Token::And);
    n_assert(this->curToken->Front().Value().GetType() != Token::Or);
    n_assert(this->curToken->Back().Value().GetType() != Token::And);
    n_assert(this->curToken->Back().Value().GetType() != Token::Or);
    n_assert(this->curToken->Back().Value().GetType() != Token::Not);
    this->curToken = &this->curToken->Parent();
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This adds an equal-check to the filter, where the attribute's id
    defines the column, and the attribute's value is the value to check
    the column's contents against. 
*/
void
FilterSet::AddEqualCheck(const Attr::Attribute& attr)
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::Equal, attr));
    this->bindAttrs.Append(attr);
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This adds an greater-check to the filter, where the attribute's id
    defines the column, and the attribute's value is the value to check
    the column's contents against.
*/
void
FilterSet::AddGreaterThenCheck(const Attr::Attribute& attr)
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::Greater, attr));
    this->bindAttrs.Append(attr);
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This adds a less-check to the filter, where the attribute's id
    defines the column, and the attribute's value is the value to check
    the column's contents against.
*/
void
FilterSet::AddLessThenCheck(const Attr::Attribute& attr)
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::Less, attr));
    this->bindAttrs.Append(attr);
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This adds a greater-equal-check to the filter, where the attribute's id
    defines the column, and the attribute's value is the value to check
    the column's contents against.
*/
void
FilterSet::AddGreaterOrEqualCheck(const Attr::Attribute& attr)
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::GreaterEqual, attr));
    this->bindAttrs.Append(attr);
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This adds a less-equal-check to the filter, where the attribute's id
    defines the column, and the attribute's value is the value to check
    the column's contents against.
*/
void
FilterSet::AddLessOrEqualCheck(const Attr::Attribute& attr)
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::LessEqual, attr));
    this->bindAttrs.Append(attr);
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This adds an AND boolean operation. This may not be the first or last
    token in a block (this is checked by the next End()).
*/
void
FilterSet::AddAnd()
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::And));
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This adds an OR boolean operation. This may not be the first or last
    token in a block (this is checked by the next End()).
*/
void
FilterSet::AddOr()
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::Or));
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This adds an NOT boolean operation. This may not be last
    token in a block (this is checked by the next End()).
*/
void
FilterSet::AddNot()
{
    n_assert(0 != this->curToken);
    this->curToken->Append(Token(Token::Not));
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
    This method should return a string fragment which contains the 
    part behind an SQL WHERE representing the condition tree encoded
    in this Filter object.
*/
String
FilterSet::AsSqlWhere() const
{
    n_error("Db::FilterSet::AsSqlWhere() called!");
    return "";
}

//------------------------------------------------------------------------------
/**
    This methods binds the actual WHERE values to a compiled command.
*/
void
FilterSet::BindValuesToCommand(const Ptr<Command>& cmd, IndexT wildcardStartIndex)
{
    n_error("Db::FilterSet::BindValuesToCommand() called!");
}

} // namespace Db
    