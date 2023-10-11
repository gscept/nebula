using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;

namespace Nebula
{
    namespace Game
    {
        public delegate void HandleMessage(in Msg msg);
        public class MsgPort
        {    
            public HandleMessage OnMessage;
        }
    }
}
