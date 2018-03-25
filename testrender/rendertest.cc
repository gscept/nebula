//------------------------------------------------------------------------------
// blockallocatortest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "memory/sliceallocatorpool.h"
#include "timing/timer.h"
#include "io/console.h"
#include "rendertest.h"
#include "graphics/graphicsserver.h"



using namespace Timing;
using namespace Graphics;
namespace Test
{


//------------------------------------------------------------------------------
/**
*/
void
RenderTest::Run()
{
	GraphicsServer* gfxServer = GraphicsServer::Create();
	gfxServer->Open();

	GraphicsEntityId ent = Graphics::CreateEntity();
	

	gfxServer->Close();
}

} // namespace Test