//------------------------------------------------------------------------------
//  environmentcontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "environmentcontext.h"
#include "shared.h"
#include "coregraphics/shaderserver.h"
#include "graphicsserver.h"
#include "lighting/lightcontext.h"
#include "models/modelcontext.h"
#include "visibility/visibilitycontext.h"
#include "resources/resourceserver.h"
#include "imgui.h"
namespace Graphics
{

struct
{
	Graphics::GraphicsEntityId skyBoxEntity;
	Graphics::GraphicsEntityId sunEntity;
	Math::vec4 bloomColor;
	float bloomThreshold;
	float maxEyeLuminance;
	Math::vec4 fogColor;
	float fogDistances[2];
	int numGlobalEnvironmentMips;
	float saturation;
	float fadeValue;

	float skyTurbidity;

	CoreGraphics::TextureId defaultEnvironmentMap;
	CoreGraphics::TextureId defaultIrradianceMap;
} envState;

_ImplementPluginContext(EnvironmentContext);

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::Create(const Graphics::GraphicsEntityId sun)
{
	__bundle.OnBeforeFrame = EnvironmentContext::OnBeforeFrame;
	__bundle.OnBegin = EnvironmentContext::RenderUI;
	__bundle.StageBits = &EnvironmentContext::__state.currentStage;

	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
	envState.sunEntity = sun;

	envState.skyBoxEntity = Graphics::CreateEntity();
	Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(envState.skyBoxEntity);

	// setup both model and visibility
	Models::ModelContext::Setup(envState.skyBoxEntity, "mdl:system/skybox.n3", "system", []()
		{
			Visibility::ObservableContext::Setup(envState.skyBoxEntity, Visibility::VisibilityEntityType::Model);
		});

	envState.bloomColor = Math::vec4(1.0f);
	envState.bloomThreshold = 25.0f;
	envState.maxEyeLuminance = 0.9f;
	envState.fogColor = Math::vec4(0.5f);
	envState.fogDistances[0] = 10.0f; // near
	envState.fogDistances[1] = 1000.0f; // far
	envState.numGlobalEnvironmentMips = 10;
	envState.saturation = 1.0f;
	envState.fadeValue = 1.0f;
	envState.skyTurbidity = 2.0f;

	envState.defaultEnvironmentMap = Resources::CreateResource("tex:system/sky_refl.dds"_atm, "system"_atm,
		[](const Resources::ResourceId id)
		{
			envState.defaultEnvironmentMap = id;
			Resources::SetMaxLOD(id, 0.0f, false);
		});

	envState.defaultIrradianceMap = Resources::CreateResource("tex:system/sky_irr.dds"_atm, "system"_atm,
		[](const Resources::ResourceId id)
		{
			envState.defaultIrradianceMap = id;
			Resources::SetMaxLOD(id, 0.0f, false);
		});
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4
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

	return Math::vec4(Yz, xz, yz, 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
CalculatePerezDistribution(float t, Math::vec4& A, Math::vec4& B, Math::vec4& C, Math::vec4& D, Math::vec4& E)
{
	A = Math::vec4(0.1787f * t - 1.4630f, -0.0193f * t - 0.2592f, -0.0167f * t - 0.2608f, 0);
	B = Math::vec4(-0.3554f * t + 0.4275f, -0.0665f * t + 0.0008f, -0.0950f * t + 0.0092f, 0);
	C = Math::vec4(-0.0227f * t + 5.3251f, -0.0004f * t + 0.2125f, -0.0079f * t + 0.2102f, 0);
	D = Math::vec4(0.1206f * t - 2.5771f, -0.0641f * t - 0.8989f, -0.0441f * t - 1.6537f, 0);
	E = Math::vec4(-0.0670f * t + 0.3703f, -0.0033f * t + 0.0452f, -0.0109f * t + 0.0529f, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
EnvironmentContext::OnBeforeFrame(const Graphics::FrameContext& ctx)
{
	Shared::PerTickParams& tickParams = CoreGraphics::ShaderServer::Instance()->GetTickParams();
	Math::mat4 transform = Lighting::LightContext::GetTransform(envState.sunEntity);
	Math::vec4 sunDir = -transform.z_axis;
	
	// update perez distribution coefficients to the shared constants
	Math::vec4 A, B, C, D, E, Z;
	CalculatePerezDistribution(envState.skyTurbidity, A, B, C, D, E);
	A.store(tickParams.A);
	B.store(tickParams.B);
	C.store(tickParams.C);
	D.store(tickParams.D);
	E.store(tickParams.E);

	float thetaS = acos(Math::dot3(sunDir, Math::vec4(0, 1, 0, 0)));
	Z = CalculateZenithLuminanceYxy(envState.skyTurbidity, thetaS);
	Z.store(tickParams.Z);

	// write parameters related to atmosphere
	envState.fogColor.store(tickParams.FogColor);
	tickParams.FogDistances[0] = envState.fogDistances[0];
	tickParams.FogDistances[1] = envState.fogDistances[1];

	// bloom parameters
	envState.bloomColor.store(tickParams.HDRBloomColor);
	tickParams.HDRBrightPassThreshold = envState.bloomThreshold;

	// eye adaptation parameters
	tickParams.MaxLuminance = envState.maxEyeLuminance;

	// global resource parameters
	tickParams.EnvironmentMap = CoreGraphics::TextureGetBindlessHandle(envState.defaultEnvironmentMap);
	tickParams.IrradianceMap = CoreGraphics::TextureGetBindlessHandle(envState.defaultIrradianceMap);
	tickParams.NumEnvMips = envState.numGlobalEnvironmentMips;

	Math::vec4 balance(1.0f);
	balance.store(tickParams.Balance);
	tickParams.Saturation = envState.saturation;
	tickParams.FadeValue = envState.fadeValue;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::RenderUI(const Graphics::FrameContext& ctx)
{
	float col[4];
	envState.fogColor.storeu(col);
	Shared::PerTickParams& tickParams = CoreGraphics::ShaderServer::Instance()->GetTickParams();
	if (ImGui::Begin("Enviroment Params"))
	{
		ImGui::SetWindowSize(ImVec2(240, 400), ImGuiCond_Once);
		ImGui::SliderFloat("Bloom Threshold", &envState.bloomThreshold, 0, 100.0f);
		ImGui::SliderFloat("Sky Turbidity", &envState.skyTurbidity, 2.0f, 15.0f);
		ImGui::InputFloat("Fog Start", &envState.fogDistances[0], 0, 10000.0f);
		ImGui::InputFloat("Fog End", &envState.fogDistances[1], 0, 10000.0f);
		ImGui::ColorEdit4("Fog Color", col);
		ImGui::SliderFloat("Max Luminance", &envState.maxEyeLuminance, 0, 100.0f);
		ImGui::SliderFloat("Color Saturation", &envState.saturation, 0, 1.0f);
		ImGui::SliderFloat("Fade Value", &envState.fadeValue, 0, 1.0f);
	}
	envState.fogColor.loadu(col);

	ImGui::End();
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetFogColor(const Math::vec4& fogColor)
{
	envState.fogColor = fogColor;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetFogDistances(const float nearFog, const float farFog)
{
	envState.fogDistances[0] = nearFog;
	envState.fogDistances[1] = farFog;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetBloomColor(const Math::vec4& bloomColor)
{
	envState.bloomColor = bloomColor;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetBloomThreshold(const float threshold)
{
	envState.bloomThreshold = threshold;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetMaxLuminance(const float maxLuminance)
{
	envState.maxEyeLuminance = maxLuminance;
}

//------------------------------------------------------------------------------
/**
*/
void 
EnvironmentContext::SetNumEnvironmentMips(const int mips)
{
	envState.numGlobalEnvironmentMips = mips;
}

} // namespace Graphics
