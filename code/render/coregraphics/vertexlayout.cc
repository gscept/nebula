//------------------------------------------------------------------------------
//  vertexlayout.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "vertexlayout.h"
#include "vertexsignaturecache.h"
namespace CoreGraphics
{
VertexSignatureCache* layoutPool = nullptr;

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutId
CreateVertexLayout(const VertexLayoutCreateInfo& info)
{
    n_assert(info.comps.Size() > 0);
    VertexLayoutInfo loadInfo;

    Util::String sig;
    IndexT i;
    SizeT size = 0;
    for (i = 0; i < info.comps.Size(); i++)
    {
        sig.Append(info.comps[i].GetSignature());
        info.comps[i].byteOffset += size;
        size += info.comps[i].GetByteSize();
    }
    sig = Util::String::Sprintf("%s", sig.AsCharPtr());
    Util::StringAtom atom(sig);
    
    loadInfo.signature = Util::StringAtom(sig);
    loadInfo.vertexByteSize = size;
    loadInfo.shader = Ids::InvalidId64;
    loadInfo.comps = info.comps;

    // reserve resource using signature as name, don't load again unless needed
    VertexLayoutId id = layoutPool->ReserveResource(atom, "render_system");
    if (layoutPool->GetState(id) == Resources::Resource::Pending)
        layoutPool->LoadFromMemory(id, &loadInfo);

    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyVertexLayout(const VertexLayoutId id)
{
    // FIXME: shutdownrace
    if(layoutPool->GetRefCount())
    layoutPool->Unload(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VertexLayoutGetSize(const VertexLayoutId id)
{
    return layoutPool->GetVertexLayoutSize(id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<VertexComponent>&
VertexLayoutGetComponents(const VertexLayoutId id)
{
    return layoutPool->GetVertexComponents(id);
}

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutId
CreateCachedVertexLayout(const VertexLayoutId id, const ShaderProgramId shader)
{
    return VertexLayoutId();
}

} // CoreGraphics
