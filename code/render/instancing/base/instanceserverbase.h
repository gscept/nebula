#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::InstanceServerBase
    
    Instancing interface server base. 
    Instancing in this context means instanced rendering.
    
    (C) 2012 Gustav Sterbrant
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "models/modelnodeinstance.h"
#include "models/modelnode.h"

namespace Instancing
{
    class InstanceRenderer;
}
namespace Base
{
class InstanceServerBase : public Core::RefCounted
{
    __DeclareSingleton(InstanceServerBase); 
    __DeclareClass(InstanceServerBase);
public:
    /// constructor
    InstanceServerBase();
    /// destructor
    virtual ~InstanceServerBase();
    /// open the InstanceServerBase
    virtual bool Open();
    /// close the InstanceServerBase
    virtual void Close();
    /// return if InstanceServerBase is open
    bool IsOpen() const;

    /// begins adding model instances to server
    void BeginInstancing(const Ptr<Models::ModelNode>& modelNode, const SizeT multiplier, const Ptr<CoreGraphics::Shader>& shader, const IndexT& pass);
    /// adds instance to current model
    void AddInstance(const IndexT& instanceCode, const Ptr<Models::ModelNodeInstance>& instance);
    /// ends adding model instances
    void EndInstancing();

    /// updates instance server (call from batch)
    virtual void Render(IndexT frameIndex);

protected:
    IndexT pass;
    Ptr<CoreGraphics::Shader> shader;
    Ptr<Instancing::InstanceRenderer> renderer;
    Ptr<Models::ModelNode> modelNode;
    Util::Dictionary<IndexT, Util::Array<Ptr<Models::ModelNodeInstance>>> instancesByCode;
    bool isBeginInstancing;
    bool isOpen;
    SizeT multiplier;
}; 

//------------------------------------------------------------------------------
/**
*/
inline bool
InstanceServerBase::IsOpen() const
{
    return this->isOpen;
}

} // namespace Instancing
//------------------------------------------------------------------------------