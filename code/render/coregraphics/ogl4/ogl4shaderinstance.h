#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4ShaderInstance
    
    Implements a single instance of a shader.
    This class is used to hold the state of an effect, such as
    shader variables and which shader bundle should be used.

    To apply the global state of the shader, you use the Begin/BeginPass functions
    which will trigger the currently active shader variation to be applied.

    To apply the local state of the shader instance, such as variables and constant
    buffers, use the Commit function.

    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "afxapi.h"
#include "coregraphics/base/shaderstatebase.h"
#include "coregraphics/base/shaderbase.h"
#include "coregraphics/shaderfeature.h"

namespace OpenGL4
{
class OGL4UniformBuffer;
class OGL4ShaderInstance : public Base::ShaderInstanceBase
{
    __DeclareClass(OGL4ShaderInstance);
public:
    /// constructor
    OGL4ShaderInstance();
    /// destructor
    virtual ~OGL4ShaderInstance();    
    /// select active variation by feature mask
    bool SelectActiveVariation(CoreGraphics::ShaderFeature::Mask featureMask);

	/// begin all uniform buffers for a synchronous update
	void BeginUpdateSync();
	/// end buffer updates for all uniform buffers
	void EndUpdateSync();
    /// commit changes before rendering
    void Commit();

    /// create variable instance
    Ptr<CoreGraphics::ShaderVariableInstance> CreateVariableInstance(const Base::ShaderVariableBase::Name& n);

    /// sets the shader in wireframe mode
    void SetWireframe(bool b);

    /// return handle to AnyFX shader
    AnyFX::Effect* GetAnyFXEffect();
	
protected:
    friend class Base::ShaderBase;
    friend class OGL4Shader;

	/// sets up shader variables from shader handler
	void SetupVariables(const GLuint& shader);
    /// setup the shader instance from its original shader object
    virtual void Setup(const Ptr<CoreGraphics::Shader>& origShader);
	/// reload the shader instance from original shader object
	virtual void Reload(const Ptr<CoreGraphics::Shader>& origShader);

	/// reload the variables from reflection
	void ReloadVariables(const GLuint& shader);

    /// cleanup the shader instance
    virtual void Cleanup();
    /// called by ogl4 shader server when ogl4 device is lost
    void OnLostDevice();
    /// called by ogl4 shader server when ogl4 device is reset
    void OnResetDevice();

private:
    bool inWireframe;
	AnyFX::Effect* effect;
    Util::Array<Ptr<CoreGraphics::ConstantBuffer>> uniformBuffers;
    Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ConstantBuffer>> uniformBuffersByName;

    struct DeferredVariableToBufferBind
    {
        unsigned offset;
        unsigned size;
        unsigned arraySize;
    };
    typedef Util::KeyValuePair<DeferredVariableToBufferBind, Ptr<CoreGraphics::ConstantBuffer>> VariableBufferBinding;
    Util::Dictionary<Util::StringAtom, VariableBufferBinding> uniformVariableBinds;

    typedef Util::KeyValuePair<Ptr<CoreGraphics::ShaderVariable>, Ptr<CoreGraphics::ConstantBuffer>> BlockBufferBinding;
    Util::Array<BlockBufferBinding> blockToBufferBindings;
};

//------------------------------------------------------------------------------
/**
*/
inline AnyFX::Effect* 
OGL4ShaderInstance::GetAnyFXEffect()
{
    return this->effect;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------

    