#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::StreamModelLoader
    
    A ResourceLoader class for setting up Models from Streams. Supports
    Nebula3 binary and XML formats, and the legacy Nebula3 nvx2 format.
    Relies on StreamReader classes which implement the actual stream
    parsing.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resourceloader.h"
#include "io/stream.h"
#include "io/iointerfaceprotocol.h"
#include "util/stack.h"
#include "models/model.h"
#include "coregraphics/streammeshloader.h"

//------------------------------------------------------------------------------
namespace Models
{
class StreamModelLoader : public Resources::ResourceLoader
{
    __DeclareClass(StreamModelLoader);
public:
    /// constructor
    StreamModelLoader();
    /// destructor
    virtual ~StreamModelLoader();
    
    /// return true if asynchronous loading is supported
    virtual bool CanLoadAsync() const;
    /// called by resource when a load is requested
    virtual bool OnLoadRequested();
    /// called by resource to cancel a pending load
    virtual void OnLoadCancelled();
    /// call frequently while after OnLoadRequested() to put Resource into loaded state
    virtual bool OnPending();
    /// set optional stream mesh loader, which is used for shapenode's mesh loading
    void SetStreamMeshLoader(const Ptr<CoreGraphics::StreamMeshLoader>& meshLaoder);

private:
    /// setup Model resource from stream
	bool SetupModelFromStream(const Ptr<IO::Stream>& stream);
    /// setup a new ModelNode
    void BeginModelNode(const Ptr<Model>& model, const Util::FourCC& classFourCC, const Util::StringAtom& name);
    /// end loading current ModelNode
    void EndModelNode();

    Ptr<IO::ReadStream> readStreamMsg;
    Util::Stack<Ptr<ModelNode> > modelNodeStack;
    Ptr<CoreGraphics::StreamMeshLoader> optionalMeshLoader;
};

//------------------------------------------------------------------------------
/**
*/
inline void 
StreamModelLoader::SetStreamMeshLoader(const Ptr<CoreGraphics::StreamMeshLoader>& meshLaoder)
{
    this->optionalMeshLoader = meshLaoder;
}

} // namespace Models
//------------------------------------------------------------------------------
    