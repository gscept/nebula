//------------------------------------------------------------------------------
//  batchattributes.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "batchattributes.h"
#include "animsplitterhelper.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/xmlreader.h"

using namespace IO;
using namespace Util;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::BatchAttributes, 'BATR', Core::RefCounted);
__ImplementSingleton(ToolkitUtil::BatchAttributes);

//------------------------------------------------------------------------------
/**
*/
BatchAttributes::BatchAttributes() : 
	isOpen(false)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
BatchAttributes::~BatchAttributes()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void 
BatchAttributes::Open()
{
	n_assert(!this->isOpen);
	this->isOpen = true;
	this->Load();
}

//------------------------------------------------------------------------------
/**
*/
void 
BatchAttributes::Close()
{
	n_assert(this->isOpen);
	this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
BatchAttributes::IsOpen() const
{
	return this->isOpen;	
}

//------------------------------------------------------------------------------
/**
*/
void 
BatchAttributes::Load()
{
	n_assert(this->isOpen);
	Ptr<Stream> batchFile = IoServer::Instance()->CreateStream(URI("proj:work/assets/batchattributes.xml"));
	Ptr<XmlReader> xmlReader = XmlReader::Create();
	if (batchFile->Open())
	{
		xmlReader->SetStream(batchFile);
		xmlReader->Open();

		if (xmlReader->SetToFirstChild()) do 
		{
			String nodeName = xmlReader->GetCurrentNodeName();
			String resource = xmlReader->GetString("name");

			Ptr<AnimSplitterHelper> splitter = AnimSplitterHelper::Create();
			splitter->Setup(xmlReader);
			this->animSplitters.Add(resource, splitter);
			Ptr<SkinHelper> skinHelper = SkinHelper::Create();
			skinHelper->Setup(xmlReader);
			this->skins.Add(resource, skinHelper);
						
		} 
		while (xmlReader->SetToNextChild());
	}
}


} // namespace ToolkitUtil