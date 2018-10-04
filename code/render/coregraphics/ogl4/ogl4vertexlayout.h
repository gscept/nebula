#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4VertexLayout
  
    OpenGL4 implementation of vertex layout.

	In OpenGL, the semantics and semantic locations of vertex attributes are completely ignored.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/base/vertexlayoutbase.h"
#include "coregraphics/base/vertexcomponentbase.h"
#include "coregraphics/renderdevice.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4VertexLayout : public Base::VertexLayoutBase
{
    __DeclareClass(OGL4VertexLayout);
public:
    /// constructor
    OGL4VertexLayout();
    /// destructor
    virtual ~OGL4VertexLayout();

    /// setup the vertex layout
    void Setup(const Util::Array<CoreGraphics::VertexComponent>& c);
    /// discard the vertex layout object
    void Discard();

    /// get opengl vertex array object
    const GLuint& GetOGL4VertexArrayObject() const;

	/// applies layout before rendering
	void Apply();
        
private:

	GLuint ogl4Vao;
	static const SizeT maxElements = 24;
	Util::String semanticName[32];	
};

//------------------------------------------------------------------------------
/**
*/
inline const GLuint& 
OGL4VertexLayout::GetOGL4VertexArrayObject() const
{
    return this->ogl4Vao;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------
