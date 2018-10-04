#pragma once
//------------------------------------------------------------------------------
/**
    @class Core::Factory

    Provides the central object factory mechanism for Nebula. Classes
    which are derived from RefCounted register themselves automatically
    to the central Factory object through the __DeclareClass and
    __ImplementClass macros.


    (C) 2005 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/array.h"
#include "util/string.h"
#include "util/fourcc.h"
#include "util/dictionary.h"
#include "util/hashtable.h"
#include "core/ptr.h"

//------------------------------------------------------------------------------
namespace Core 
{
class RefCounted;
class Rtti;

//------------------------------------------------------------------------------
class Factory
{
public:
    /// get pointer to singleton instance (cannot use singleton.h!)
    static Factory* Instance();
    /// static instance destruction method
    static void Destroy();

    /// register a RTTI object with the factory (with class name and class fourcc code)
    void Register(const Rtti* rtti, const Util::String& className, const Util::FourCC& classFourCC);
    /// register a RTTI object with the factory (without fourcc code)
    void Register(const Rtti* rtti, const Util::String& className);
    /// check if a class exists by class name
    bool ClassExists(const Util::String& className) const;
    /// check if a class exists by FourCC code
    bool ClassExists(const Util::FourCC classFourCC) const;
    /// get class rtti object by name
    const Rtti* GetClassRtti(const Util::String& className) const;
    /// get class rtti object by fourcc code
    const Rtti* GetClassRtti(const Util::FourCC& classFourCC) const;
    /// create an object by class name
	void* Create(const Util::String& className) const;
    /// create an object by FourCC code
	void* Create(const Util::FourCC classFourCC) const;
	/// create an array of objects by class name
	void* CreateArray(const Util::String& className, SizeT num) const;
	/// create an array object by FourCC code
	void* CreateArray(const Util::FourCC classFourCC, SizeT num) const;

private:
    /// constructor is private
    Factory();
    /// destructor is private
    ~Factory();

    static Factory* Singleton;
    Util::HashTable<Util::String, const Rtti*> nameTable;		// for fast lookup by class name
    Util::Dictionary<Util::FourCC, const Rtti*> fourccTable;	// for fast lookup by fourcc code
};

} // namespace Foundation
//------------------------------------------------------------------------------
