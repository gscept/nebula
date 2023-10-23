using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;
using System.Reflection;

namespace Nebula
{
    namespace Game
    {
        public class MsgDispatcher
        {
            public delegate void HandleMessage(in Msg msg);
            private Dictionary<Type, MsgEvent> events = new Dictionary<Type, MsgEvent>();

            public void AttachHandler(HandleMessage handler, Type[] msgTypes)
            {
                if (msgTypes == null)
                    return;
                
                for (int i = 0; i < msgTypes.Length; i++) 
                {
                    if (!events.ContainsKey(msgTypes[i]))
                    {
                        events[msgTypes[i]] = new MsgEvent();
                    }
                    events[msgTypes[i]].OnMessageEvent += handler;
                }
            }

            public void Dispatch<T>(T msg) where T : struct, Msg
            {
                Type type = typeof(T);
                if (events.ContainsKey(type))
                {
                    events[type].Dispatch(msg);
                }
            }

            private class MsgEvent
            {
                public event HandleMessage OnMessageEvent;
                public void Dispatch<T>(T msg) where T : struct, Msg
                {
                    OnMessageEvent?.Invoke((Msg)msg);
                }
            }
        }
    }
}
