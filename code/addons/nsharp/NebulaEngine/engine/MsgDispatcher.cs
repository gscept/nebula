using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;

using System.Reflection;

namespace Nebula
{
    namespace Game
    {
        public class MsgDispatcher
        {
            private interface IMessageChannel
            {
                void Remove(Property property);
                bool IsEmpty { get; }
            }

            private sealed class MessageChannel<T> : IMessageChannel where T : Msg
            {
                private List<IMessageHandler<T>> handlers = new List<IMessageHandler<T>>();

                public void Add(IMessageHandler<T> handler)
                {
                    this.handlers.Add(handler);
                }

                public void Remove(Property property)
                {
                    for (int i = this.handlers.Count - 1; i >= 0; i--)
                    {
                        if (Object.ReferenceEquals(this.handlers[i], property))
                            this.handlers.RemoveAt(i);
                    }
                }

                public bool IsEmpty { get { return this.handlers.Count == 0; } }

                public void Dispatch(in T message)
                {
                    for (int i = 0; i < this.handlers.Count; i++)
                        this.handlers[i].OnMessage(in message);
                }
            }

            private Dictionary<Type, IMessageChannel> channels;
            private List<Type> emptyChannels;

            public void Register<T>(IMessageHandler<T> handler) where T : Msg
            {
                if (this.channels == null)
                    this.channels = new Dictionary<Type, IMessageChannel>();

                Type messageType = typeof(T);
                IMessageChannel channel;
                if (!this.channels.TryGetValue(messageType, out channel))
                {
                    MessageChannel<T> typedChannel = new MessageChannel<T>();
                    this.channels.Add(messageType, typedChannel);
                    channel = typedChannel;
                }

                ((MessageChannel<T>)channel).Add(handler);
            }

            public void Unregister(Property property)
            {
                if (this.channels == null)
                    return;

                if (this.emptyChannels == null)
                    this.emptyChannels = new List<Type>();
                this.emptyChannels.Clear();

                foreach (KeyValuePair<Type, IMessageChannel> entry in this.channels)
                {
                    entry.Value.Remove(property);
                    if (entry.Value.IsEmpty)
                        this.emptyChannels.Add(entry.Key);
                }

                for (int i = 0; i < this.emptyChannels.Count; i++)
                {
                    this.channels.Remove(this.emptyChannels[i]);
                }
            }

            public void Dispatch<T>(in T message) where T : Msg
            {
                if (this.channels == null)
                    return;

                IMessageChannel channel;
                if (this.channels.TryGetValue(typeof(T), out channel))
                    ((MessageChannel<T>)channel).Dispatch(in message);
            }
        }
    }
}
