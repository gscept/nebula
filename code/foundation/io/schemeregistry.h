#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::SchemeRegistry
    
    Central registry for URI schemes, associates an URI scheme (e.g. http, 
    file, ...) with a Nebula stream class.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"    
#include "threading/criticalsection.h"

//------------------------------------------------------------------------------
namespace IO
{
class SchemeRegistry : public Core::RefCounted
{
    __DeclareClass(SchemeRegistry);
    __DeclareInterfaceSingleton(SchemeRegistry);
public:
    /// constructor
    SchemeRegistry();
    /// destructor
    virtual ~SchemeRegistry();
    
    /// setup the scheme registry (may only be called at app startup by the main thread)
    void Setup();
    /// discard the scheme registry
    void Discard();
    /// return true if the scheme registry is valid
    bool IsValid() const;

    /// associate an uri scheme with a stream class
    void RegisterUriScheme(const Util::String& uriScheme, const Core::Rtti& classRtti);
    /// unregister an uri scheme
    void UnregisterUriScheme(const Util::String& uriScheme);
    /// return true if an uri scheme has been registered
    bool IsUriSchemeRegistered(const Util::String& uriScheme) const;
    /// get the registered stream class for an uri scheme
    const Core::Rtti& GetStreamClassByUriScheme(const Util::String& uriScheme) const;
    /// get an array of all registered schemes
    Util::Array<Util::String> GetAllRegisteredUriSchemes() const;

private:
    /// register standard URI schemes, called by the Setup method
    void SetupStandardSchemes();

    Threading::CriticalSection critSect;
    Util::Dictionary<Util::String, const Core::Rtti*> schemeRegistry;
    bool isValid;
};

} // namespace IO
//------------------------------------------------------------------------------
