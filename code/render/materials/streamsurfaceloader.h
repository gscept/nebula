#pragma once
//------------------------------------------------------------------------------
/**
	@class Materials::StreamSurfaceLoader
	
	Loads surface from XML declaration.
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcestreampool.h"
#include "io/stream.h"
#include "io/iointerfaceprotocol.h"
namespace Materials
{
class StreamSurfaceLoader : public Resources::ResourceStreamPool
{
	__DeclareClass(StreamSurfaceLoader);
public:
	/// constructor
	StreamSurfaceLoader();
	/// destructor
	virtual ~StreamSurfaceLoader();

	/// setup loader
	void Setup();

    /// return true if asynchronous loading is supported
    virtual bool CanLoadAsync() const;
    /// called by resource when a load is requested
    virtual bool OnLoadRequested();
    /// called by resource to cancel a pending load
    virtual void OnLoadCancelled();
    /// call frequently while after OnLoadRequested() to put Resource into loaded state
    virtual bool OnPending();

private:
    /// setup material from stream
    bool SetupMaterialFromStream(const Ptr<IO::Stream>& stream);

	/// load shader
	LoadStatus Load(const Ids::Id24 res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload shader
	void Unload(const Ids::Id24 id);

    Ptr<IO::ReadStream> readStreamMsg;
};
} // namespace Materials