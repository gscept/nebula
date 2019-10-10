//------------------------------------------------------------------------------
//  environmentcontext.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "environmentcontext.h"
#include "shared.h"
#include "coregraphics/shaderserver.h"
#include "graphicsserver.h"
#include "lighting/lightcontext.h"
#include "models/modelcontext.h"
#include "visibility/visibilitycontext.h"
namespace Graphics
{

struct
{
	Graphics::GraphicsEntityId skyBoxEntity;
	Graphics::GraphicsEntityId sunEntity;
	Math::float4 BloomColor;
	float BloomThreshold;
	float MaxEyeLuminance;
	Math::float4 FogColor;
	float FogDistances[2];
	int NumGlobalEnvironmentMips;
} envState;

_ImplementPluginContext(EnvironmentContext);

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::Create(const Graphics::GraphicsEntityId sun)
{
	__bundle.OnBeforeFrame = EnvironmentContext::OnBeforeFrame;
	__bundle.OnWaitForWork = nullptr;
	__bundle.OnBeforeView = nullptr;
	__bundle.OnAfterView = nullptr;
	__bundle.OnAfterFrame = nullptr;
	__bundle.StageBits = &EnvironmentContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = nullptr;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
	envState.sunEntity = sun;

	envState.skyBoxEntity = Graphics::CreateEntity();
	Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(envState.skyBoxEntity);

	// setup both model and visibility
	Models::ModelContext::Setup(envState.skyBoxEntity, "mdl:system/skybox.n3", "system");
	Visibility::ObservableContext::Setup(envState.skyBoxEntity, Visibility::VisibilityEntityType::Model);

	envState.BloomColor = Math::float4(1.0f);
	envState.BloomThreshold = 6.0f;
	envState.MaxEyeLuminance = 1.0f;
	envState.FogColor = Math::float4(0.5f);
	envState.FogDistances[0] = 10.0f; // near
	envState.FogDistances[1] = 1000.0f; // far
	envState.NumGlobalEnvironmentMips = 10;
}

//------------------------------------------------------------------------------
/**
*/
Math::float4
CalculateZenithLuminanceYxy(float t, float thetaS)
{
	float chi = (4.0 / 9.0 - t / 120.0) * (PI - 2.0 * thetaS);
	float Yz = (4.0453 * t - 4.9710) * tan(chi) - 0.2155 * t + 2.4192;

	float theta2 = thetaS * thetaS;
	float theta3 = theta2 * thetaS;
	float T = t;
	float T2 = t * t;

	float xz =
		(0.00165 * theta3 - 0.00375 * theta2 + 0.00209 * thetaS + 0.0) * T2 +
		(-0.02903 * theta3 + 0.06377 * theta2 - 0.03202 * thetaS + 0.00394) * T +
		(0.11693 * theta3 - 0.21196 * theta2 + 0.06052 * thetaS + 0.25886);

	float yz =
		(0.00275 * theta3 - 0.00610 * theta2 + 0.00317 * thetaS + 0.0) * T2 +
		(-0.04214 * theta3 + 0.08970 * theta2 - 0.04153 * thetaS + 0.00516) * T +
		(0.15346 * theta3 - 0.26756 * theta2 + 0.06670 * thetaS + 0.26688);

	return Math::float4(Yz, xz, yz, 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
CalculatePerezDistribution(float t, Math::float4& A, Math::float4& B, Math::float4& C, Math::float4& D, Math::float4& E)
{
	A = Math::float4(0.1787f * t - 1.4630f, -0.0193f * t - 0.2592f, -0.0167f * t - 0.2608f, 0);
	B = Math::float4(-0.3554f * t + 0.4275f, -0.0665f * t + 0.0008f, -0.0950f * t + 0.0092f, 0);
	C = Math::float4(-0.0227f * t + 5.3251f, -0.0004f * t + 0.2125f, -0.0079f * t + 0.2102f, 0);
	D = Math::float4(0.1206f * t - 2.5771f, -0.0641f * t - 0.8989f, -0.0441f * t - 1.6537f, 0);
	E = Math::float4(-0.0670f * t + 0.3703f, -0.0033f * t + 0.0452f, -0.0109f * t + 0.0529f, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
EnvironmentContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks)
{
	Shared::PerTickParams& tickParams = CoreGraphics::ShaderServer::Instance()->GetTickParams();
	Math::matrix44 transform = Lighting::LightContext::GetTransform(envState.sunEntity);
	Math::float4 sunDir = -transform.get_zaxis();
	
	// update perez distribution coefficients to the shared constants
	const float turbidity = 2.0f;
	Math::float4 A, B, C, D, E, Z;
	CalculatePerezDistribution(turbidity, A, B, C, D, E);
	Math::float4::storeu(A, tickParams.A);
	Math::float4::storeu(B, tickParams.B);
	Math::float4::storeu(C, tickParams.C);
	Math::float4::storeu(D, tickParams.D);
	Math::float4::storeu(E, tickParams.E);

	float thetaS = acos(Math::float4::dot3(sunDir, Math::vector(0, 1, 0)));
	Z = CalculateZenithLuminanceYxy(turbidity, thetaS);
	Math::float4::storeu(Z, tickParams.Z);

	// write parameters related to atmosphere
	Math::float4::storeu(envState.FogColor, tickParams.FogColor);
	tickParams.FogDistances[0] = envState.FogDistances[0];
	tickParams.FogDistances[1] = envState.FogDistances[1];

	// bloom parameters
	Math::float4::storeu(envState.BloomColor, tickParams.HDRBloomColor);
	tickParams.HDRBrightPassThreshold = envState.BloomThreshold;

	// eye adaptation parameters
	tickParams.MaxLuminance = envState.MaxEyeLuminance;

	// global resource parameters
	tickParams.NumEnvMips = envState.NumGlobalEnvironmentMips;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetFogColor(const Math::float4& fogColor)
{
	envState.FogColor = fogColor;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetFogDistances(const float nearFog, const float farFog)
{
	envState.FogDistances[0] = nearFog;
	envState.FogDistances[1] = farFog;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetBloomColor(const Math::float4& bloomColor)
{
	envState.BloomColor = bloomColor;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetBloomThreshold(const float threshold)
{
	envState.BloomThreshold = threshold;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetMaxLuminance(const float maxLuminance)
{
	envState.MaxEyeLuminance = maxLuminance;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetNumEnvironmentMips(const int mips)
{
	envState.NumGlobalEnvironmentMips = mips;
}

} // namespace Graphics
