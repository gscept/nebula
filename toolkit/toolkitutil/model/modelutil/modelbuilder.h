#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ModelBuilder
    
    Merges ModelConstants, ModelAttributes and the optional ModelPhysics into a single n3 binary file.
    
    (C) 2012 Gustav Sterbrant
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "modelphysics.h"
#include "modelconstants.h"
#include "modelattributes.h"
#include "model/binarymodelwriter.h"

namespace ToolkitUtil
{
class ModelBuilder : public Core::RefCounted
{
    __DeclareClass(ModelBuilder);
public:
    /// constructor
    ModelBuilder();
    /// destructor
    virtual ~ModelBuilder();

    /// set constants pointer
    void SetConstants(const Ptr<ModelConstants>& constants);
    /// get constants
    const Ptr<ModelConstants>& GetConstants() const;
    /// set attributes pointer
    void SetAttributes(const Ptr<ModelAttributes>& attributes);
    /// get attributes
    const Ptr<ModelAttributes>& GetAttributes() const;
    /// set physics pointer
    void SetPhysics(const Ptr<ModelPhysics>& physics);
    /// get physics
    const Ptr<ModelPhysics>& GetPhysics() const;
    /// saves physics model to np3
    bool SaveN3Physics(const IO::URI& uri, Platform::Code platform);

    /// saves model to N3
    bool SaveN3(const IO::URI& uri, Platform::Code platform);

private:
    /// writes shape nodes 
    void WriteShapes(const Ptr<ModelWriter>& writer);
    /// write character node
    void WriteCharacters(const Ptr<ModelWriter>& writer);
    /// writes skins
    void WriteSkins(const Ptr<ModelWriter>& writer);
    /// writes physics
    Util::Blob WritePhysics();

    /// writes particles
    void WriteParticles(const Ptr<ModelWriter>& writer);

    Ptr<ModelConstants> constants;
    Ptr<ModelAttributes> attributes;
    Ptr<ModelPhysics> physics;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
ModelBuilder::SetConstants(const Ptr<ModelConstants>& constants)
{
    n_assert(constants.isvalid());
    this->constants = constants;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ModelConstants>& 
ModelBuilder::GetConstants() const
{
    return this->constants;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelBuilder::SetAttributes(const Ptr<ModelAttributes>& attributes)
{
    n_assert(attributes.isvalid());
    this->attributes = attributes;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ModelAttributes>& 
ModelBuilder::GetAttributes() const
{
    return this->attributes;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelBuilder::SetPhysics(const Ptr<ModelPhysics>& physics)
{
    n_assert(physics.isvalid());
    this->physics = physics;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ModelPhysics>& 
ModelBuilder::GetPhysics() const
{
    return this->physics;
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------