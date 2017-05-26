//------------------------------------------------------------------------------
//  win32registrytest.cc
//  (C) 2007 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "win32registrytest.h"
#include "system/nebulasettings.h"

namespace Test
{
__ImplementClass(Test::Win32RegistryTest, 'wrgt', Test::TestCase);

using namespace System;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
Win32RegistryTest::Run()
{   
	const String vendor("gscept");
	const String key("Nebula3Test");

    // write a key
    this->Verify(NebulaSettings::WriteString(vendor, key, "name1", "value1"));
	this->Verify(NebulaSettings::WriteString(vendor, key, "name2", "value2"));

    // check that keys exist
	this->Verify(NebulaSettings::Exists(vendor, key, ""));
	this->Verify(NebulaSettings::Exists(vendor, key, "name1"));
	this->Verify(NebulaSettings::Exists(vendor, key, "name2"));
	this->Verify(!NebulaSettings::Exists(vendor, key, "bla"));

    // read values back
	this->Verify("value1" == NebulaSettings::ReadString(vendor, key, "name1"));
	this->Verify("value2" == NebulaSettings::ReadString(vendor, key, "name2"));

    // delete keys
	this->Verify(NebulaSettings::Delete(vendor, key));
	this->Verify(!NebulaSettings::Exists(vendor, key, ""));
}

} // namespace Test