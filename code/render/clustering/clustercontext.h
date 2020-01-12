#pragma once
//------------------------------------------------------------------------------
/**
	Context handling GPU cluster culling

	(C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/window.h"
#include <array>
#include "cluster_generate.h"

namespace Clustering
{

class ClusterContext : public Graphics::GraphicsContext
{
	_DeclarePluginContext();
public:
	/// constructor
	ClusterContext();
	/// destructor
	virtual ~ClusterContext();

	/// setup light context using CameraSettings
	static void Create(float ZNear, float ZFar, const CoreGraphics::WindowId window);

	/// get cluster buffer
	static const CoreGraphics::ShaderRWBufferId GetClusterBuffer();
	/// get number of clusters 
	static const SizeT GetNumClusters();
	/// get cluster dimensions
	static const std::array<SizeT, 3> GetClusterDimensions();
	/// get cluster uniforms
	static const ClusterGenerate::ClusterUniforms& GetUniforms();

	/// do light classification for tiled/clustered compute
	static void OnBeforeView(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
#ifndef PUBLIC_BUILD
	//
	static void OnRenderDebug(uint32_t flags);
#endif
private:

	/// run light classification compute
	static void UpdateClusters();
};
} // namespace Clustering
