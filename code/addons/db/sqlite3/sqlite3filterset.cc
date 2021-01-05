//------------------------------------------------------------------------------
//  sqlite3filterset.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "db/sqlite3/sqlite3filterset.h"
#include "db/command.h"

namespace Db
{
__ImplementClass(Db::Sqlite3FilterSet, 'SQFS', Db::FilterSet);

using namespace Util;

const Util::String Sqlite3FilterSet::OpenBracketFrag(" ( ");
const Util::String Sqlite3FilterSet::CloseBracketFrag(" ) ");
const Util::String Sqlite3FilterSet::AndFrag(" AND ");
const Util::String Sqlite3FilterSet::OrFrag(" OR ");
const Util::String Sqlite3FilterSet::NotFrag(" NOT ");
const Util::String Sqlite3FilterSet::DotFrag(".");
const Util::String Sqlite3FilterSet::EqualFrag("==");
const Util::String Sqlite3FilterSet::NotEqualFrag("!=");
const Util::String Sqlite3FilterSet::GreaterFrag(">");
const Util::String Sqlite3FilterSet::LessFrag("<");
const Util::String Sqlite3FilterSet::GreaterEqualFrag(">=");
const Util::String Sqlite3FilterSet::LessEqualFrag("<=");
const Util::String Sqlite3FilterSet::WildcardFrag("?");
const Util::String Sqlite3FilterSet::TickFrag("'");

//------------------------------------------------------------------------------
/**
    This compiles the filter tree into an SQL WHERE statement.
*/
String
Sqlite3FilterSet::AsSqlWhere() const
{
    n_assert(!this->IsEmpty());   
    String str;
    str.Reserve(4096);
    this->RecurseBuildWhere(&(this->tokens.Root()), str);
    return str;
}

//------------------------------------------------------------------------------
/**
    This method is recursively called to build an SQL WHERE statement
    from the current filter tree.
*/
void
Sqlite3FilterSet::RecurseBuildWhere(const SimpleTree<Token>::Node* curNode, String& inOutStr) const
{
    n_assert(0 != curNode);
    const Token& token = curNode->Value();
    Token::Type type = token.GetType();
    if (Token::Root == type)
    {
        // just recurse without brackets
        IndexT i;
        for (i = 0; i < curNode->Size(); i++)
        {
            this->RecurseBuildWhere(&curNode->Child(i), inOutStr);
        }
    }
    else if (Token::Block == type)
    {
        // add a block of brackets and recurse
        inOutStr.Append(OpenBracketFrag);
        IndexT i;
        for (i = 0; i < curNode->Size(); i++)
        {
            this->RecurseBuildWhere(&curNode->Child(i), inOutStr);
        }
        inOutStr.Append(CloseBracketFrag);
    }
    else if (Token::And == type)
    {
        inOutStr.Append(AndFrag);
    }
    else if (Token::Or == type)
    {
        inOutStr.Append(OrFrag);
    }
    else if (Token::Not == type)
    {
        inOutStr.Append(NotFrag);
    }
    else
    {
        // build a compare fragment
        inOutStr.Append(token.GetAttr().GetName());
        switch (type)
        {
            case Token::Equal:          inOutStr.Append(EqualFrag); break;
            case Token::NotEqual:       inOutStr.Append(NotEqualFrag); break;
            case Token::Greater:        inOutStr.Append(GreaterFrag); break;
            case Token::Less:           inOutStr.Append(LessFrag); break;
            case Token::GreaterEqual:   inOutStr.Append(GreaterEqualFrag); break;
            case Token::LessEqual:      inOutStr.Append(LessEqualFrag); break;
            default:
                n_error("Filter::RecurseBuildWhere(): invalid token type!");
                break;
        }
        inOutStr.Append(WildcardFrag);
    }
}

//------------------------------------------------------------------------------
/**
    This methods binds the actual WHERE values to a compiled command.
*/
void
Sqlite3FilterSet::BindValuesToCommand(const Ptr<Command>& cmd, IndexT wildcardStartIndex)
{
    n_assert(0 != cmd);
    IndexT i = 0;
    SizeT num = this->bindAttrs.Size();
    for (i = 0; i < num; i++)
    {
        IndexT wildCardIndex = i + wildcardStartIndex;
        const Attr::Attribute& attr = this->bindAttrs[wildCardIndex];
        switch (attr.GetValueType())
        {
            case Attr::IntType:
                cmd->BindInt(wildCardIndex, attr.GetInt());
                break;
            case Attr::FloatType:
                cmd->BindFloat(wildCardIndex, attr.GetFloat());
                break;
            case Attr::BoolType:
                cmd->BindBool(wildCardIndex, attr.GetBool());
                break;            
            case Attr::Vec4Type:
                cmd->BindVec4(wildCardIndex, attr.GetVec4());
                break;
            case Attr::StringType:
                cmd->BindString(wildCardIndex, attr.GetString());
                break;
            case Attr::Mat4Type:
                cmd->BindMat4(wildCardIndex, attr.GetMat4());
                break;
            case Attr::BlobType:
                cmd->BindBlob(wildCardIndex, attr.GetBlob());
                break;
            case Attr::GuidType:
                cmd->BindGuid(wildCardIndex, attr.GetGuid());
                break;
            case Attr::VoidType:
                n_error("Sqlite3FilterSet::BindValuesToCommand() void type");
                break;
        }
    }
}

} // namespace Db
