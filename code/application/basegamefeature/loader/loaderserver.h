#pragma once
//------------------------------------------------------------------------------
/**
    @class BaseGameFeature::LoaderServer

    The BaseGameFeature::LoaderServer is the central object of the loader subsystem
    Usually you don't work directly with the Loader subsystem, but instead
    use higher level classes like the Game::SetupManager and 
    Game::SaveGameManager.

    (C) 2003 RadonLabs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/ptr.h"
#include "core/singleton.h"
#include "basegamefeature/loader/userprofile.h"
#include "entityloaderbase.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace BaseGameFeature
{
class LevelLoader;

class LoaderServer : public Core::RefCounted
{
    __DeclareSingleton(LoaderServer)
	__DeclareClass(LoaderServer)
public:
    /// constructor
    LoaderServer();
    /// destructor
    virtual ~LoaderServer();

    /// enable/disable debug text messages during load
    void SetDebugTextEnabled(bool b);
    /// get debug text enabled flag
    bool GetDebugTextEnabled() const;

    /// open the loader subsystem
    virtual bool Open();
    /// close the loader subsystem
    virtual void Close();
    /// return true if open
    bool IsOpen() const;
    
    /// create a new user profile object
    virtual Ptr<UserProfile> CreateUserProfile() const;
    /// set the current user profile
    void SetUserProfile(const Ptr<UserProfile>& p);
    /// get the current user profile
    const Ptr<UserProfile>& GetUserProfile() const;
    
    /// load a new level, this method is usually called by Game::SetupManager
	virtual bool LoadLevel(const Util::String& levelName);

    /// attach loader
    void AttachEntityLoader(const Ptr<EntityLoaderBase>& loader);
    /// remove loader
    void DetachEntityLoader();
    /// load entities from file with entityloader
    void LoadEntities(const Util::String& file);

    /// set progress indicator gui resource
    void SetProgressResource(const Util::String& r);
    /// get progress indicator gui resource
    const Util::String& GetProgressResource() const;
    /// set the max progress value
    void SetMaxProgressValue(int v);
    /// get the max progress value
    int GetMaxProgressValue() const;
    /// advance the progress indicator
    void AdvanceProgress(int amount);
    /// set optional progress text
    void SetProgressText(const Util::String& s);
    /// get optional progress text
    const Util::String& GetProgressText() const;
    /// update the progress indicator display
    virtual void UpdateProgressDisplay();
    /// open the progress indicator
    void OpenProgressIndicator();
    /// close the progress indicator
    void CloseProgressIndicator();
    /// get filename for next screenshot
    IO::URI GetScreenshotFilepath(const Util::String& extension);
    
private:
    friend class LevelLoader;

    bool isOpen;
    bool debugTextEnabled;
    Ptr<UserProfile> userProfile;
    Ptr<EntityLoaderBase> entityLoader;
};

//------------------------------------------------------------------------------
/**
*/
inline void
LoaderServer::SetDebugTextEnabled(bool b)
{
    this->debugTextEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
LoaderServer::GetDebugTextEnabled() const
{
    return this->debugTextEnabled;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
LoaderServer::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
    This sets the current user profile.
*/
inline void
LoaderServer::SetUserProfile(const Ptr<UserProfile>& p)
{
    this->userProfile = p;
}

//------------------------------------------------------------------------------
/**
    Returns the current user profile
*/
inline const Ptr<UserProfile>&
LoaderServer::GetUserProfile() const
{
    return this->userProfile;
}

//------------------------------------------------------------------------------
/**
*/
inline void
LoaderServer::SetProgressResource(const Util::String& s)
{
    n_assert(this->IsOpen());
    //this->progressIndicator->SetResource(s);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
LoaderServer::GetProgressResource() const
{
    n_assert(this->IsOpen());
    const static Util::String emptyString;
    return emptyString;
}

//------------------------------------------------------------------------------
/**
*/
inline void
LoaderServer::SetMaxProgressValue(int v)
{
    n_assert(this->IsOpen());
    //this->progressIndicator->SetMaxProgressValue(v);
}

//------------------------------------------------------------------------------
/**
*/
inline int
LoaderServer::GetMaxProgressValue() const
{
    n_assert(this->IsOpen());
    return 0;//return this->progressIndicator->GetMaxProgressValue();
}

//------------------------------------------------------------------------------
/**
*/
inline void
LoaderServer::AdvanceProgress(int amount)
{
    n_assert(this->IsOpen());
    //this->progressIndicator->AdvanceProgress(amount);
}

//------------------------------------------------------------------------------
/**
*/
inline void
LoaderServer::SetProgressText(const Util::String& s)
{
    n_assert(this->IsOpen());
    //this->progressIndicator->SetText(s);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
LoaderServer::GetProgressText() const
{
    n_assert(this->IsOpen());
    const static Util::String emptyString;
    return emptyString;
}

//------------------------------------------------------------------------------
/**
*/
inline void
LoaderServer::OpenProgressIndicator()
{
    n_assert(this->IsOpen());
    //this->progressIndicator->Open();
    //this->progressIndicator->SetDebugTextEnabled(this->debugTextEnabled);
    //this->progressIndicator->SetText("No Progress Indicator Text Set!");
    //this->progressIndicator->Present();
}

//------------------------------------------------------------------------------
/**
*/
inline void
LoaderServer::CloseProgressIndicator()
{
    n_assert(this->IsOpen());
    //this->progressIndicator->Close();
}

//------------------------------------------------------------------------------
/**
*/
inline void
LoaderServer::UpdateProgressDisplay()
{
    n_assert(this->IsOpen());
    //if (this->progressIndicator->IsOpen())
    //{
    //    this->progressIndicator->Present();
    //}
}

}; // namespace BaseGameFeature
//------------------------------------------------------------------------------
