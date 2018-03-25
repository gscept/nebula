#pragma once
//------------------------------------------------------------------------------
/**
    @class LevelEditor2::EditorBaseGameFeatureUnit
    
	Adapted from BaseGameFeatureUnit:
	Creates an instance of game and static dbs using the data: from the sdk
	or toolkit folder and uses the blueprints from the sdk as well

    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "basegamefeature/basegamefeatureunit.h"

//------------------------------------------------------------------------------
namespace Toolkit
{

class EditorBaseGameFeatureUnit : public BaseGameFeature::BaseGameFeatureUnit    
{
    __DeclareClass(EditorBaseGameFeatureUnit);
    __DeclareSingleton(EditorBaseGameFeatureUnit);

public:

    /// constructor
    EditorBaseGameFeatureUnit();
    /// destructor
    virtual ~EditorBaseGameFeatureUnit();

	/// called from BaseGameFeatureUnit::ActivateProperties()
    virtual void OnActivate();
	/// setup a new game, will ignore default level
	virtual bool NewGame();
	/// called at the end of the feature trigger cycle. will not call handleinput as the parent class does
	virtual void OnEndFrame();

protected:

};
};