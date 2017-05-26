//------------------------------------------------------------------------------
//  streamsurfaceloader.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "streamsurfaceloader.h"
#include "io/iointerface.h"
#include "io/memorystream.h"
#include "materials/surface.h"
#include "io/xmlreader.h"
#include "materialserver.h"
#include "resources/resourcemanager.h"
#include "io/bxmlreader.h"

using namespace IO;
using namespace Resources;
using namespace Messaging;
using namespace Util;
namespace Materials
{
__ImplementClass(Materials::StreamSurfaceLoader, 'SSML', Resources::ResourceLoader);

//------------------------------------------------------------------------------
/**
*/
StreamSurfaceLoader::StreamSurfaceLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
StreamSurfaceLoader::~StreamSurfaceLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamSurfaceLoader::CanLoadAsync() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamSurfaceLoader::OnLoadRequested()
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
        if (this->SetupMaterialFromStream(stream))
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
StreamSurfaceLoader::OnLoadCancelled()
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
StreamSurfaceLoader::OnPending()
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
            if (this->SetupMaterialFromStream(this->readStreamMsg->GetStream()))
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
*/
bool
StreamSurfaceLoader::SetupMaterialFromStream(const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());

    // setup resource
    const Ptr<Surface>& surface = this->resource.downcast<Surface>();
	const Ptr<MaterialServer>& matServer = MaterialServer::Instance();

	Ptr<BXmlReader> reader = BXmlReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        // make sure it's a valid frame shader file
        if (!reader->HasNode("/NebulaT/Surface"))
        {
            n_error("StreamSurfaceMaterialLoader: '%s' is not a valid surface!", stream->GetURI().AsString().AsCharPtr());
            return false;
        }

        // send to first node
        reader->SetToNode("/NebulaT/Surface");

        // load surface
        Util::StringAtom materialTemplate = reader->GetString("template");
		if (!matServer->HasMaterial(materialTemplate)) return false;
		const Ptr<Material>& material = matServer->GetMaterialByName(materialTemplate);
        const Util::Dictionary<Util::StringAtom, Material::MaterialParameter>& parameters = material->GetParameters();
        if (reader->SetToFirstChild("Param")) do
        {
            Util::StringAtom paramName = reader->GetString("name");
			if (!parameters.Contains(paramName))
			{
				n_warning(Util::String::Sprintf("No parameter matching name '%s' exists in the material template '%s'\n", paramName.AsString().AsCharPtr(), material->GetName().AsString().AsCharPtr()).AsCharPtr());
				continue;
			}
            const Material::MaterialParameter& param = parameters[paramName];

			// set variant value which we will use in the surface constants
            Variant var = param.defaultVal;
            switch (param.defaultVal.GetType())
            {
                case Variant::Float:
                    var.SetFloat(reader->GetOptFloat("value", 0.0f));
                    break;
                case Variant::Int:
					var.SetInt(reader->GetOptInt("value", 0));
                    break;
                case Variant::Bool:
					var.SetBool(reader->GetOptBool("value", false));
                    break;
                case Variant::Float4:
					var.SetFloat4(reader->GetOptFloat4("value", Math::float4(0)));
                    break;
                case Variant::Float2:
					var.SetFloat2(reader->GetOptFloat2("value", Math::float2(0)));
                    break;
                case Variant::Matrix44:
					var.SetMatrix44(reader->GetOptMatrix44("value", Math::matrix44()));
                    break;
                case Variant::String:
                {
                    var.SetString(reader->GetString("value"));
                    break;
                }
            }

            // create binding object
            Surface::SurfaceValueBinding obj;
            obj.value = var;
            obj.system = param.system;

            // add to the static values in the surface
            surface->staticValues.Add(paramName, obj);
		}
		while (reader->SetToNextChild("Param"));

        surface->Setup(material);
    }
    else
    {
        return false;
    }

    // everything went fine!
    return true;
}

} // namespace Materials