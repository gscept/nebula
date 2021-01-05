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
    const String key("NebulaTest");

    // write a key
    VERIFY(NebulaSettings::WriteString(vendor, key, "name1", "value1"));
    VERIFY(NebulaSettings::WriteString(vendor, key, "name2", "value2"));

    // check that keys exist
    VERIFY(NebulaSettings::Exists(vendor, key, ""));
    VERIFY(NebulaSettings::Exists(vendor, key, "name1"));
    VERIFY(NebulaSettings::Exists(vendor, key, "name2"));
    VERIFY(!NebulaSettings::Exists(vendor, key, "bla"));

    // read values back
    VERIFY("value1" == NebulaSettings::ReadString(vendor, key, "name1"));
    VERIFY("value2" == NebulaSettings::ReadString(vendor, key, "name2"));

    // delete keys
    VERIFY(NebulaSettings::Delete(vendor, key));
    VERIFY(!NebulaSettings::Exists(vendor, key, ""));
}

} // namespace Test