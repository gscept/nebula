//------------------------------------------------------------------------------
//  scenecompiler.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenecompiler.h"
#include "io/binarywriter.h"
#include "io/binaryreader.h"
#include "io/filestream.h"

namespace BaseGameFeature
{


const static uint sceneMagic = 'SCN0';

//------------------------------------------------------------------------------
/**
*/
SceneComponent::SceneComponent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SceneComponent::~SceneComponent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SceneCompiler::SceneCompiler()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SceneCompiler::~SceneCompiler()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
SceneCompiler::Compile(Util::String filename)
{
	auto uri = IO::URI(filename);
	Ptr<IO::BinaryWriter> writer = IO::BinaryWriter::Create();
	Ptr<IO::FileStream> stream = IO::FileStream::Create();
	stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
	stream->SetURI(uri);
	stream->Open();
	writer->SetStream(stream);

	writer->WriteUInt(sceneMagic);
	writer->WriteUInt(this->numEntities);
	writer->WriteUInt(this->numComponents);
	writer->WriteUIntArray(this->parentIndices);

	for (auto component : this->components)
	{
		writer->WriteUInt(component.fourcc.AsUInt());
		writer->WriteUInt(component.numInstances);
		
		for (auto blob : component.data)
		{
			writer->WriteBlob(blob);
		}
	}

	stream->Close();
	
	return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
SceneCompiler::Decompile(Util::String filename)
{
	auto uri = IO::URI(filename);
	Ptr<IO::BinaryReader> reader = IO::BinaryReader::Create();
	Ptr<IO::FileStream> stream = IO::FileStream::Create();
	stream->SetAccessMode(IO::Stream::AccessMode::ReadAccess);
	stream->SetURI(uri);
	stream->Open();
	reader->SetStream(stream);
	uint filemagic = reader->ReadUInt();
	if (sceneMagic != filemagic)
	{
		n_assert2(sceneMagic != filemagic, "Incorrect magic number!")
		return false;
	}
	this->numEntities = reader->ReadUInt();
	this->numComponents = reader->ReadUInt();
	this->parentIndices = reader->ReadUIntArray();

	for (SizeT i = 0; i < this->numComponents; i++)
	{
		SceneComponent component;
		component.fourcc = Util::FourCC(reader->ReadUInt());
		component.numInstances = reader->ReadUInt();
		
		for (SizeT k = 0; k < component.numInstances; k++)
		{
			component.data.Append(reader->ReadBlob());
		}
		this->components.Append(component);
	}

	stream->Close();

	return true;
}
} // namespace BaseGameFeature
