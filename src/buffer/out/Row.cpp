/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "Row.hpp"
#include "Ucs2CharRow.hpp"

// Routine Description:
// - swaps two ROWs
// Arguments:
// - a - the first ROW to swap
// - b - the second ROW to swap
// Return Value:
// - <none>
void swap(ROW& a, ROW& b) noexcept
{
    a.swap(b);
}

// Routine Description:
// - constructor
// Arguments:
// - rowId - the row index in the text buffer
// - rowWidth - the width of the row, cell elements
// - fillAttribute - the default text attribute
// Return Value:
// - constructed object
ROW::ROW(const SHORT rowId, const short rowWidth, const TextAttribute fillAttribute) :
    _id{ rowId },
    _rowWidth{ gsl::narrow<size_t>(rowWidth) },
    _charRow{ std::make_unique<Ucs2CharRow>(rowWidth) },
    _attrRow{ rowWidth, fillAttribute }
{
}

// Routine Description:
// - copy constructor
// Arguments:
// - a - the object to copy
// Return Value:
// - the copied object
ROW::ROW(const ROW& a) :
    _attrRow{ a._attrRow },
    _rowWidth{ a._rowWidth },
    _id{ a._id }
{
    // we only support ucs2 encoded char rows
    FAIL_FAST_IF_MSG(a._charRow->GetSupportedEncoding() != ICharRow::SupportedEncoding::Ucs2,
                     "only support UCS2 char rows currently");

    Ucs2CharRow charRow = *static_cast<const Ucs2CharRow* const>(a._charRow.get());
    _charRow = std::make_unique<Ucs2CharRow>(charRow);
}

// Routine Description:
// - assignment operator overload
// Arguments:
// - a - the object to copy to this one
// Return Value:
// - a reference to this object
ROW& ROW::operator=(const ROW& a)
{
    ROW temp{ a };
    this->swap(temp);
    return *this;
}

// Routine Description:
// - move constructor
// Arguments:
// - a - the object to move
// Return Value:
// - the constructed object
ROW::ROW(ROW&& a) noexcept :
    _charRow{ std::move(a._charRow) },
    _attrRow{ std::move(a._attrRow) },
    _id{ std::move(a._id) },
    _rowWidth{ _rowWidth }
{
}

// Routine Description:
// - swaps fields with another ROW
// Arguments:
// - other - the object to swap with
// Return Value:
// - <none>
void ROW::swap(ROW& other) noexcept
{
    using std::swap;
    swap(_charRow, other._charRow);
    swap(_attrRow, other._attrRow);
    swap(_id, other._id);
    swap(_rowWidth, other._rowWidth);
}
size_t ROW::size() const noexcept
{
    return _rowWidth;
}

const ICharRow& ROW::GetCharRow() const
{
    return *_charRow;
}

ICharRow& ROW::GetCharRow()
{
    return const_cast<ICharRow&>(static_cast<const ROW* const>(this)->GetCharRow());
}

const ATTR_ROW& ROW::GetAttrRow() const noexcept
{
    return _attrRow;
}

ATTR_ROW& ROW::GetAttrRow() noexcept
{
    return const_cast<ATTR_ROW&>(static_cast<const ROW* const>(this)->GetAttrRow());
}

SHORT ROW::GetId() const noexcept
{
    return _id;
}

void ROW::SetId(const SHORT id) noexcept
{
    _id = id;
}

// Routine Description:
// - Sets all properties of the ROW to default values
// Arguments:
// - Attr - The default attribute (color) to fill
// Return Value:
// - <none>
bool ROW::Reset(const TextAttribute Attr)
{
    _charRow->Reset();
    try
    {
        _attrRow.Reset(Attr);
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
        return false;
    }
    return true;
}

// Routine Description:
// - resizes ROW to new width
// Arguments:
// - width - the new width, in cells
// Return Value:
// - S_OK if successful, otherwise relevant error
[[nodiscard]]
HRESULT ROW::Resize(const size_t width)
{
    const auto oldWidth = _charRow->size();
    RETURN_IF_FAILED(_charRow->Resize(width));
    try
    {
        _attrRow.Resize(oldWidth, width);
    }
    CATCH_RETURN();
    return S_OK;
}

// Routine Description:
// - clears char data in column in row
// Arguments:
// - column - 0-indexed column index
// Return Value:
// - <none>
void ROW::ClearColumn(const size_t column)
{
    THROW_HR_IF(E_INVALIDARG, column >= _charRow->size());
    _charRow->ClearCell(column);
}

std::wstring ROW::GetText() const
{
    return _charRow->GetText();
}

std::vector<OutputCell> ROW::AsCells() const
{
    return AsCells(0, size());
}

std::vector<OutputCell> ROW::AsCells(const size_t startIndex) const
{
    return AsCells(startIndex, size() - startIndex);
}

std::vector<OutputCell> ROW::AsCells(const size_t startIndex, const size_t count) const
{
    std::vector<OutputCell> cells;
    cells.reserve(size());

    // Unpack the attributes into an array so we can iterate over them.
    const auto unpackedAttrs = _attrRow.UnpackAttrs();

    const Ucs2CharRow* const charRow = static_cast<const Ucs2CharRow* const>(_charRow.get());
    for (size_t i = 0; i < count; ++i)
    {
        const auto index = startIndex + i;
        cells.emplace_back(charRow->GetGlyphAt(index), charRow->GetAttribute(index), unpackedAttrs[index]);
    }
    return cells;
}

const OutputCell ROW::at(const size_t column) const
{
    const Ucs2CharRow* const charRow = static_cast<const Ucs2CharRow* const>(_charRow.get());
    return { charRow->GetGlyphAt(column), charRow->GetAttribute(column), _attrRow.GetAttrByColumn(column) };
}