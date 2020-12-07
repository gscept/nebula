#ifndef DB2_SQLITE3FILTERSET_H
#define DB2_SQLITE3FILTERSET_H
//------------------------------------------------------------------------------
/**
    @class Db2::Sqlite3FilterSet
  
    SQLite3 implement of Db2::FilterSet.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "db/filterset.h"

//------------------------------------------------------------------------------
namespace Db
{
class Sqlite3FilterSet : public FilterSet
{
    __DeclareClass(Sqlite3FilterSet);
public:
    /// compile into an SQL WHERE statement
    virtual Util::String AsSqlWhere() const;
    /// bind filter attribute values to command
    virtual void BindValuesToCommand(const Ptr<Command>& cmd, IndexT wildcardStartIndex);

private:
    /// recursing build method for AsSqlWhere()
    void RecurseBuildWhere(const Util::SimpleTree<Token>::Node* curNode, Util::String& inOutStr) const;

    // static string fragments for string construction (prevents excessive string object construction)
    static const Util::String OpenBracketFrag;
    static const Util::String CloseBracketFrag;
    static const Util::String AndFrag;
    static const Util::String OrFrag;
    static const Util::String NotFrag;
    static const Util::String DotFrag;
    static const Util::String EqualFrag;
    static const Util::String NotEqualFrag;
    static const Util::String GreaterFrag;
    static const Util::String LessFrag;
    static const Util::String GreaterEqualFrag;
    static const Util::String LessEqualFrag;
    static const Util::String WildcardFrag;
    static const Util::String TickFrag;
};

} // namespace Db
//------------------------------------------------------------------------------
#endif
