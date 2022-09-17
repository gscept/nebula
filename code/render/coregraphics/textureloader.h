#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamTextureCache
  
    Resource loader for loading texture data from a Nebula stream. Supports
    synchronous and asynchronous loading.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "resources/resourceloader.h"
#include "gliml.h"

namespace CoreGraphics
{
class TextureLoader : public Resources::ResourceLoader
{
    __DeclareClass(TextureLoader);
public:
    /// constructor
    TextureLoader();
    /// destructor
    virtual ~TextureLoader();

private:

    struct TextureStreamData
    {
        gliml::context ctx;
        void* mappedBuffer;
        uint mappedBufferSize;
        ubyte lowestLod;
        ubyte maxMip;
    };

    /// load texture
    Resources::ResourceUnknownId LoadFromStream(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// unload texture
    void Unload(const Resources::ResourceId id);

    /// stream mips
    void StreamMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate) override;
};

} // namespace CoreGraphics
