#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4ShaderProgram

    Under OpenGL4, a shader variation is represented by an AnyFX effect 
    program which must be annotated by a FeatureMask string.

    (C) 2013 Gustav Sterbrant
*/
#include "afxapi.h"
#include "coregraphics/base/shadervariationbase.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4ShaderProgram : public Base::ShaderVariationBase
{
    __DeclareClass(OGL4ShaderProgram);
public:
    /// constructor
    OGL4ShaderProgram();
    /// destructor
    virtual ~OGL4ShaderProgram();

	/// applies program
	void Apply();
	/// performs a variable commit to the current program
	void Commit();
    /// override current render state to use wireframe
    void SetWireframe(bool b);

	/// returns true if shader variation needs to use patches
	const bool UsePatches() const;

	/// get program
	AnyFX::EffectProgram* GetProgram() const;
private:
    friend class OGL4Shader;
    friend class OGL4StreamShaderLoader;

	/// setup from AnyFX program
	void Setup(AnyFX::EffectProgram* program);

	bool usePatches;
	AnyFX::EffectProgram* program;

};

//------------------------------------------------------------------------------
/**
*/
inline AnyFX::EffectProgram*
OGL4ShaderProgram::GetProgram() const
{
	return this->program;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool 
OGL4ShaderProgram::UsePatches() const
{
	return this->usePatches;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------
