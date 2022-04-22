#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::TextureAttrTable
    
    Reads the batch-convert texture attribute table and offers a 
    friendly C++ interface to it.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "toolkitutil/texutil/textureattrs.h"
#include "util/array.h"
#include "util/dictionary.h"
#include "util/stringatom.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class TextureAttrTable
{
public:
    /// constructor
    TextureAttrTable();
    /// destructor
    ~TextureAttrTable();

    /// setup content (parses proj:batchattributes.xml)
    bool Setup(const Util::String& path);
    /// saves content
    bool Save(const Util::String& path);
    /// discard content (clears attrs table)
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;

    /// check if a texture entry exists, texName must be "category/texture.ext"
    bool HasEntry(const Util::String& texName) const;
    /// get the tex attrs for texture, return default attrs if no entry found
    const TextureAttrs& GetEntry(const Util::String& texName) const;
    /// sets the tex attrs for the texture, replaces if already exists
    void SetEntry(const Util::String& texName, const TextureAttrs& attrs);
    /// specifically get the default texture attributes entry
    const TextureAttrs& GetDefaultEntry() const;

private:
    Util::Array<TextureAttrs> texAttrs;
    Util::Dictionary<Util::StringAtom, IndexT> indexMap;
    TextureAttrs defaultAttrs;
    bool valid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
TextureAttrTable::IsValid() const
{
    return this->valid;
}

} // namespace TextureAttrTable
//------------------------------------------------------------------------------
    
    
    