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