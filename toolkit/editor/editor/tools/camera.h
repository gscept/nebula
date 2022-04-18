#pragma once
//------------------------------------------------------------------------------
/**
	Editor::Camera

	(C) 2018-2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "renderutil/mayacamerautil.h"
#include "renderutil/freecamerautil.h"
#include "input/inputserver.h"
#include "graphics/view.h"
#include "graphics/stage.h"

namespace Editor
{

class Camera
{
public:
    enum CameraMode
    {
        ORBIT,
        FREECAM
    };

    enum ProjectionMode
    {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

	Camera();
	~Camera();

	void AttachToView(const Ptr<Graphics::View>& view);
	void Setup(SizeT screenWidth, SizeT screenHeight);
	void Update();
	void Reset();

    bool const GetCameraMode() const;
    void SetCameraMode(CameraMode mode);

    bool const GetProjectionMode() const;
    void SetProjectionMode(ProjectionMode mode);

    void SetTransform(Math::mat4 const& val);

public:
    // Perspective settings
    float fov = 75.0f;

    // Orthographic settings
    float orthoWidth = 20;
    float orthoHeight = 20;

private:
	Graphics::GraphicsEntityId cameraEntityId;

	Math::transform44 transform;
    
    SizeT screenWidth = 0;
    SizeT screenHeight = 0;

    CameraMode cameraMode = CameraMode::ORBIT;
    ProjectionMode projectionMode = ProjectionMode::PERSPECTIVE;
	float zoomIn = 0.0f;
	float zoomOut = 0.0f;
	Math::float2 panning{ 0.0f,0.0f };
	Math::float2 orbiting{ 0.0f,0.0f };
	RenderUtil::MayaCameraUtil mayaCameraUtil;
	RenderUtil::FreeCameraUtil freeCamUtil;
	Math::vec3 defaultViewPoint;
};

} // namespace Editor
