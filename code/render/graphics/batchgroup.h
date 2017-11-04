#pragma once
//------------------------------------------------------------------------------
/**
    @class Graphics::BatchGroup
  
    BatchGroup denotes a zero indexed name registry which corresponds to the 
    type of materials being batched during a FrameBatch. The name from the 
    'batchType' field gets converted into an index in this class, which is then
    used when retrieving all materials which utilizes this batch type.

    Materials have the same field in their template definition, meaning that 
    for each frame batch using a specific batch type, all materials with the same
    batch type defined will be rendered in this pass, which acts as the bridge
    between the frame shader system and the material rendering system.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/stringatom.h"
#include "util/array.h"
#include "util/dictionary.h"

//------------------------------------------------------------------------------
namespace Graphics
{
class BatchGroup
{
public:
    /// human readable name of a ModelNodeType
    typedef Util::StringAtom Name;
    /// binary code for a ModelNodeType
    typedef IndexT Code;

    /// convert from string
    static Code FromName(const Name& name);
    /// convert to string
    static Name ToName(Code c);
    /// maximum number of different ModelNodeTypes
    static const IndexT NumBatchGroups = 256;
    /// invalid model node type code
    static const IndexT InvalidBatchGroup = InvalidIndex;

private:
    friend class GraphicsServer;

    /// constructor
    BatchGroup();

    Util::Dictionary<Name, IndexT> nameToCode;
    Util::Array<Name> codeToName;
};

} // namespace Graphics
//------------------------------------------------------------------------------

