//------------------------------------------------------------------------------
//  schemeregistry.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/schemeregistry.h"
#include "io/stream.h"
#include "io/filestream.h"
#include "http/httpstream.h"
#include "http/httpnzstream.h"
#include "safefilestream.h"

namespace IO
{
__ImplementClass(IO::SchemeRegistry, 'SCRG', Core::RefCounted);
__ImplementInterfaceSingleton(IO::SchemeRegistry);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
SchemeRegistry::SchemeRegistry() :
    isValid(false)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
SchemeRegistry::~SchemeRegistry()
{
    if (this->IsValid())
    {
        this->Discard();
    }
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
SchemeRegistry::Setup()
{
    this->critSect.Enter();

    n_assert(!this->IsValid());
    this->isValid = true;
    this->SetupStandardSchemes();
    
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
SchemeRegistry::Discard()
{
    this->critSect.Enter();

    n_assert(this->IsValid());
    this->isValid = false;
    this->schemeRegistry.Clear();

    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
bool
SchemeRegistry::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
    Associates an URI scheme with a stream class. If the same URI
    scheme is already registered, the old association will be overwritten.
*/
void
SchemeRegistry::RegisterUriScheme(const String& uriScheme, const Core::Rtti& classRtti)
{
    this->critSect.Enter();

    n_assert(classRtti.IsDerivedFrom(Stream::RTTI));
    n_assert(uriScheme.IsValid());
    if (this->schemeRegistry.Contains(uriScheme))
    {
        this->UnregisterUriScheme(uriScheme);
    }
    this->schemeRegistry.Add(uriScheme, &classRtti);

    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
SchemeRegistry::UnregisterUriScheme(const String& uriScheme)
{
    this->critSect.Enter();

    n_assert(uriScheme.IsValid());
    n_assert(this->schemeRegistry.Contains(uriScheme));
    this->schemeRegistry.Erase(uriScheme);

    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
bool
SchemeRegistry::IsUriSchemeRegistered(const String& uriScheme) const
{
    this->critSect.Enter();
    n_assert(uriScheme.IsValid());
    bool result = this->schemeRegistry.Contains(uriScheme);
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
const Core::Rtti&
SchemeRegistry::GetStreamClassByUriScheme(const String& uriScheme) const
{
    this->critSect.Enter();
    n_assert(uriScheme.IsValid());
    const Core::Rtti& result = *(this->schemeRegistry[uriScheme]);
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
Array<String> 
SchemeRegistry::GetAllRegisteredUriSchemes() const
{
    this->critSect.Enter();
    Array<String> result = this->schemeRegistry.KeysAsArray();
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
    Registers the standard URI schemes.
    NOTE: private method, thus no critical section protection is necessary!
*/
void
SchemeRegistry::SetupStandardSchemes()
{
    this->RegisterUriScheme("file", FileStream::RTTI);
	this->RegisterUriScheme("safefile", SafeFileStream::RTTI);

#if __NEBULA_HTTP_FILESYSTEM__
    this->RegisterUriScheme("http", Http::HttpStream::RTTI);
    this->RegisterUriScheme("httpnz", Http::HttpNzStream::RTTI);
#endif
}

} // namespace IO
