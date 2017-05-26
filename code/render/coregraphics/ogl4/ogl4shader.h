#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4Shader
    
    OGL4 implementation of a shader.

    @todo lost/reset device handling
    
    (C) 2013 Gustav Sterbrant
*/
#include "util/blob.h"
#include "coregraphics/base/shaderbase.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4Shader : public Base::ShaderBase
{
    __DeclareClass(OGL4Shader);
public:
	
    /// constructor
    OGL4Shader();
    /// destructor
    virtual ~OGL4Shader();
   
    /// unload the resource, or cancel the pending load
    virtual void Unload();
	/// returns effect
	AnyFX::Effect* GetOGL4Effect() const;

    /// begin updating shader state
    void BeginUpdate();
    /// end updating shader state
    void EndUpdate();

	/// reloads a shader from file
	void Reload();

private:
    friend class OGL4StreamShaderLoader;

	/// cleans up the shader
	void Cleanup();

    /// called by ogl4 shader server when ogl4 device is lost
    void OnLostDevice();
    /// called by ogl4 shader server when ogl4 device is reset
    void OnResetDevice();

	AnyFX::Effect* ogl4Effect;
    Ptr<CoreGraphics::ConstantBuffer> globalBlockBuffer;
	Ptr<CoreGraphics::ShaderVariable> globalBlockBufferVar;
};

//------------------------------------------------------------------------------
/**
*/
inline AnyFX::Effect* 
OGL4Shader::GetOGL4Effect() const
{
	return this->ogl4Effect;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------



    