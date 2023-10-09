using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;
using System.Reflection;
using System.Linq;

namespace Nebula
{
    namespace Game
    {
        public sealed class PropertyManager
        {
            private List<Property>[] eventListeners = new List<Property>[(int)Nebula.Game.FrameEvent.NumEvents];

            private static readonly PropertyManager instance = new PropertyManager();
            // Explicit static constructor to tell C# compiler
            // not to mark type as beforefieldinit
            static PropertyManager() { }
            private PropertyManager()
            {
                for (int i = 0; i < this.eventListeners.Length; i++)
                {
                    this.eventListeners[i] = new List<Property>();
                }
            }

            /// <summary>
            /// Returns the property manager singleton instance.
            /// </summary>
            public static PropertyManager Instance
            {
                get
                {
                    return instance;
                }
            }

            /// <summary>
            /// Registers a property to the manager.
            /// </summary>
            /// <remarks>
            /// Developers usually doesn't need to do this manually - it is subsequently called when adding a property to an entity.
            /// </remarks>
            /// <see cref="Nebula.Game.Entity.AddProperty(Property)"/>
            public void RegisterProperty(Property property)
            {
                FrameEvent[] acceptedEvents = property.AcceptedEvents();
                if (acceptedEvents != null)
                {
                    for (int i = 0; i < acceptedEvents.Length; i++)
                    {
                        this.eventListeners[(int)acceptedEvents[i]].Add(property);
                    }
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
                    if (!prop.IsValid)
                    {
                        // Remove invalid properties
                        props.EraseSwap(i);
                        i--; // rerun the same index
                        continue;
                    }
                    if (prop.Active)
                        prop.OnBeginFrame();
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
                    if (!prop.IsValid)
                    {
                        // Remove invalid properties
                        props.EraseSwap(i);
                        i--; // rerun the same index
                        continue;
                    }

                    if (prop.Active)
                        prop.OnFrame();
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
                    if (!prop.IsValid)
                    {
                        // Remove invalid properties
                        props.EraseSwap(i);
                        i--; // rerun the same index
                        continue;
                    }

                    if (prop.Active)
                        prop.OnEndFrame();
                }
            }
        }
    }
}

