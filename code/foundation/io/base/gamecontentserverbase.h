#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::GameContentServerBase
    
    The game content server initializes access to game content on console
    platforms. The GameContentServer must be created by the main thread
    before the first IoServer is created.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"

//------------------------------------------------------------------------------
namespace Base
{
class GameContentServerBase : public Core::RefCounted
{
    __DeclareClass(GameContentServerBase);
    __DeclareInterfaceSingleton(GameContentServerBase);
public:
    /// constructor
    GameContentServerBase();
    /// destructor
    virtual ~GameContentServerBase();

    /// set human readable game title
    void SetTitle(const Util::String& title);
    /// get human readable game title
    const Util::String& GetTitle() const;
    /// set title id
    void SetTitleId(const Util::String& titleId);
    /// get title id
    const Util::String& GetTitleId() const;
    /// set title version
    void SetVersion(const Util::String& version);
    /// get title version
    const Util::String& GetVersion() const;

    /// setup the object
    void Setup();
    /// discard the object
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;

protected:
    Util::String title;
    Util::String titleId;
    Util::String version;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
GameContentServerBase::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GameContentServerBase::SetTitle(const Util::String& t)
{
    this->title = t;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
GameContentServerBase::GetTitle() const
{
    return this->title;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GameContentServerBase::SetTitleId(const Util::String& tid)
{
    this->titleId = tid;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
GameContentServerBase::GetTitleId() const
{
    return this->titleId;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GameContentServerBase::SetVersion(const Util::String& v)
{
    this->version = v;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
GameContentServerBase::GetVersion() const
{
    return this->version;
}

} // namespace Base
//------------------------------------------------------------------------------

