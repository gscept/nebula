//------------------------------------------------------------------------------
//  streamsurfacesaver.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "streamsurfacesaver.h"
#include "surface.h"
#include "io/xmlwriter.h"

namespace Materials
{
__ImplementClass(Materials::StreamSurfaceSaver, 'SSMS', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
StreamSurfaceSaver::StreamSurfaceSaver()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
StreamSurfaceSaver::~StreamSurfaceSaver()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamSurfaceSaver::OnSave()
{
	n_assert(this->stream.isvalid());
	const Ptr<Surface>& sur = this->resource.downcast<Surface>();

	Ptr<IO::XmlWriter> writer = IO::XmlWriter::Create();
	writer->SetStream(this->stream);
	if (writer->Open())
	{
		writer->BeginNode("NebulaT");
			writer->BeginNode("Surface");
				writer->SetString("template", sur->GetMaterialTemplate()->GetName().AsString());
				IndexT i;
				for (i = 0; i < sur->staticValues.Size(); i++)
				{
                    const Util::KeyValuePair<Util::StringAtom, Surface::SurfaceValueBinding>& pair = sur->staticValues.KeyValuePairAtIndex(i);

                    // don't save system variables
                    if (pair.Value().system) continue;

					writer->BeginNode("Param");
						writer->SetString("name", pair.Key().AsString());
						const Util::Variant& val = pair.Value().value;

						// assume its texture if variant is object
                        if (val.GetType() != Util::Variant::Object) writer->SetString("value", val.ToString());
						else
						{
							Ptr<CoreGraphics::Texture> tex = (CoreGraphics::Texture*)val.GetObject();
							Util::String resWithoutExt = tex->GetResourceId().AsString();
							resWithoutExt.StripFileExtension();
							writer->SetString("value", resWithoutExt);
						}
						
					writer->EndNode();
				}
			writer->EndNode();
		writer->EndNode();
	}
	else
	{
		n_error("Could not open '%s' for writing", stream->GetURI().LocalPath().AsCharPtr());
		return false;
	}
	return true;
}

} // namespace Materials