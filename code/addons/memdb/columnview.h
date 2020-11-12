#pragma once
//------------------------------------------------------------------------------
/**
    ColumnView

    Persistant array/buffer view for columns within a table.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "table.h"

namespace MemDb
{

template<typename TYPE>
class ColumnView
{
public:
    ColumnView() : data(nullptr), numRows(nullptr)
    {
        // empty
    }
    ColumnView(ColumnIndex const columnId, void** ptrptr, uint32_t const* const numRows) :
        data(ptrptr),
        cid(columnId),
        numRows(numRows)
    {
        // empty
    }

    ~ColumnView() = default;
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
    ColumnIndex cid;
    uint32_t const* numRows;
};

} // namespace MemDb
