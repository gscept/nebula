using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Collections;
using System.Collections.Generic;
using Mathf;
using Nebula;
using Nebula.Game;

namespace Nebula
{
    namespace Game
    {
        public enum FrameEvent
        {
            // Update order
            OnBeginFrame, // Called every frame before rendering (old vis results)
            OnFixedFrame, // Called every couple of frames with a fixed delta frame time. These updates are distributed and might not be called on the same frame for all components (called after OnBeginFrame)
            OnFrame, // Called every frame while rendering (up to date vis results)
            OnEndFrame, // Called after rendering is done

            // Total number of events
            NumEvents
        }

        public class Property
        {
            private bool active = false;

            /// <summary>
            /// Activate or deactivate property
            /// This is not trivial and can potentially be costly
            /// </summary>
            public bool Active
            { 
                get { return active; }
                set
                { 
                    if (active == value)
                    {
                        return;
                    }

                    if (value)
                    {
                        this.OnActivate();
                    }
                    else
                    {
                        this.OnDeactivate();
                    }
                    active = value;
                }
            }

            private Entity entity = null;
            
            public void Destroy()
            {
                if (IsValid && active)
                {
                    this.OnDeactivate();
                    entity = null;
                }
            }

            public Entity Entity
            {
                get
                {
                    return entity;
                }
                set
                {
                    if (entity == null)
                    {
                        entity = value;
                    }
                    else
                    {
                        Debug.Log("Warning: Trying to change owner entity of a property that is already owned!");
                    }
                }
            }
            
            public bool IsValid { get { return entity != null; } }

            /// <summary>
            /// Override in subclass to tell the property manager which functions to call for this class
            /// </summary>
            public virtual Nebula.Game.FrameEvent[] AcceptedEvents()
            {
                return null;
            }

            public virtual System.Type[] AcceptedMessages()
            {
                // Override in subclass
                return null;
            }

            public virtual void OnMessage(in Msg msg)
            {
                // Override in subclass
            }

            public override string ToString()
            {
                if (entity != null)
                {
                    return GetType().ToString() + " -> Entity: " + entity.ToString();
                }

                return GetType().ToString();
            }

            public virtual void OnActivate() { }
            public virtual void OnDeactivate() { }
            public virtual void OnBeginFrame() { }
            public virtual void OnFrame() { }
            public virtual void OnEndFrame() { }
        }
    }
}
