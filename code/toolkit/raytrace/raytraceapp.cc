//------------------------------------------------------------------------------
// raytraceapp.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "raytraceapp.h"
#include "coregraphics\legacy\nvx2streamreader.h"

namespace Toolkit
{

//------------------------------------------------------------------------------
/**
*/
RaytraceApp::RaytraceApp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
RaytraceApp::~RaytraceApp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
RaytraceApp::Open()
{
	if (ToolkitApp::Open())
	{
		ToolkitApp::SetupProjectInfo();
		if (this->args.HasArg("-m"))
		{
			if (this->args.HasArg("-mesh"))
			{
				Util::String mesh = this->args.GetString("-mesh");
				this->LoadMesh(Util::String::Sprintf("dst:meshes/%s.nvx2", mesh.AsCharPtr()));
				return true;
			}
			else if (this->args.HasArg("-level"))
			{
				Util::String level = this->args.GetString("-level");
				return true;
			}
			else
			{
				this->ShowHelp();
			}
		}
		else
		{
			this->ShowHelp();
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytraceApp::Close()
{

	ToolkitApp::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
RaytraceApp::ShowHelp()
{
	n_printf("NebulaT Raytracer\n"
		"raytrace [-mesh | -level] <id> -m [lightmap | thickness]\n"
		"-mesh	<id>		Load mesh, id must be in the format <folder>/<file>, .nvx2 will be assumed.\n"
		"-level <id>		Load entire level.\n"
		"-m <name>		Select generation metod.\n"
		"	'lightmap' will generate per-object lightmaps and save them as an atlas for the level.\n"
		"	'thickness' will generate a per-vertex set of thickness encoded as spherical harmonic coefficients.\n"
		"-help			Show this help");
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ObjectContext>
RaytraceApp::LoadMesh(const IO::URI& mesh)
{
	Ptr<ObjectContext> object = nullptr;
	Ptr<Legacy::Nvx2StreamReader> reader = Legacy::Nvx2StreamReader::Create();
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(mesh);
	if (stream->Open())
	{
		// setup reader
		reader->SetStream(stream);
		reader->SetRawMode(true);
		reader->Open();

		// create context
		object = ObjectContext::Create();
		object->SetTransform(Math::matrix44::identity());
		object->SetMesh(reader->GetPrimitiveGroups(), reader->GetVertexComponents(), reader->GetVertexData(), reader->GetNumVertices(), reader->GetVertexWidth(), reader->GetIndexData(), reader->GetNumIndices());

		// close stream
		reader->Close();
	}
	else
	{
		n_printf("Invalid path %s\n", mesh.LocalPath().AsCharPtr());
	}
	return object;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytraceApp::LoadLevel(const IO::URI& level)
{
	n_printf(level.LocalPath().AsCharPtr());
}

} // namespace Toolkit