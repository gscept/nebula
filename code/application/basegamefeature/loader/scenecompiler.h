#pragma once
//------------------------------------------------------------------------------
/**
	SceneCompiler

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/fourcc.h"
#include "util/blob.h"
#include "io/memorystream.h"

namespace BaseGameFeature
{


//------------------------------------------------------------------------------
/**
	@class	ComponentBuildData

	Contains build data for components.

	mStream contains the raw data that will be written to the buffer.
	Note that the component instance owner needs to be correctly indexed.
*/
class ComponentBuildData
{
public:
	ComponentBuildData();
	~ComponentBuildData();

	void InitializeStream();

    Util::FourCC fourcc;
    uint numInstances;
	Ptr<IO::MemoryStream> mStream;
};

//------------------------------------------------------------------------------
/**
	@class	SceneCompiler
	
	Contains build data for an entire scene.
*/
class SceneCompiler
{
public:
    SceneCompiler();
    ~SceneCompiler();

    bool Compile(Util::String filename);
	bool Decompile(Util::String filename);

    uint numEntities;
    uint numComponents;
	Util::Array<uint> parentIndices;
    Util::Array<ComponentBuildData> components;
};

} // namespace BaseGameFeature