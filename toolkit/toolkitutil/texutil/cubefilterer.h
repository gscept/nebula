#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::CubeFilterer
    
    Takes an input CoreGraphics::Texture and filters it if it's a cubemap.
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/texture.h"
//#include "CubeMapGen/CCubeMapProcessor.h"
#include "io/uri.h"
namespace ToolkitUtil
{

class CubeFilterer : public Core::RefCounted
{
    __DeclareClass(CubeFilterer);
public:
    /// constructor
    CubeFilterer();
    /// destructor
    virtual ~CubeFilterer();

    /// set cube faces to be used for the pass
    void SetCubeFaces(const Util::FixedArray<Ptr<CoreGraphics::Texture>>& cubefaces);
    /// set the output texture name
    void SetOutputFile(const IO::URI& output);
    /// set the output size (squared size)
    void SetOutputSize(uint size);

    /// set specular power
    void SetSpecularPower(uint power);
    /// set if we should generate mips
    void SetGenerateMips(bool b);

    /// generates sampled cube map and saves into output, the third argument is a function pointer to a function which handles progress
    void Filter(bool irradiance, void* messageHandler, void(*CubeFilterer_Progress)(const Util::String&, void*));
    /// generate cube map for depth
    void DepthCube(bool genConeMap, void* messageHandler, void(*CubeFilterer_Progress)(const Util::String&, void*));

private:
    Util::FixedArray<Ptr<CoreGraphics::Texture>> cubefaces;
    IO::URI output;
    bool generateMips;
    uint power;
    uint size;

    //CCubeMapProcessor processor;
};

//------------------------------------------------------------------------------
/**
*/
inline void
CubeFilterer::SetCubeFaces(const Util::FixedArray<Ptr<CoreGraphics::Texture>>& cubefaces)
{
    this->cubefaces = cubefaces;
}

//------------------------------------------------------------------------------
/**
*/
inline void
CubeFilterer::SetOutputFile(const IO::URI& output)
{
    this->output = output;
}

//------------------------------------------------------------------------------
/**
*/
inline void
CubeFilterer::SetOutputSize(uint size)
{
    this->size = size;
}

//------------------------------------------------------------------------------
/**
*/
inline void
CubeFilterer::SetSpecularPower(uint power)
{
    this->power = power;
}

//------------------------------------------------------------------------------
/**
*/
inline void
CubeFilterer::SetGenerateMips(bool b)
{
    this->generateMips = b;
}

} // namespace ToolkitUtil