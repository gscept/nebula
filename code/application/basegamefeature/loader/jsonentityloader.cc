//------------------------------------------------------------------------------
//  jsonentityloader.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jsonentityloader.h"
#include "io/jsonreader.h"

namespace BaseGameFeature
{

bool JsonEntityLoader::insideLoading = false;

//------------------------------------------------------------------------------
/**
*/
JsonEntityLoader::JsonEntityLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JsonEntityLoader::~JsonEntityLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
JsonEntityLoader::Load(const Util::String& file)
{    
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
	Ptr<IO::FileStream> stream = IO::FileStream::Create();
	stream->SetAccessMode(IO::Stream::AccessMode::ReadAccess);
	stream->SetURI(uri);
    reader->SetStream(stream);
	if (reader->Open())
    {
        
        reader->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
JsonEntityLoader::IsLoading()
{
    return insideLoading;
}

} // namespace BaseGameFeature


