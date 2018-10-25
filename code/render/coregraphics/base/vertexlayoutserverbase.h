#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::VertexLayoutServerBase
    
    The VertexLayoutServerBase creates VertexLayout objects shared by their
    vertex component signature. On some platforms it is more efficient
    to share VertexLayout objects across meshes with identical
    vertex structure. Note that there is no way to manually discard
    vertex components. Vertex components stay alive for the life time
    of the application until the Close() method of the VertexLayoutServerBase
    is called.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "coregraphics/vertexcomponent.h"
#include "util/stringatom.h"

//------------------------------------------------------------------------------

namespace CoreGraphics
{    
class VertexLayout;
}

namespace Base
{
class VertexLayoutServerBase : public Core::RefCounted
{
    __DeclareClass(VertexLayoutServerBase);
    __DeclareSingleton(VertexLayoutServerBase);
public:
    /// constructor
    VertexLayoutServerBase();
    /// destructor
    virtual ~VertexLayoutServerBase();
    
    /// open the server
    void Open();
    /// close the server
    void Close();
    /// return true if open
    bool IsOpen() const;
    
    /// create shared vertex layout object
    Ptr<CoreGraphics::VertexLayout> CreateSharedVertexLayout(const Util::Array<CoreGraphics::VertexComponent>& vertexComponents);
	/// calculate vertex size based components
	SizeT CalculateVertexSize(const Util::Array<CoreGraphics::VertexComponent>& vertexComponents);
protected:
    bool isOpen;
    Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::VertexLayout> > vertexLayouts;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
VertexLayoutServerBase::IsOpen() const
{
    return this->isOpen;
}

} // namespace VertexLayoutServerBase
//------------------------------------------------------------------------------


    