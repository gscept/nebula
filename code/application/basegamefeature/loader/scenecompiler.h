#pragma once
//------------------------------------------------------------------------------
/**
	SceneCompiler

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/fourcc.h"
#include "util/blob.h"

namespace BaseGameFeature
{

class SceneComponent
{
public:
	SceneComponent();
	~SceneComponent();

    Util::FourCC fourcc;
    uint numInstances;
    Util::Array<Util::Blob> data;
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
    Util::Array<SceneComponent> components;
};

} // namespace BaseGameFeature