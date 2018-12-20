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
ComponentBuildData::ComponentBuildData()
{

}

//------------------------------------------------------------------------------
/**
*/
ComponentBuildData::~ComponentBuildData()
{

}

//------------------------------------------------------------------------------
/**
*/
void
ComponentBuildData::InitializeStream()
{
	this->mStream = IO::MemoryStream::Create();
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
		// TODO: Write description of component so that we can make sure we're not reading junk or outdated component data.

		// Write size of stream
		writer->WriteUInt(component.mStream->GetSize());
		// Fill with content
		writer->WriteRawData(component.mStream->GetRawPointer(), component.mStream->GetSize());
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
		ComponentBuildData component;
		component.fourcc = Util::FourCC(reader->ReadUInt());
		component.numInstances = reader->ReadUInt();
		// TODO: Read description of component so that we can make sure we're not reading junk or outdated component data.

		component.InitializeStream();
		uint size = reader->ReadUInt();

		component.mStream->SetSize(size);
		reader->ReadRawData(component.mStream->GetRawPointer(), size);

		this->components.Append(component);
	}

	stream->Close();

	return true;
}
} // namespace BaseGameFeature
