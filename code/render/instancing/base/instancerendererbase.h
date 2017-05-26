#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::InstanceRendererBase
    
    Base class for instance renderers.
    
    (C) 2012 Gustav Sterbrant
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "math/matrix44.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/shader.h"
namespace Base
{
class InstanceRendererBase : public Core::RefCounted
{
	__DeclareClass(InstanceRendererBase);
public:
	/// constructor
	InstanceRendererBase();
	/// destructor
	virtual ~InstanceRendererBase();

	/// setup renderer
	void Setup();
	/// close rendered
	void Close();

	/// sets the shader instance
    void SetShader(const Ptr<CoreGraphics::Shader>& shader);
	/// gets the shader instance
    const Ptr<CoreGraphics::Shader>& GetShader() const;

	/// set instancing render multiplier
	void SetInstanceMultiplier(SizeT multiplier);

	/// begins transform updates, clears transform array
	void BeginUpdate(SizeT amount);
	/// adds transform
	void AddTransform(const Math::matrix44& matrix);
    /// add id
    void AddId(const int id);
	/// ends transform updates
	void EndUpdate();

	/// render instances
	virtual void Render(const SizeT multiplier);

protected:
	Ptr<CoreGraphics::Shader> shader;
	Util::Array<Math::matrix44> modelTransforms;
	Util::Array<Math::matrix44> modelViewTransforms;
	Util::Array<Math::matrix44> modelViewProjectionTransforms;
    Util::Array<int> objectIds;
	bool inBeginUpdate;
	bool isOpen;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
InstanceRendererBase::SetShader(const Ptr<CoreGraphics::Shader>& shader)
{
	n_assert(shader.isvalid());
	this->shader = shader;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Shader>&
InstanceRendererBase::GetShader() const
{
	return this->shader;
}

} // namespace Instancing
//------------------------------------------------------------------------------