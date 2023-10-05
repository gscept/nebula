using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;

// Example of imported, "unmanaged" component
[NativeCppClass]
struct AudioEmitter
{
    uint emitterId;
    bool autoplay;
    bool loop;
}
