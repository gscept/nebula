using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;

namespace Nebula
{
    namespace Game
    {
        public interface Msg
        {
            // Empty
        }

        public interface IMessageHandler<T> where T : Msg
        {
            void OnMessage(in T message);
        }
    }
}
