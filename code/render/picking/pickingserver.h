#pragma once
//------------------------------------------------------------------------------
/**
    @class Picking::PickingServer
    
    Server responsible to handle id-based rendering.
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "core/refcounted.h"
#include "math/float2.h"
#include "math/rectangle.h"

namespace Graphics
{
	class CameraEntity;
	class View;
}
namespace CoreGraphics
{
	class Texture;
}
namespace Frame
{
	class FrameScript;
}

namespace Picking
{
class PickingServer : public Core::RefCounted
{
	__DeclareClass(PickingServer);
	__DeclareSingleton(PickingServer);	
public:
	/// constructor
	PickingServer();
	/// destructor
	virtual ~PickingServer();

	/// open the PickingServer
	void Open();
	/// close the PickingServer
	void Close();
	/// returns true if server is open
	const bool IsOpen() const;

	/// renders a frame
    void Render();

	/// returns picking id from pixel
	IndexT FetchIndex(const Math::float2& position);
	/// returns array of picking ids from a rectangle
	void FetchSquare(const Math::rectangle<float>& rect, Util::Array<IndexT> & items, Util::Array<IndexT> & edgeItems);
	/// returns depth of position
	float FetchDepth(const Math::float2& position);
	/// returns normal of position
	Math::float4 FetchNormal(const Math::float2& position);
private:
	bool isOpen;
	IndexT frameIndex;
	Ptr<Frame::FrameScript> frameShader;
	Ptr<CoreGraphics::Texture> pickingBuffer;
	Ptr<CoreGraphics::Texture> depthBuffer;
	Ptr<CoreGraphics::Texture> normalBuffer;

	Ptr<Graphics::CameraEntity> pickingCamera;
	Ptr<Graphics::View> pickingView;
}; 

//------------------------------------------------------------------------------
/**
*/
inline const bool 
PickingServer::IsOpen() const
{
	return this->isOpen;
}

} // namespace Picking
//------------------------------------------------------------------------------