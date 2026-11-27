using System;
using System.Collections.Generic;

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
                private readonly List<IMessageHandler<T>> handlers = [];

                public void Add(IMessageHandler<T> handler)
                {
                    this.handlers.Add(handler);
                }

                public void Remove(Property property)
                {
                    for (int i = this.handlers.Count - 1; i >= 0; i--)
                    {
                        if (Object.ReferenceEquals(this.handlers[i], property))
                        {
                            this.handlers.RemoveAt(i);
                        }
                    }
                }

                public bool IsEmpty => this.handlers.Count == 0;

                public void Dispatch(in T message)
                {
                    for (int i = 0; i < this.handlers.Count; i++)
                    {
                        // A property in a world that is not simulating is inert, which is the same rule the
                        // update methods have. A handler that is not a property has no world to ask
                        if (this.handlers[i] is Property property && !property.IsSimulating())
                        {
                            continue;
                        }

                        try
                        {
                            this.handlers[i].OnMessage(in message);
                        }
                        catch (Exception exception)
                        {
                            FaultReporter.Report(this.handlers[i], nameof(Property.OnMessage), exception);
                        }
                    }
                }
            }

            private Dictionary<Type, IMessageChannel> channels;
            private List<Type> emptyChannels;

            public void Register<T>(IMessageHandler<T> handler) where T : Msg
            {
                this.channels ??= [];

                Type messageType = typeof(T);
                if (!this.channels.TryGetValue(messageType, out IMessageChannel channel))
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
                {
                    return;
                }

                this.emptyChannels ??= [];
                this.emptyChannels.Clear();

                foreach (KeyValuePair<Type, IMessageChannel> entry in this.channels)
                {
                    entry.Value.Remove(property);
                    if (entry.Value.IsEmpty)
                    {
                        this.emptyChannels.Add(entry.Key);
                    }
                }

                for (int i = 0; i < this.emptyChannels.Count; i++)
                {
                    _ = this.channels.Remove(this.emptyChannels[i]);
                }
            }

            public void Dispatch<T>(in T message) where T : Msg
            {
                if (this.channels == null)
                {
                    return;
                }

                if (this.channels.TryGetValue(typeof(T), out IMessageChannel channel))
                {
                    ((MessageChannel<T>)channel).Dispatch(in message);
                }
            }
        }
    }
}
