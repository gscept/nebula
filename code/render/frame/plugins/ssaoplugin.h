#pragma once
//------------------------------------------------------------------------------
/**
	Implements HBAO  as a script algorithm
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "frameplugin.h"
#include "coregraphics/shader.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/barrier.h"
namespace Frame
{
class SSAOPlugin : public FramePlugin
{
public:
	/// constructor
	SSAOPlugin();
	/// destructor
	virtual ~SSAOPlugin();

	/// setup algorithm
	void Setup() override;
	/// discard algorithm
	void Discard() override;
    /// resize algorithm
    void Resize() override;

private:

	
	CoreGraphics::ShaderId hbaoShader, blurShader;
	CoreGraphics::ShaderProgramId xDirectionHBAO, yDirectionHBAO, xDirectionBlur, yDirectionBlur;

	Util::FixedArray<CoreGraphics::ResourceTableId> hbaoTable, blurTableX, blurTableY;
	//CoreGraphics::ResourceTableId hbaoTable, blurTableX, blurTableY;
	CoreGraphics::ConstantBufferId hbaoConstants, blurConstants;
	IndexT hbao0, hbao1, hbaoX, hbaoY, hbaoBlurRG, hbaoBlurR, hbaoC, blurC;

	CoreGraphics::ConstantBinding uvToViewAVar, uvToViewBVar, r2Var,
		aoResolutionVar, invAOResolutionVar, strengthVar, tanAngleBiasVar,
		powerExponentVar, blurFalloff, blurDepthThreshold;
		
	// read-write textures
	CoreGraphics::TextureId internalTargets[2];

	CoreGraphics::BarrierId barriers[4];

	struct AOVariables
	{
		float fullWidth, fullHeight;
		float width, height;
		float downsample;
		float nearZ, farZ;
		float sceneScale;
		Math::float2 uvToViewA;
		Math::float2 uvToViewB;
		float radius, r, r2;
		float negInvR2;
		Math::float2 focalLength;
		Math::float2 aoResolution;
		Math::float2 invAOResolution;
		float maxRadiusPixels;
		float strength;
		float tanAngleBias;
		float blurThreshold;
		float blurFalloff;
	} vars;
};
} // namespace Frame
