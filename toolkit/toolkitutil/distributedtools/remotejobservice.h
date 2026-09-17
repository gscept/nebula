#pragma once
//------------------------------------------------------------------------------
/**
    @class DistributedTools::RemoteJobService

    A remote job service is able to connect to a distributed job service
    on another machine over TCP. If a connection was sucessfull, it's possible
    to send jobs to the service and start them.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "timing/timer.h"
#include "net/socket/ipaddress.h"
#include "util/guid.h"

namespace IO
{
    class XmlWriter;
    class XmlReader;
}
namespace Net
{
    class MessageClient;
}

//------------------------------------------------------------------------------
namespace DistributedTools
{
class DistributedJob;

class RemoteJobService : public Core::RefCounted
{
__DeclareClass(DistributedTools::RemoteJobService)
public:
    
    /// possible states of the service
    enum ServiceState
    {
        Closed,         // service is not available
        Connecting,     // service is conneted, but no valid state received yet
        Ready,          // service is ready
        Reserved,       // service is reserved by another client
        ExclusiveReady, // service is reserved by this client and is accepting jobs
        Busy,           // service is working
        ServiceError,   // service is in an error state
    };

    /// constructor
    RemoteJobService();
    
    /// connect to remote service
    virtual bool Open();
    /// close connection
    virtual void Close();
    
    /// send a ready job
    virtual bool SendJob(const Ptr<DistributedJob> & job);
    /// run all jobs which were sent to the service
    virtual void RunJobs();
    /// update service
    virtual void Update();

    /// is remote service open
    virtual bool IsOpen();
    
    /// check if service is accepting jobs
    virtual ServiceState GetServiceState();
    /// get sent job guid list
    const Util::Array<Util::Guid> & GetSentJobGuids();
    /// get finished job guid list
    const Util::Array<Util::Guid> & GetFinishedJobGuids();
    /// get failed job guid list
    const Util::Array<Util::Guid> & GetFailedJobGuids();
    /// remove job from failed list
    void RemoveFromFailedJobList(Util::Guid guid);
    /// mark the service as dirty until a CleanedUp message was received
    void SetDirty();
    /// send a message to the service, that it was banned
    void SendBannedInfo();
    /// returns true if the service has cleaned up after it was banned
    bool HasCleanedUp();
    /// reset the state of the service, if it was in an error state
    void Reset();

    /// set Ip address of service
    void SetIpAddress(const Net::IpAddress & ip);
    /// get Ip address of service
    const Net::IpAddress & GetIpAddress();
    /// get processor count of service
    SizeT GetProcessorCount();
    /// get the error message that is valid, when the service is in error state
    Util::String GetErrorMessage();
    /// set the initialize job list
    void SetInitializeJobs(const Util::Array<Ptr<DistributedJob>> & jobs );
    /// get the initialize job list
    const Util::Array<Ptr<DistributedJob>> & GetInitializeJobs();
    /// set the finalize job list
    void SetFinalizeJobs(const Util::Array<Ptr<DistributedJob>> & jobs );
    /// get the finalize job list
    const Util::Array<Ptr<DistributedJob>> & GetFinalizeJobs();
    /// get the uncomplete-job list
    const Util::Array<Ptr<DistributedJob>> & GetUncompleteJobs();
    /// set verbose flag
    void SetVerboseFlag(bool val);

private:
    /// handle command xml node
    virtual void HandleCommandNode(const Ptr<IO::XmlReader> & reader, const Net::IpAddress & address);
    /// handle a state node of a command
    virtual void HandleStateNode(const Ptr<IO::XmlReader> & reader, const Net::IpAddress & address);
    /// send a state request
    virtual bool SendStateRequest();
    /// send a initial request
    virtual bool SendDetailedStateRequest();

    void WriteClientHeader(const Ptr<IO::XmlWriter> & writer);

    Ptr<Net::MessageClient> tcpclient;
    bool isOpen;
    bool isClean;
    bool verboseFlag;
    ServiceState state;
    
    Util::Array<Util::Guid> sentJobGuids;
    Util::Array<Util::Guid> addedJobGuids;
    Util::Array<Util::Guid> failedJobGuids;
    Util::Array<Util::Guid> finishedJobGuids;
    Util::Array<Util::Guid> pendingRequestGuids;
    
    Util::Array<Ptr<DistributedJob>> uncompleteJobs;
    Util::Array<Ptr<DistributedJob>> initializeJobs;
    Util::Array<Ptr<DistributedJob>> finalizeJobs;

    Timing::Timer timer;
    Timing::Timer lastReceivedAnswerTimer;
    Net::IpAddress ipAddress;
    SizeT processorCount;
    Util::String errorMessage;
    Util::Guid clientID;
};
//------------------------------------------------------------------------------
/**
    Is service active   
*/
inline
bool
RemoteJobService::IsOpen()
{
    return this->isOpen;
}
//------------------------------------------------------------------------------
/**
    Gets the state of the service       
*/
inline
RemoteJobService::ServiceState
RemoteJobService::GetServiceState()
{
    return this->state;
}
//------------------------------------------------------------------------------
/**
    Get guids of sent and running jobs in one list
*/
inline
const Util::Array<Util::Guid> &
RemoteJobService::GetSentJobGuids()
{
    return this->sentJobGuids;
}
//------------------------------------------------------------------------------
/**
    Get guids of finished jobs  
*/
inline
const Util::Array<Util::Guid> &
RemoteJobService::GetFinishedJobGuids()
{
    return this->finishedJobGuids;
}
//------------------------------------------------------------------------------
/**
    Get guids of failed jobs
*/
inline
const Util::Array<Util::Guid> &
RemoteJobService::GetFailedJobGuids()
{
    return this->failedJobGuids;
}
//------------------------------------------------------------------------------
/**
    remove job from failed job list 
*/
inline
void
RemoteJobService::RemoveFromFailedJobList(Util::Guid guid)
{
    IndexT index = this->failedJobGuids.FindIndex(guid);
    if(index != InvalidIndex)
    {
        this->failedJobGuids.EraseIndex(index);
    }
}

//------------------------------------------------------------------------------
/**
    Set ip address of service   
*/
inline
void
RemoteJobService::SetIpAddress(const Net::IpAddress & ip)
{
    this->ipAddress = ip;
}

//------------------------------------------------------------------------------
/**
Get ip address of service   
*/
inline
const Net::IpAddress &
RemoteJobService::GetIpAddress()
{
    return this->ipAddress;
}

//------------------------------------------------------------------------------
/**
    Get processor count of service  
*/
inline
SizeT
RemoteJobService::GetProcessorCount()
{
    return this->processorCount;
}

//------------------------------------------------------------------------------
/**
    Get the error message that is valid when service is in error state  
*/
inline
Util::String
RemoteJobService::GetErrorMessage()
{
    n_assert(ServiceError == this->state);
    return this->errorMessage;
}
//------------------------------------------------------------------------------
/**
    Set the initialize jobs 
*/
inline
void
RemoteJobService::SetInitializeJobs(const Util::Array<Ptr<DistributedJob>> & jobs )
{
    this->initializeJobs = jobs;
}

//------------------------------------------------------------------------------
/**
    Get the initialize jobs 
*/
inline
const Util::Array<Ptr<DistributedJob>> &
RemoteJobService::GetInitializeJobs()
{
    return this->initializeJobs;
}

//------------------------------------------------------------------------------
/**
    Set the finalize jobs   
*/
inline
void
RemoteJobService::SetFinalizeJobs(const Util::Array<Ptr<DistributedJob>> & jobs )
{
    this->finalizeJobs = jobs;
}

//------------------------------------------------------------------------------
/**
    Get the finalize jobs   
*/
inline
const Util::Array<Ptr<DistributedJob>> &
RemoteJobService::GetFinalizeJobs()
{
    return this->finalizeJobs;
}

//------------------------------------------------------------------------------
/**
    Get the uncomplete jobs 
*/
inline
const Util::Array<Ptr<DistributedJob>> &
RemoteJobService::GetUncompleteJobs()
{
    return this->uncompleteJobs;
}

//------------------------------------------------------------------------------
/**
    set verbose flag    
*/
inline
void
RemoteJobService::SetVerboseFlag(bool val)
{
    this->verboseFlag = val;
}

//------------------------------------------------------------------------------
/**
    set dirty flag  
*/
inline
void
RemoteJobService::SetDirty()
{
    this->isClean = false;
}
} // namespace DistributedTools