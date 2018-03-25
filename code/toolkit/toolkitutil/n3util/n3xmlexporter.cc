//------------------------------------------------------------------------------
//  n3xmlconverter.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "n3xmlexporter.h"
#include "io/ioserver.h"
#include "util/fourcc.h"
#include "io/memorystream.h"

using namespace IO;
using namespace Util;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::N3XmlExporter, 'N3XC', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
N3XmlExporter::N3XmlExporter()
{
	/// empty
}

//------------------------------------------------------------------------------
/**
*/
N3XmlExporter::~N3XmlExporter()
{
	/// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExporter::Open()
{
	this->modelReader = XmlReader::Create();
	this->modelWriter = BinaryModelWriter::Create();
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExporter::Close()
{
	this->modelReader = nullptr;
	this->modelWriter = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool 
N3XmlExporter::RecursiveParse( Ptr<IO::XmlReader> reader, Ptr<BinaryModelWriter> writer)
{

	Util::String nodeName = reader->GetCurrentNodeName();
	if (nodeName == "Nebula3Model")
	{
		// skip to next node
	}
	else if (nodeName == "Model")
	{
		this->modelWriter->BeginModel("", FourCC('MODL'), reader->GetString("name"));
	}
	else if (nodeName == "ModelNode")
	{
		Util::String className = reader->GetString("className");
		Util::String nodeName = reader->GetString("name");
		FourCC classFourCC;
		if (className == "TransformNode")
		{
			classFourCC = 'TRFN';
		}
		else if (className == "ShapeNode")
		{
			classFourCC = 'SPND';
		}
		else if (className == "MaterialShapeNode")
		{
			classFourCC = 'MSND';
		}
		else if (className == "CharacterSkinNode")
		{
			classFourCC = 'CHSN';
		}
		else if (className == "CharacterMaterialSkinNode")
		{
			classFourCC = 'CMSN';
		}
		else if (className == "CharacterNode")
		{
			classFourCC = 'CHRN';
		}
		else if (className == "ParticleSystemNode")
		{
			classFourCC = 'PSND';
		}
		else if (className == "ParticleSystemMaterialNode")
		{
			classFourCC = 'PSMD';
		}
		writer->BeginModelNode("",classFourCC, nodeName);
	}
	/// tags
	else
	{
		Util::String varType = nodeName;
		if (varType == "s")
		{
			writer->WriteString(reader->GetContent());
		}
		else if (varType == "f4")
		{
			Math::float4 vector = reader->GetContent().AsFloat4();
			writer->WriteFloat4(vector);
		}
		else if (varType == "i")
		{
			int value = reader->GetContent().AsInt();
			writer->WriteInt(value);
		}
		else if (varType == "f")
		{
			float value = reader->GetContent().AsFloat();
			writer->WriteFloat(value);
		}
		else if (varType == "b")
		{
			bool value = reader->GetContent().AsBool();
			writer->WriteBool(value);
		}
		else
		{
			writer->BeginTag("", FourCC::FromString(nodeName));
		}
	}

	if (reader->SetToFirstChild()) do 
	{
		if (!this->RecursiveParse(reader, writer))
		{
			return false;
		}
	} 
	while (reader->SetToNextChild());

	// close appropriate node
	if (nodeName == "Model")
	{
		writer->EndModel();
	}
	else if (nodeName == "ModelNode")
	{
		writer->EndModelNode();
	}
	else
	{
		writer->EndTag();
	}

	// recursion done
	return true;
}


//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExporter::ExportFile( const IO::URI& file )
{
	n_assert(file.IsValid());
	String fileName = file.LocalPath().ExtractFileName();
	fileName.StripFileExtension();
	String category = file.LocalPath().ExtractLastDirName();
	IoServer* ioServer = IoServer::Instance();

	n_printf("-------------------Exporting: %s-------------------\n", (file.GetHostAndLocalPath().AsCharPtr()));
	this->Progress(1, "Exporting model: " + file.GetHostAndLocalPath());

	String path = "mdl:" + category;
	if (!ioServer->CreateDirectory(path))
	{
		n_error("Could not create directory '%s'!", path.AsCharPtr());
	}

	Ptr<Stream> sourceStream = IoServer::Instance()->CreateStream(file);
	Ptr<Stream> destStream = IoServer::Instance()->CreateStream(IO::URI(path + "/" + fileName + ".n3"));

	sourceStream->SetAccessMode(Stream::ReadAccess);
	destStream->SetAccessMode(Stream::WriteAccess);

	sourceStream->Open();
	destStream->Open();

	this->modelReader->SetStream(sourceStream);
	this->modelReader->Open();

	this->modelWriter->SetStream(destStream);
	this->modelWriter->Open();

	this->modelReader->SetToRoot();
	this->modelReader->SetToFirstChild();
	this->RecursiveParse(this->modelReader, this->modelWriter);

	this->modelReader->Close();
	this->modelWriter->Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExporter::ExportDir( const Util::String& category )
{
	String categoryDir = "proj:work/assets" + category;
	Array<String> files = IoServer::Instance()->ListFiles(categoryDir, "*.xml");
	for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
	{
		this->ExportFile(categoryDir + "/" + files[fileIndex]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExporter::ExportAll()
{
	String workDir = "proj:work/assets";
	Array<String> directories = IoServer::Instance()->ListDirectories(workDir, "*");
	for (int directoryIndex = 0; directoryIndex < directories.Size(); directoryIndex++)
	{
		String category = workDir + "/" + directories[directoryIndex];
		Array<String> files = IoServer::Instance()->ListFiles(category, "*.xml");
		for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
		{
			this->ExportFile(category + "/" + files[fileIndex]);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExporter::ExportMemory( const Util::String& file, const IO::URI& dest )
{
	Ptr<MemoryStream> sourceStream = MemoryStream::Create();
	sourceStream->SetAccessMode(Stream::ReadWriteAccess);
	sourceStream->Open();
	sourceStream->Write((void*)file.AsCharPtr(), file.Length());
	sourceStream->Seek(0, Stream::Begin);

	Ptr<Stream> destStream = IoServer::Instance()->CreateStream(dest);
	destStream->SetAccessMode(Stream::WriteAccess);
	destStream->Open();

	this->modelReader->SetStream(sourceStream.upcast<Stream>());
	this->modelReader->Open();

	this->modelWriter->SetStream(destStream);
	this->modelWriter->Open();

	this->modelReader->SetToRoot();
	this->modelReader->SetToFirstChild();
	this->RecursiveParse(this->modelReader, this->modelWriter);

	this->modelReader->Close();
	this->modelWriter->Close();
}

} // namespace ToolkitUtil