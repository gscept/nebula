//------------------------------------------------------------------------------
//  @file dds.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/geometry.h"
#include "physics/formats/formats.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/path.h"

#if WITH_NEBULA_PHYSICS_FORMAT_DDS
#include "gliml.h"

namespace Physics
{

//------------------------------------------------------------------------------
/**
    The resource is a texture URN, folder:file:tex, which is read from the exported dds file.
*/
bool
DdsHeightField(Util::StringAtom resource, HeightFieldInput& out)
{
    const IO::Path path = IO::Path::Parse(resource.AsString());
    if (!path.IsFile() || path.GetType() != "tex")
    {
        n_warning("DdsHeightField: '%s' is not a texture URN (folder:file:tex)\n", resource.Value());
        return false;
    }

    Ptr<IO::Stream> stream = IO::CreateStream(path.ExportURI());
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        return false;
    }

    void* heightData = stream->MemoryMap();
    uint heightSize = stream->GetSize();
    n_assert(heightData != nullptr);

    bool parsed = false;

    gliml::context ctx;
    if (ctx.load_dds(heightData, heightSize))
    {
        int depth = ctx.image_depth(0, 0);
        int width = ctx.image_width(0, 0);
        int height = ctx.image_height(0, 0);

        // decode the format locally instead of going through CoreGraphics::Gliml
        if (depth != 1)
        {
            n_warning("DdsHeightField: '%s' is a volume texture, a heightmap must be 2D\n", resource.Value());
        }
        else if (ctx.image_format() == GLIML_FOURCC_DXT10 && ctx.image_internal_format() == gliml::DXGI_FORMAT_R16_UNORM)
        {
            const ushort* heightBuffer = (const ushort*)ctx.image_data(0, 0);

            // the image layout is the height field input layout, columns along x and rows along z
            out.width = (SizeT)width;
            out.height = (SizeT)height;
            out.samples.Resize(out.width * out.height);
            for (SizeT i = 0; i < out.samples.Size(); i++)
            {
                out.samples[i] = heightBuffer[i];
            }
            parsed = true;
        }
        else
        {
            // offline tools read heightmaps too, a texture exported with the wrong format must not take them down
            n_warning("DdsHeightField: '%s' is not an R16 heightmap\n", resource.Value());
        }
    }
    else
    {
        n_warning("DdsHeightField: '%s' is not a dds file\n", resource.Value());
    }

    stream->MemoryUnmap();
    stream->Close();
    return parsed;
}

} // namespace Physics
#endif
