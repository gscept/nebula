using System.Collections.Generic;

namespace Nebula
{
    namespace Game
    {
        public sealed class PropertyManager
        {
            private readonly List<Property>[] eventListeners = new List<Property>[(int)Nebula.Game.FrameEvent.NumEvents];

            private static readonly PropertyManager instance = new PropertyManager();

            // Explicit static constructor to tell C# compiler
            // not to mark type as beforefieldinit
            static PropertyManager()
            {
            }

            private PropertyManager()
            {
                for (int i = 0; i < this.eventListeners.Length; i++)
                {
                    this.eventListeners[i] = [];
                }
            }

            /// <summary>
            /// Returns the property manager singleton instance.
            /// </summary>
            public static PropertyManager Instance => instance;

            /// <summary>
            /// Registers a property to the manager.
            /// </summary>
            /// <remarks>
            /// Developers usually doesn't need to do this manually - it is subsequently called when adding a property to an entity.
            /// </remarks>
            /// <see cref="Nebula.Game.Entity.AddProperty(Property)"/>
            public void RegisterProperty(Property property)
            {
                FrameEvent[] acceptedEvents = property.GetAcceptedEvents();
                if (acceptedEvents != null)
                {
                    for (int i = 0; i < acceptedEvents.Length; i++)
                    {
                        this.eventListeners[(int)acceptedEvents[i]].Add(property);
                    }
                }
            }

            public void UnregisterProperty(Property property)
            {
                FrameEvent[] acceptedEvents = property.GetAcceptedEvents();
                if (acceptedEvents == null)
                {
                    return;
                }

                for (int i = 0; i < acceptedEvents.Length; i++)
                {
                    _ = this.eventListeners[(int)acceptedEvents[i]].Remove(property);
                }
            }

            internal void Clear()
            {
                for (int i = 0; i < this.eventListeners.Length; i++)
                {
                    this.eventListeners[i].Clear();
                }
            }

            /// <summary>
            /// Calls all eventlistener properties that has subscribed to the OnBeginFrame event
            /// </summary>
            public void OnBeginFrame()
            {
                List<Property> props = this.eventListeners[(int)FrameEvent.OnBeginFrame];
                for (int i = 0; i < props.Count; i++)
                {
                    Property prop = props[i];
                    if (!prop.IsValid())
                    {
                        // Remove invalid properties
                        props.EraseSwap(i);
                        i--; // rerun the same index
                        continue;
                    }

                    // An entity can be valid without having an instance, because an editor command can be
                    // holding its id on the undo stack. The property stays in the list, because the instance
                    // can come back with a redo, but nothing that reads the world runs against an entity that
                    // has nothing in it
                    if (!prop.Entity.HasInstance())
                    {
                        continue;
                    }

                    // Active is only true in a world that is simulating, so this is the gate that keeps the
                    // properties of a world the editor is only authoring from running
                    if (prop.Active)
                    {
                        prop.OnBeginFrame();
                    }
                }
            }

            /// <summary>
            /// Calls all eventlistener properties that has subscribed to the OnFixedFrame event, once for
            /// every fixed step their time source has available this frame
            /// </summary>
            public void OnFixedFrame()
            {
                List<Property> props = this.eventListeners[(int)FrameEvent.OnFixedFrame];
                for (int i = 0; i < props.Count; i++)
                {
                    Property prop = props[i];
                    if (!prop.IsValid())
                    {
                        // Remove invalid properties
                        props.EraseSwap(i);
                        i--; // rerun the same index
                        continue;
                    }

                    // The same guard as the other events
                    if (!prop.Entity.HasInstance())
                    {
                        continue;
                    }

                    if (!prop.Active)
                    {
                        continue;
                    }

                    TimeSource source = TimeManager.Instance.Get(prop.FixedTimeSource());
                    if (source == null)
                    {
                        continue;
                    }

                    // The same rule the frame pipeline has: an event with no fixed step runs once per frame,
                    // and a fixed event runs once for every step the source has available
                    int steps = source.FixedFrameTime > 0.0 ? source.AvailableSteps : 1;
                    for (int step = 0; step < steps; step++)
                    {
                        prop.OnFixedFrame();
                    }
                }
            }

            /// <summary>
            /// Calls all eventlistener properties that has subscribed to the OnFrame event
            /// </summary>
            public void OnFrame()
            {
                List<Property> props = this.eventListeners[(int)FrameEvent.OnFrame];
                for (int i = 0; i < props.Count; i++)
                {
                    Property prop = props[i];
                    if (!prop.IsValid())
                    {
                        // Remove invalid properties
                        props.EraseSwap(i);
                        i--; // rerun the same index
                        continue;
                    }

                    // The same guard as the other events: a valid entity without an instance is an id
                    // something is holding, and a property that reads the world has nothing to read
                    if (!prop.Entity.HasInstance())
                    {
                        continue;
                    }

                    // The same gate as the other events
                    if (prop.Active)
                    {
                        prop.OnFrame();
                    }
                }
            }

            /// <summary>
            /// Calls all eventlistener properties that has subscribed to the OnEndFrame event
            /// </summary>
            public void OnEndFrame()
            {
                List<Property> props = this.eventListeners[(int)FrameEvent.OnEndFrame];
                for (int i = 0; i < props.Count; i++)
                {
                    Property prop = props[i];
                    if (!prop.IsValid())
                    {
                        // Remove invalid properties
                        props.EraseSwap(i);
                        i--; // rerun the same index
                        continue;
                    }

                    // The same guard as the other events
                    if (!prop.Entity.HasInstance())
                    {
                        continue;
                    }

                    // The same gate as the other events
                    if (prop.Active)
                    {
                        prop.OnEndFrame();
                    }
                }
            }
        }
    }
}
