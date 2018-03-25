//------------------------------------------------------------------------------
//  levelviewergamestate.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "viewergamestate.h"
#include "graphics/stage.h"
#include "math/vector.h"
#include "math/matrix44.h"
#include "graphicsfeatureunit.h"
#include "managers/factorymanager.h"
#include "managers/entitymanager.h"
#include "posteffect/posteffectmanager.h"
#include "managers/enventitymanager.h"
#include "scriptfeature/managers/scripttemplatemanager.h"
#include "attr/attribute.h"
#include "graphicsfeature/graphicsattr/graphicsattributes.h"
#include "managers/focusmanager.h"
#include "input/keyboard.h"
#include "scriptingfeature/properties/scriptingproperty.h"
#include "scriptingfeature/scriptingprotocol.h"
#include "remoteinterface/qtremoteserver.h"
#include "basegamefeature/basegameprotocol.h"
#include "../levelviewerapplication.h"
#include "basegametiming/gametimesource.h"
#include "debugrender/debugrender.h"

namespace Tools
{
__ImplementClass(Tools::LevelViewerGameState, 'DMGS', BaseGameFeature::GameStateHandler);

using namespace BaseGameFeature;
using namespace GraphicsFeature;
using namespace PostEffect;
using namespace Graphics;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
LevelViewerGameState::LevelViewerGameState():applyTransform(true),entitiesLoaded(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
LevelViewerGameState::~LevelViewerGameState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelViewerGameState::OnStateEnter( const Util::String& prevState )
{
	Util::String newLevel = this->GetLevelName();
	if(this->lastLevel.IsEmpty())
	{
		this->lastLevel = newLevel;
	}
	
	GameStateHandler::OnStateEnter(prevState);
		
	if(prevState == "Reload" && this->lastLevel == newLevel && this->applyTransform)
	{
		Ptr<BaseGameFeature::SetTransform> msg = BaseGameFeature::SetTransform::Create();
		msg->SetMatrix(this->focusTransform);
		BaseGameFeature::FocusManager::Instance()->GetCameraFocusEntity()->SendSync(msg.cast<Messaging::Message>());
	}

}

//------------------------------------------------------------------------------
/**
*/
void 
LevelViewerGameState::OnStateLeave( const Util::String& nextState )
{	
	if(nextState == "Reload")
	{
		Ptr<Game::Entity> ent = BaseGameFeature::FocusManager::Instance()->GetCameraFocusEntity();
		if(ent.isvalid())
		{
			this->focusTransform = ent->GetMatrix44(Attr::Transform);		
		}				
	}		

	GameStateHandler::OnStateLeave(nextState);
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
LevelViewerGameState::OnFrame()
{
	//handle all user input
	if (Input::InputServer::HasInstance() && this->entitiesLoaded)
	{
		this->HandleInput();
	}

	Dynui::ImguiAddon::BeginFrame();
	Dynui::ImguiConsole::Instance()->Render();	

	// test text rendering
	/*Timing::Time frameTime = (float)BaseGameFeature::GameTimeSource::Instance()->GetFrameTime();
	Util::String fpsTxt;
	fpsTxt.Format("Game FPS: %.2f", 1/frameTime);
	_debug_text(fpsTxt, Math::float2(0.0,0.0), Math::float4(1,1,1,1));
	*/

	QtRemoteInterfaceAddon::QtRemoteServer::Instance()->OnFrame();
	Dynui::ImguiAddon::EndFrame();
	return GameStateHandler::OnFrame();
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelViewerGameState::OnLoadBefore()
{
	this->entitiesLoaded = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelViewerGameState::OnLoadAfter()
{
	this->entitiesLoaded = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelViewerGameState::HandleInput()
{
	const Ptr<Input::Keyboard>& kbd = Input::InputServer::Instance()->GetDefaultKeyboard();

	if(kbd->KeyDown(Input::Key::F5))
	{
		bool applyTrans = false;
		if(kbd->KeyPressed(Input::Key::Shift))
		{
			applyTrans = true;
		}
		this->ReloadLevel(applyTrans);
		const Ptr<BaseGameFeature::GameStateHandler>& state = App::GameApplication::Instance()->FindStateHandlerByName("Reload").cast<BaseGameFeature::GameStateHandler>();
		state->SetLevelName(BaseGameFeature::BaseGameFeatureUnit::GetCurrentLevel());
		LevelViewerGameStateApplication::Instance()->RequestState("Reload");
	}
	if (kbd->KeyDown(Input::Key::F10))
	{
		if (UI::UiFeatureUnit::Instance()->HasLayout("_levellist"))
		{
			UI::UiFeatureUnit::Instance()->GetLayout("_levellist")->Toggle();
		}		
	}
	if (kbd->KeyDown(Input::Key::F11))
	{
		if (UI::UiFeatureUnit::Instance()->HasLayout("_layoutlist"))
		{
			UI::UiFeatureUnit::Instance()->GetLayout("_layoutlist")->Toggle();
		}		
	}
}

//------------------------------------------------------------------------------
/**
*/
void
LevelViewerGameState::LoadLevel(const Util::String &level, bool applyTransform)
{
	this->applyTransform = applyTransform;
	const Ptr<BaseGameFeature::GameStateHandler>& state = App::GameApplication::Instance()->FindStateHandlerByName("Reload").cast<BaseGameFeature::GameStateHandler>();
	state->SetLevelName(level);
	LevelViewerGameStateApplication::Instance()->RequestState("Reload");
}

//------------------------------------------------------------------------------
/**
*/
void
LevelViewerGameState::ReloadLevel(bool keepTransform)
{
	this->LoadLevel(BaseGameFeature::BaseGameFeatureUnit::GetCurrentLevel(), keepTransform);
}

} // namespace Tools