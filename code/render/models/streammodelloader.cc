//------------------------------------------------------------------------------
//  streammodelloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/streammodelloader.h"
#include "io/iointerface.h"
#include "io/memorystream.h"
#include "models/model.h"
#include "models/nodes/shapenode.h"
#include "characters/characterskinnode.h"
#include "particles/particlesystemnode.h"

namespace Models
{
__ImplementClass(Models::StreamModelLoader, 'SMDL', Resources::ResourceLoader);

using namespace Core;
using namespace Messaging;
using namespace IO;
using namespace Util;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
StreamModelLoader::StreamModelLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StreamModelLoader::~StreamModelLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamModelLoader::CanLoadAsync() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamModelLoader::OnLoadRequested()
{
    n_assert(this->GetState() == Resource::Initial);
    n_assert(this->resource.isvalid());
    if (this->resource->IsAsyncEnabled())
    {
        // perform asynchronous load
        // send off an asynchronous loader job
        n_assert(!this->readStreamMsg.isvalid());
        this->readStreamMsg = ReadStream::Create();
        this->readStreamMsg->SetURI(this->resource->GetResourceId().Value());
        this->readStreamMsg->SetStream(MemoryStream::Create());
        IoInterface::Instance()->Send(this->readStreamMsg.upcast<Message>());
        
        // go into Pending state
        this->SetState(Resource::Pending);
        return true;
    }
    else
    {
        // perform synchronous load
        Ptr<Stream> stream = IoServer::Instance()->CreateStream(this->resource->GetResourceId().Value());
        if (this->SetupModelFromStream(stream))
        {
            this->SetState(Resource::Loaded);
            return true;
        }
        // fallthrough: synchronous loading failed
        this->SetState(Resource::Failed);
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
StreamModelLoader::OnLoadCancelled()
{
    n_assert(this->GetState() == Resource::Pending);
    n_assert(this->readStreamMsg.isvalid());
    IoInterface::Instance()->Cancel(this->readStreamMsg.upcast<Message>());
    this->readStreamMsg = 0;
    ResourceLoader::OnLoadCancelled();
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamModelLoader::OnPending()
{
    n_assert(this->GetState() == Resource::Pending);
    n_assert(this->readStreamMsg.isvalid());
    bool retval = false;

    // check if asynchronous loader job has finished
    if (this->readStreamMsg->Handled())
    {
        // ok, loader job has finished
        if (this->readStreamMsg->GetResult())
        {
            // IO operation was successful
            if (this->SetupModelFromStream(this->readStreamMsg->GetStream()))
            {
                // everything ok!
                this->SetState(Resource::Loaded);                
                retval = true;
            }
            else
            {
                // this probably wasn't a Model file...
                this->SetState(Resource::Failed);
            }
        }
        else
        {
            // error during IO operation
            this->SetState(Resource::Failed);
        }
        // we no longer need the loader job message
        this->readStreamMsg = 0;
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
    This method actually setups the Model object from the data in the stream.
*/
bool
StreamModelLoader::SetupModelFromStream(const Ptr<Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    n_assert(this->modelNodeStack.IsEmpty());

    const Ptr<Model>& model = this->resource.downcast<Model>();

    // create a binary reader to parse the N3 file
    // @todo: map stream to memory for faster loading!
    Ptr<BinaryReader> reader = BinaryReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        // make sure it really it's actually an n3 file and check the version
        // also, we assume that the file has host-native endianess (that's
        // ensured by the asset tools)
        FourCC magic = reader->ReadUInt();
        uint version = reader->ReadUInt();
        if (magic != FourCC('NEB3'))
        {
            n_error("StreamModelLoader: '%s' is not a N3 binary file!", stream->GetURI().AsString().AsCharPtr());
            return false;
        }
        if (version != 1)
        {
            n_error("StreamModelLoader: '%s' has wrong version!", stream->GetURI().AsString().AsCharPtr());
            return false;
        }

        // start reading tags
        bool done = false;
        while ((!stream->Eof()) && (!done))
        {
            FourCC fourCC = reader->ReadUInt();
            if (fourCC == FourCC('>MDL'))
            {
                // start of Model
                FourCC classFourCC = reader->ReadUInt();
                String name = reader->ReadString();
                n_assert(model->IsInstanceOf(classFourCC));
            }
            else if (fourCC == FourCC('<MDL'))
            {
                // end of Model, if we're reloading, we shouldn't load all resources again...
                done = true;
				model->LoadResources();
                model->UpdateBoundingBox();
            }
            else if (fourCC == FourCC('>MND'))
            {
                // start of a ModelNode
                FourCC classFourCC = reader->ReadUInt();
                String name = reader->ReadString();
                this->BeginModelNode(model, classFourCC, name);
            }
            else if (fourCC == FourCC('<MND'))
            {
                // end of current ModelNode
                this->EndModelNode();
            }					
            else
            {
                // a data tag, let current model node parse the data
                this->modelNodeStack.Peek()->ParseDataTag(fourCC, reader);
            }
        }
        reader->Close();
        n_assert(this->modelNodeStack.IsEmpty());
        return true;
    }
    else
    {
        // model load failed
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Create a new ModelNode object and attach it to the model. This method
    is called when a new '>MND' tag is encountered in the model file.
*/
void
StreamModelLoader::BeginModelNode(const Ptr<Model>& model, const FourCC& classFourCC, const StringAtom& name)
{
    // setup a new ModelNode object
    Ptr<ModelNode> modelNode = (ModelNode*) Core::Factory::Instance()->Create(classFourCC);
    n_assert(modelNode->IsA(ModelNode::RTTI));
    modelNode->SetName(name);
    if (!this->modelNodeStack.IsEmpty())
    {
        modelNode->SetParent(this->modelNodeStack.Peek());
        this->modelNodeStack.Peek()->AddChild(modelNode);
    }
    this->modelNodeStack.Push(modelNode);

    // special case for shapenodes
    if (classFourCC == FourCC('SPND') && this->optionalMeshLoader.isvalid())
    {
        const Ptr<ShapeNode>& shapeNode = modelNode.cast<ShapeNode>();
        shapeNode->SetMeshResourceLoader(this->optionalMeshLoader.cast<ResourceLoader>());
    }

    // attach to Model
    model->AttachNode(modelNode);
    modelNode->BeginParseDataTags();    
}

//------------------------------------------------------------------------------
/**
    End parsing the current ModelNode. This method is called when a
    '<MND' tag is encountered in the model file.
*/
void
StreamModelLoader::EndModelNode()
{
    n_assert(!this->modelNodeStack.IsEmpty());
    Ptr<ModelNode> modelNode = this->modelNodeStack.Pop();
    modelNode->EndParseDataTags();
}

} // namespace Models
