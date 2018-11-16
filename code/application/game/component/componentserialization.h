#pragma once
//------------------------------------------------------------------------------
/**
	Component serialization functions

	Implements various serialization functions for different types of arrays.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/arrayallocator.h"
#include "io/memorystream.h"
#include "io/binarywriter.h"
#include "io/binaryreader.h"

namespace Game
{
//------------------------------------------------------------------------------
/**
*/
template<class X>
__forceinline typename std::enable_if<std::is_trivial<X>::value == false, void>::type
Serialize(const Ptr<IO::BinaryWriter>& writer, const Util::Array<X>& data)
{
	static_assert(false, "Type is not trivial and does have a serialize template specialization!");
}

//------------------------------------------------------------------------------
/**
*/
template<class X>
__forceinline typename std::enable_if<std::is_trivial<X>::value == false, void>::type
Deserialize(const Ptr<IO::BinaryReader>& reader, Util::Array<X>& data, uint32_t offset, uint32_t numInstances)
{
	static_assert(false, "Type is not trivial and does have a Deserialize template specialization!");
}

//------------------------------------------------------------------------------
/**
*/
template<class X>
__forceinline typename std::enable_if<std::is_trivial<X>::value == true, void>::type
Serialize(const Ptr<IO::BinaryWriter>& writer, const Util::Array<X>& data)
{
	writer->WriteRawData((void*)&data[0], data.ByteSize());
}

//------------------------------------------------------------------------------
/**
*/
template<class X>
__forceinline typename std::enable_if<std::is_trivial<X>::value == true, void>::type
Deserialize(const Ptr<IO::BinaryReader>& reader, Util::Array<X>& data, uint32_t offset, uint32_t numInstances)
{
	data.SetSize(offset + numInstances);
	reader->ReadRawData((void*)&data[offset], numInstances * data.TypeSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Serialize<Game::Entity>(const Ptr<IO::BinaryWriter>& writer, const Util::Array<Game::Entity>& data)
{
	writer->WriteRawData((void*)&data[0], data.ByteSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Deserialize<Game::Entity>(const Ptr<IO::BinaryReader>& reader, Util::Array<Game::Entity>& data, uint32_t offset, uint32_t numInstances)
{
	data.SetSize(offset + numInstances);
	reader->ReadRawData((void*)&data[offset], numInstances * data.TypeSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Serialize<Math::matrix44>(const Ptr<IO::BinaryWriter>& writer, const Util::Array<Math::matrix44>& data)
{
	writer->WriteRawData((void*)&data[0], data.ByteSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Deserialize<Math::matrix44>(const Ptr<IO::BinaryReader>& reader, Util::Array<Math::matrix44>& data, uint32_t offset, uint32_t numInstances)
{
	data.SetSize(offset + numInstances);
	reader->ReadRawData((void*)&data[offset], numInstances * data.TypeSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Serialize<Math::float4>(const Ptr<IO::BinaryWriter>& writer, const Util::Array<Math::float4>& data)
{
	writer->WriteRawData((void*)&data[0], data.ByteSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Deserialize<Math::float4>(const Ptr<IO::BinaryReader>& reader, Util::Array<Math::float4>& data, uint32_t offset, uint32_t numInstances)
{
	data.SetSize(offset + numInstances);
	reader->ReadRawData((void*)&data[offset], numInstances * data.TypeSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Serialize<Util::String>(const Ptr<IO::BinaryWriter>& writer, const Util::Array<Util::String>& data)
{
	// Write each string
	for (SizeT i = 0; i < data.Size(); ++i)
	{
		writer->WriteString(data[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Deserialize<Util::String>(const Ptr<IO::BinaryReader>& reader, Util::Array<Util::String>& data, uint32_t offset, uint32_t numInstances)
{
	data.SetSize(offset + numInstances);
	// read each string
	for (SizeT i = 0; i < numInstances; ++i)
	{
		data[offset + i] = reader->ReadString();
	}
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Serialize<Util::Guid>(const Ptr<IO::BinaryWriter>& writer, const Util::Array<Util::Guid>& data)
{
	for (SizeT i = 0; i < data.Size(); ++i)
	{
		writer->WriteGuid(data[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Deserialize<Util::Guid>(const Ptr<IO::BinaryReader>& reader, Util::Array<Util::Guid>& data, uint32_t offset, uint32_t numInstances)
{
	data.SetSize(offset + numInstances);
	for (SizeT i = 0; i < numInstances; ++i)
	{
		data[offset + i] = reader->ReadGuid();
	}
}

template <class...Ts, std::size_t...Is>
void WriteDataSequenced(const Util::ArrayAllocator<Game::Entity, Ts...>& data, const Ptr<IO::BinaryWriter>& writer, std::index_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0, (
		Serialize<Ts>(writer, data.GetArray<Is + 1>()), 0)...
	};
}

template <class...Ts, std::size_t...Is>
void ReadDataSequenced(Util::ArrayAllocator<Game::Entity, Ts...>& data, const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances, std::index_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0, (
		data.GetArray<Is + 1>().SetSize(offset + numInstances),
		Deserialize<Ts>(reader, data.GetArray<Is + 1>(), offset, numInstances), 0)...
	};
}

} // namespace Game