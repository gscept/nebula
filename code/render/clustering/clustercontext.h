#pragma once
//------------------------------------------------------------------------------
/**
	Context handling GPU cluster culling

	(C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/buffer.h"
#include "coregraphics/constantbuffer.h"
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
	static const CoreGraphics::BufferId GetClusterBuffer();
	/// get number of clusters 
	static const SizeT GetNumClusters();
	/// get cluster dimensions
	static const std::array<SizeT, 3> GetClusterDimensions();
	/// get cluster uniforms
	static const ClusterGenerate::ClusterUniforms& GetUniforms();
	/// get cluster constant buffer
	static const CoreGraphics::ConstantBufferId GetConstantBuffer();

	/// update constants
	static void UpdateResources(const Graphics::FrameContext& ctx);
#ifndef PUBLIC_BUILD
	/// implement me
	static void OnRenderDebug(uint32_t flags);
#endif
private:

	/// run light classification compute
	static void UpdateClusters();
};
} // namespace Clustering
