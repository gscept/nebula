#pragma once
//------------------------------------------------------------------------------
/**
	@class Materials::StreamSurfaceSaver
	
	Saves a surface to an XML file.
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcesaver.h"
#include "io/stream.h"
#include "io/iointerfaceprotocol.h"
namespace Materials
{
class StreamSurfaceSaver : public Resources::ResourceSaver
{
	__DeclareClass(StreamSurfaceSaver);
public:
	/// constructor
	StreamSurfaceSaver();
	/// destructor
	virtual ~StreamSurfaceSaver();

	/// set stream to save to
	void SetStream(const Ptr<IO::Stream>& stream);
	/// get save-stream
	const Ptr<IO::Stream>& GetStream() const;

	/// called by resource when a save is requested
	bool OnSave();

private:
	Ptr<IO::Stream> stream;
};

//------------------------------------------------------------------------------
/**
*/
inline void
StreamSurfaceSaver::SetStream(const Ptr<IO::Stream>& s)
{
	this->stream = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<IO::Stream>&
StreamSurfaceSaver::GetStream() const
{
	return this->stream;
}

} // namespace Materials