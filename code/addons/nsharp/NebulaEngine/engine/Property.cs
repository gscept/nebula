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
            private Entity entity = null;
            
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

            /// <summary>
            /// The entity that owns this property.
            /// </summary>
            /// <remarks>
            /// You cannot change the entity after one has been associated with the property.
            /// </remarks>
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

            /// <summary>
            /// Checks if the property is valid (attached to an entity)
            /// </summary>
            public bool IsValid { get { return entity != null; } }

            /// <summary>
            /// Override in subclass to tell the property manager which methods to call for this class
            /// </summary>
            public virtual Nebula.Game.FrameEvent[] AcceptedEvents()
            {
                return null;
            }

            /// <summary>
            /// Override in subclass
            /// </summary>
            public virtual System.Type[] AcceptedMessages()
            {
                return null;
            }

            /// <summary>
            /// Called when a message is sent to the property's associated entity
            /// </summary>
            public virtual void OnMessage(in Msg msg)
            {
                // Override in subclass
            }

            /// <summary>
            /// String representation of this property
            /// </summary>
            public override string ToString()
            {
                if (entity != null)
                {
                    return GetType().ToString() + " -> Entity: " + entity.ToString();
                }

                return GetType().ToString();
            }

            /// <summary>
            /// Called when the property is attached to an entity, or when Property.Active is changed to true.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// </remarks>
            public virtual void OnActivate() { }

            /// <summary>
            /// Called when the property is detached from an entity, or when Property.Active is changed to false.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// </remarks>
            public virtual void OnDeactivate() { }

            /// <summary>
            /// Called once every frame at the beginning of the frame.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// The property AcceptedEvents must return the Event code for this method to be called at all.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            public virtual void OnBeginFrame() { }

            /// <summary>
            /// Called once every frame
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// The property AcceptedEvents must return the Event code for this method to be called at all.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            public virtual void OnFrame() { }

            /// <summary>
            /// Called once every frame at the end of the frame.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// The property AcceptedEvents must return the Event code for this method to be called at all.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            public virtual void OnEndFrame() { }

            /// <summary>
            /// Destroys the property.
            /// </summary>
            /// <remarks>
            /// You should not call this manually, it is called automatically by destroying an entity, or removing the property
            /// </remarks>
            public void Destroy()
            {
                if (IsValid && active)
                {
                    this.OnDeactivate();
                    entity = null;
                }
            }
        }
    }
}
