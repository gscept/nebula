#pragma once
//------------------------------------------------------------------------------
/**
    ColumnData

    Persistant "array" accessor for columns within a table.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "table.h"

namespace Game
{

namespace Db
{

template<typename TYPE>
class ColumnData
{
public:
    ColumnData() : data(nullptr), numRows(nullptr)
    {
        // empty
    }
    ColumnData(ColumnId const columnId, void** ptrptr, uint32_t const* const numRows, bool state = false) :
        data(ptrptr),
        cid(columnId),
        isState(state),
        numRows(numRows)
    {
        // empty
    }

    ~ColumnData() = default;
    TYPE& operator[](IndexT index)
    {
        n_assert(this->data != nullptr);
        n_assert(*this->data != nullptr);
#ifdef NEBULA_BOUNDSCHECKS
        n_assert(index >= 0 && index < *this->numRows);
#endif
        void* dataptr = *this->data;
        TYPE* ptr = reinterpret_cast<TYPE*>(dataptr);
        return (ptr[index]);
    }

    TYPE const& operator[](IndexT index) const
    {
        n_assert(this->data != nullptr);
        n_assert(*this->data != nullptr);
#ifdef NEBULA_BOUNDSCHECKS
        n_assert(index >= 0 && index < *this->numRows);
#endif
        void* dataptr = *this->data;
        TYPE* ptr = reinterpret_cast<TYPE*>(dataptr);
        return (ptr[index]);
    }
private:
    void** data;
    ColumnId cid;
    uint32_t const* numRows;
    bool isState = false;
};

} // namespace Db

} // namespace Game
