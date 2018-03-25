//------------------------------------------------------------------------------
//  remotejobservice.cc
//  (C) 2009 RadonLabs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "remotejobservice.h"
#include "distributedjobs/distributedjob.h"
#include "io/xmlreader.h"
#include "io/xmlwriter.h"
#include "io/memorystream.h"
#include "io/console.h"
#include "net/messageclient.h"

using namespace Net;
using namespace IO;
using namespace Util;

namespace DistributedTools
{
    __ImplementClass(DistributedTools::RemoteJobService,'DRJS',Core::RefCounted)
//------------------------------------------------------------------------------
/**
    Constructor	
*/
RemoteJobService::RemoteJobService() :
    isOpen(false),
    state(Closed),
    processorCount(1),
    verboseFlag(false),
    isClean(true)
{
}

//------------------------------------------------------------------------------
/**
    Etablish a connection to a remote service	
*/
bool
RemoteJobService::Open()
{
    n_assert(!this->isOpen);
   
    if (!this->clientID.IsValid())
    {
        this->clientID.Generate();
    }

    this->tcpclient = MessageClient::Create();
    this->tcpclient->SetBlocking(true);
    this->tcpclient->SetServerAddress(this->ipAddress);
    
    if (TcpClient::Error != this->tcpclient->Connect())
    {
        if (this->tcpclient->IsConnected())
        {
            this->state = Connecting;
            this->SendDetailedStateRequest();
            this->isOpen = true;
            return true;
        }
    }
    this->tcpclient = nullptr;
    return false;
}

//------------------------------------------------------------------------------
/**
    Close connection to service	
*/
void
RemoteJobService::Close()
{
    n_assert(this->isOpen);
    // we have to check for connected tcpclient,
    // because Send() can disconnect from
    // the client in some cases...
    if (this->tcpclient->IsConnected())
    {
        this->tcpclient->Disconnect();
    }
    this->tcpclient = nullptr;
    this->state = Closed;

    if (this->timer.Running())
    {
        this->timer.Stop();
    }
    this->timer.Reset();

    if (this->lastReceivedAnswerTimer.Running())
    {
        this->lastReceivedAnswerTimer.Stop();
    }
    this->lastReceivedAnswerTimer.Reset();

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
	Send a job to the service
*/
bool
RemoteJobService::SendJob(const Ptr<DistributedJob> & job)
{

    bool success = false;
    n_assert(this->tcpclient.isvalid());
    if (!this->tcpclient->IsConnected())
    {
        this->Close();
        this->state = ServiceError;
        this->errorMessage = "Lost connection to service.";
        return false;
    }
    Ptr<XmlWriter> writer = XmlWriter::Create();
    writer->SetStream(this->tcpclient->GetSendStream());
    if (writer->Open())
    {
        writer->BeginNode("Command");
            this->WriteClientHeader(writer);
            writer->SetString("name","JobCommand");
            if (job->IsA('DTAJ'))
            {
                writer->BeginNode("AppJob");
            }
            else if (job->IsA('DDCJ'))
            {
                writer->BeginNode("DeleteDirectoryContentJob");
            }
            else
            {
                writer->BeginNode("Job");
            }
            job->GenerateJobCommand(writer);
            writer->EndNode();
        writer->EndNode();
        writer->Close();
        success = this->tcpclient->Send();
        if (success)
        {
            this->sentJobGuids.Append(job->GetGuid());
            this->uncompleteJobs.Append(job);
        }
    }
    writer = nullptr;
    return success;
}

//------------------------------------------------------------------------------
/**
	Run all sent jobs
*/
void
RemoteJobService::RunJobs()
{
    n_assert(this->tcpclient.isvalid());
    if (!this->tcpclient->IsConnected())
    {
        this->Close();
        this->state = ServiceError;
        this->errorMessage = "Lost connection to service.";
        return;
    }
    Ptr<XmlWriter> writer = XmlWriter::Create();
    writer->SetStream(this->tcpclient->GetSendStream());
    if (writer->Open())
    {
        writer->BeginNode("Command");
        this->WriteClientHeader(writer);
        writer->SetString("name","RunJobs");
            writer->BeginNode("InitializeJobs");
            IndexT job;
            for (job = 0; job < this->initializeJobs.Size(); job++)
            {
            	if (this->initializeJobs[job]->IsA('DTAJ'))
            	{
                    writer->BeginNode("AppJob");
            	}
                else if (this->initializeJobs[job]->IsA('DDCJ'))
                {
                    writer->BeginNode("DeleteDirectoryContentJob");
                }
                else
                {
                    writer->BeginNode("Job");
                }
                this->initializeJobs[job]->GenerateJobCommand(writer);
                writer->EndNode();
            }
            writer->EndNode();
            writer->BeginNode("FinalizeJobs");
            for (job = 0; job < this->finalizeJobs.Size(); job++)
            {
                if (this->finalizeJobs[job]->IsA('DTAJ'))
                {
                    writer->BeginNode("AppJob");
                }
                else if (this->finalizeJobs[job]->IsA('DDCJ'))
                {
                    writer->BeginNode("DeleteDirectoryContentJob");
                }
                else
                {
                    writer->BeginNode("Job");
                }
                this->finalizeJobs[job]->GenerateJobCommand(writer);
                writer->EndNode();
            }
            writer->EndNode();
        writer->EndNode();
        writer->Close();
        this->tcpclient->Send();
    }
    writer = nullptr;
    this->state = Busy;

    // because out-dated state messages may arrive, remove all guids
    // from the pending list... this way only actual messages are handled
    if (this->pendingRequestGuids.Size() > 0)
    {
        n_printf("Discarding %i pending requests.\n",this->pendingRequestGuids.Size());
    }
    this->pendingRequestGuids.Clear();

    return;    
}
//------------------------------------------------------------------------------
/**
    Updates the state of the job and handles receiving messages
*/
void
RemoteJobService::Update()
{
    n_assert(this->isOpen);
    n_assert(this->tcpclient.isvalid());
    if (!this->tcpclient->IsConnected())
    {
        this->Close();
        this->state = ServiceError;
        this->errorMessage = "Lost connection to service.";
        return;
    }
    // send a state request all 1 seconds
    if (timer.GetTime()>1 || !timer.Running())
    {
        if (!this->SendStateRequest())
        {
            // the connection seems to be invalid
            this->Close();
            this->state = ServiceError;
            this->errorMessage = "Lost connection to service.";
            return;
        }
    }

    // switch to non-blocking mode only for receiving
    this->tcpclient->SetBlocking(false);
    if (this->tcpclient->Recv())
    {
        this->tcpclient->SetBlocking(true);
        
        Ptr<XmlReader> reader = XmlReader::Create();
        reader->SetStream(this->tcpclient->GetRecvStream());
        if (reader->Open())
        {
            // check if incoming data is a service command
            if (reader->HasNode("/Command"))
            {
                reader->SetToNode("/Command");
                this->HandleCommandNode(reader, this->tcpclient->GetServerAddress());
                // restart answer timer
                if (this->lastReceivedAnswerTimer.Running())
                {
                    this->lastReceivedAnswerTimer.Stop();
                    this->lastReceivedAnswerTimer.Reset();
                }
                this->lastReceivedAnswerTimer.Start();
            }
            reader->Close();
        }
        reader = nullptr;
    }
    this->tcpclient->SetBlocking(true);

    // switch to error state, if last answer is over 15 minutes old...
    if (this->lastReceivedAnswerTimer.GetTime() > 900)
    {
        this->errorMessage = "Expected service answers timed out.";
        this->state = ServiceError;
    }
}
//------------------------------------------------------------------------------
/**
    Handles a command node in the given xml reader that points to this node.
*/
void
RemoteJobService::HandleCommandNode(const Ptr<XmlReader> & reader, const Net::IpAddress & address)
{
    n_assert("Command" == reader->GetCurrentNodeName());
    if (reader->HasAttr("name"))
    {
        // handle different commands here
        if ("DetailedState" == reader->GetString("name"))
        {
            this->HandleStateNode(reader, address);
            if (reader->HasNode("Processorcount"))
            {
                if (reader->SetToFirstChild("Processorcount"))
                {
                    if(reader->HasAttr("value"))
                    {
                        this->processorCount = reader->GetInt("value");
                    }
                    reader->SetToParent();
                }
            }
        }
        else if ("CurrentState" == reader->GetString("name"))
        {
            this->HandleStateNode(reader, address);
        }
        else if ("JobAppendSuccess" == reader->GetString("name"))
        {
            if (reader->HasAttr("jobGuid"))
            {
                IndexT idx = this->sentJobGuids.FindIndex(Guid::FromString(reader->GetString("jobGuid")));
                n_assert(idx!=InvalidIndex);
                this->addedJobGuids.Append(this->sentJobGuids[idx]);
                this->sentJobGuids.EraseIndex(idx);
            }
        }
        else if ("JobAppendFailed" == reader->GetString("name"))
        {
            if (reader->HasAttr("jobGuid"))
            {
                bool retry = false;
                if (reader->HasAttr("reason"))
                {
                    if (reader->GetString("reason") == "busy")
                    {
                        retry = true;
                        this->state = Busy;
                    }
                    else if(reader->GetString("reason") == "reserved")
                    {
                        retry = true;
                        this->state = Reserved;
                    }

                }
                if (retry)
                {
                    IndexT idx = this->sentJobGuids.FindIndex(Guid::FromString(reader->GetString("jobGuid")));
                    n_assert(idx!=InvalidIndex);
                    this->failedJobGuids.Append(this->sentJobGuids[idx]);
                    this->sentJobGuids.EraseIndex(idx);
                }
                else
                {
                    IndexT idx = this->sentJobGuids.FindIndex(Guid::FromString(reader->GetString("jobGuid")));
                    n_assert(idx!=InvalidIndex);
                    this->sentJobGuids.EraseIndex(idx);
                    this->state = ServiceError;
                    this->errorMessage = "Failed to add a job to a remote service.";
                }
            }
        }
        else if ("CleanedUp" == reader->GetString("name"))
        {
            this->isClean = true;
        }
        
        // check for job output

        if (reader->HasNode("JobOutput"))
        {
            reader->SetToFirstChild("JobOutput");
            do 
            {
                Console::Instance()->Print(
                    "\n\n[Remote output from %s]\n\n%s\n\n",
                    address.GetHostName().AsCharPtr(),
                    reader->GetContent().AsCharPtr());
            } while (reader->SetToNextChild("JobOutput"));
        }

        // check for finished-job or error nodes in the command xml
        
        if (reader->HasNode("JobFinished"))
        {
            reader->SetToFirstChild("JobFinished");
            do 
            {
                n_printf("Received command node 'JobFinished'\n");
                if (reader->HasAttr("jobGuid"))
                {
                    Guid guid = Guid::FromString(reader->GetString("jobGuid"));
                    this->finishedJobGuids.Append(guid);
                    IndexT idx = this->addedJobGuids.FindIndex(guid);
                    if (idx!=InvalidIndex)
                    {
                        this->addedJobGuids.EraseIndex(idx);
                    }
                }
                if (reader->HasAttr("wasLastJob"))
                {
                    if (reader->GetBool("wasLastJob"))
                    {
                        this->uncompleteJobs.Clear();
                    }
                }
            } while (reader->SetToNextChild("JobFinished"));
        }
        
        if (reader->HasNode("JobError"))
        {
            reader->SetToFirstChild("JobError");
            do
            {
                if (reader->HasAttr("jobGuid"))
                {
                    this->state = ServiceError;
                    this->errorMessage.Format("Job with Guid %s reported an error.\n",reader->GetString("jobGuid").AsCharPtr());
                }
            } while (reader->SetToNextChild("JobError"));
        }
    }
}

//------------------------------------------------------------------------------
/**
    Parses the valid state node for the current state
*/
void
RemoteJobService::HandleStateNode(const Ptr<IO::XmlReader> & reader, const Net::IpAddress & address)
{
    if(reader->HasAttr("value") && reader->HasAttr("guid") && this->state != ServiceError)
    {
        // handle that message only if the received guid is equal
        // to one guid in the pending guid list. In that case remove
        // the guid from the list.
        IndexT guidIdx = this->pendingRequestGuids.FindIndex(
                                Guid::FromString(reader->GetString("guid"))
                                );
        if(guidIdx!=InvalidIndex)
        {
            this->pendingRequestGuids.EraseIndex(guidIdx);
            if(this->verboseFlag)
            {
                n_printf("Received State: '%s' from %s ( %s )\n", reader->GetString("value").AsCharPtr(), address.GetHostName().AsCharPtr(), address.GetHostAddr().AsCharPtr());
            }
            if("ready" == reader->GetString("value"))
            {
                this->state = Ready;
            }
            else if("reserved" == reader->GetString("value"))
            {
                this->state = Reserved;
            }
            else if("exclusiveReady" == reader->GetString("value"))
            {
                this->state = ExclusiveReady;
            }
            else if("busy" == reader->GetString("value"))
            {
                this->state = Busy;
            }
            else if("inactive" == reader->GetString("value"))
            {
                this->state = Closed;
            }
            else
            {
                this->state = ServiceError;
                this->errorMessage = "Received invalid state";
            }
        }
        else
        {
            if(verboseFlag)
            {
                n_printf("Received and ignored outdated State: '%s' from %s ( %s )\n", reader->GetString("value").AsCharPtr(), address.GetHostName().AsCharPtr(), address.GetHostAddr().AsCharPtr());
            }
        }
    }
    else
    {
        this->state = ServiceError;
        this->errorMessage = "Received invalid state";
    }
}

//------------------------------------------------------------------------------
/**
    Send a state request, to get the state of the remote service	
*/
bool
RemoteJobService::SendStateRequest()
{
    // Create a guid that identifies this request.
    // If a state messsage is received, it will only handled,
    // if it has a guid in it, that was sent by us.
    // Pending guids may be deleted, if no out-dated messages should handled...   
    Guid guid;
    guid.Generate();

    if(this->timer.Running())
    {
        this->timer.Stop();
        this->timer.Reset();
    }
    this->timer.Start();
    bool retval = false;
    n_assert(this->tcpclient.isvalid());
    if(!this->tcpclient->IsConnected())
    {
        this->Close();
        this->state = ServiceError;
        this->errorMessage = "Lost connection to service.";
        return retval;
    }
    Ptr<XmlWriter> writer = XmlWriter::Create();
    writer->SetStream(this->tcpclient->GetSendStream());
    if(writer->Open())
    {
        writer->BeginNode("Command");
            this->WriteClientHeader(writer);
            writer->SetString("name","StateRequest");
            writer->SetString("guid",guid.AsString());
        writer->EndNode();
        writer->Close();
        retval = this->tcpclient->Send();
        if(retval)
        {
            this->pendingRequestGuids.Append(guid);
        }
    }
    writer = nullptr;
    return retval;
}

//------------------------------------------------------------------------------
/**
    Send a request that asks for a detailed status of the service.
    This is called when the service connects to the remote process the first time.
*/
bool
RemoteJobService::SendDetailedStateRequest()
{
    // Create a guid that identifies this request.
    // If a state messsage is received, it will only handled,
    // if it has a guid in it, that was sent by us.
    // Pending guids may be deleted, if no out-dated messages should handled...   
    Guid guid;
    guid.Generate();

    bool retval = false;
    n_assert(this->tcpclient.isvalid());
    if(!this->tcpclient->IsConnected())
    {
        this->Close();
        this->state = ServiceError;
        this->errorMessage = "Lost connection to service.";
        return retval;
    }
    Ptr<XmlWriter> writer = XmlWriter::Create();
    writer->SetStream(this->tcpclient->GetSendStream());
    if(writer->Open())
    {
        writer->BeginNode("Command");
            this->WriteClientHeader(writer);
            writer->SetString("name","DetailedStateRequest");
            writer->SetString("guid",guid.AsString());
        writer->EndNode();
        writer->Close();
        retval = this->tcpclient->Send();
        if(retval)
        {
            this->pendingRequestGuids.Append(guid);
        }
    }
    writer = nullptr;
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
RemoteJobService::WriteClientHeader(const Ptr<IO::XmlWriter> & writer)
{
    writer->SetString("clientID", this->clientID.AsString());
}

//------------------------------------------------------------------------------
/**
	Tries to send a message to the service, that it was banned
*/
void
RemoteJobService::SendBannedInfo()
{
    if(this->IsOpen())
    {    
        // Create a guid that identifies this request.
        // If a state messsage is received, it will only handled,
        // if it has a guid in it, that was sent by us.
        // Pending guids may be deleted, if no out-dated messages should handled...   
        Guid guid;
        guid.Generate();
        
        bool retval = false;

        Ptr<XmlWriter> writer = XmlWriter::Create();
        writer->SetStream(this->tcpclient->GetSendStream());
        if (writer->Open())
        {
            writer->BeginNode("Command");
            this->WriteClientHeader(writer);
            writer->SetString("name","Banned");
            writer->SetString("guid",guid.AsString());
            writer->EndNode();
            writer->Close();
            retval = this->tcpclient->Send();
            if(retval)
            {
                this->pendingRequestGuids.Append(guid);
            }
        }
        writer = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
	Returns true if a message from the service was received previously,
    that declares the service as cleaned up.
*/
bool
RemoteJobService::HasCleanedUp()
{
    return this->isClean;
}

//------------------------------------------------------------------------------
/**
	Reset all internal parameters of this service. After reset was called,
    the service is in the closed state.
*/
void
RemoteJobService::Reset()
{
    // if service is open just close it, else be sure tcpclient and state
    // are resetted
    if (this->IsOpen())
    {
        this->Close();
    }
    else
    {
        this->tcpclient = nullptr;
        this->state = Closed;
    }
    this->addedJobGuids.Clear();
    this->failedJobGuids.Clear();
    this->finalizeJobs.Clear();
    this->finishedJobGuids.Clear();
    this->initializeJobs.Clear();
    this->pendingRequestGuids.Clear();
    this->sentJobGuids.Clear();
    this->uncompleteJobs.Clear();
    this->processorCount = 1;
    this->errorMessage = "";
    
    if (this->timer.Running())
    {
        this->timer.Stop();
    }
    this->timer.Reset();

    if (this->lastReceivedAnswerTimer.Running())
    {
        this->lastReceivedAnswerTimer.Stop();
    }
    this->lastReceivedAnswerTimer.Reset();
}

} // namespace DistributedTools