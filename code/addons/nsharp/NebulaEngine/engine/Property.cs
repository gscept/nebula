using System;
using System.Runtime.CompilerServices;
using System.Diagnostics.CodeAnalysis;

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

        /// <summary>
        /// A behaviour attached to an entity.
        /// </summary>
        /// <remarks>
        /// A property runs in a world that is simulating, only if ShouldActivate returns true. 
        /// A property can decide for itself when it starts and when it stops.
        /// </remarks>
        public class Property
        {
            // What the property was asked to be, and what it actually is. They are not the same thing,
            // because a property only runs in a world that is simulating: setting Active in an editor
            // records the intent, and OnActivate happens when that world starts simulating
            private bool requestedActive;
            private bool active;
            private Entity entity;
            private FrameEvent[] acceptedEvents;
            private bool acceptedEventsCached;

            /// <summary>
            /// Activate or deactivate property
            /// This is not trivial and can potentially be costly
            /// </summary>
            /// <remarks>
            /// The value is whether the property is really running, which is the result of OnActivate having
            /// been called. When the world is not simulating the setter only records the intent, and the
            /// activation happens when the world starts simulating.
            /// </remarks>
            public bool Active
            {
                get { return this.active; }
                set
                {
                    if (value)
                    {
                        if (this.active)
                        {
                            return;
                        }

                        this.requestedActive = true;

                        if (this.IsSimulating() && this.ShouldActivate())
                        {
                            this.Activate();
                        }
                    }
                    else
                    {
                        if (!this.requestedActive && !this.active)
                        {
                            return;
                        }

                        this.requestedActive = false;
                        this.Deactivate();
                    }
                }
            }

            /// <summary>
            /// What the property was asked to be, which is what a world that starts simulating activates
            /// its properties from
            /// </summary>
            internal bool RequestedActive => this.requestedActive;

            /// <summary>
            /// Override in subclass to tell the entity whether this property should be running.
            /// </summary>
            /// <remarks>
            /// Asked once per frame, for a property that is not running, in a world that is simulating. The
            /// default is true, so a property that does not override it is activated immediately
            /// </remarks>
            public virtual bool ShouldActivate()
            {
                return true;
            }

            /// <summary>
            /// Override in subclass to tell the entity that this property should stop running.
            /// </summary>
            /// <remarks>
            /// Asked once per frame, for a property that is running, in a world that is simulating. The default
            /// is false, so a property that does not override it is never deactivated by itself.
            /// </remarks>
            public virtual bool ShouldDeactivate()
            {
                return false;
            }

            /// <summary>
            /// Whether the property's world is simulating
            /// </summary>
            internal bool IsSimulating()
            {
                return this.entity != null && this.entity.World.IsSimulating();
            }

            /// <summary>
            /// Runs OnActivate and records that it has run. Called by the setter when the world is
            /// simulating, and by the world when its mode changes to simulating
            /// </summary>
            internal void Activate()
            {
                if (this.active)
                {
                    return;
                }

                this.OnActivate();
                this.active = true;
            }

            /// <summary>
            /// Runs OnDeactivate for a property that was activated. A property that was never activated has
            /// nothing to deactivate
            /// </summary>
            internal void Deactivate()
            {
                if (!this.active)
                {
                    return;
                }

                this.OnDeactivate();
                this.active = false;
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
                    return this.entity;
                }
                set
                {
                    if (Object.ReferenceEquals(this.entity, value))
                    {
                        return;
                    }

                    if (this.entity == null)
                    {
                        this.entity = value;
                    }
                    else
                    {
                        Debug.Log("Warning: Trying to change owner entity of a property that is already owned!");
                    }
                }
            }

            /// <summary>
            /// Override in subclass to tell the property manager which methods to call for this class
            /// </summary>
            public virtual Nebula.Game.FrameEvent[] AcceptedEvents()
            {
                return null;
            }

            internal FrameEvent[] GetAcceptedEvents()
            {
                if (!this.acceptedEventsCached)
                {
                    this.acceptedEvents = this.AcceptedEvents();
                    this.acceptedEventsCached = true;
                }
                return this.acceptedEvents;
            }

            /// <summary>
            /// The time source this property's fixed events run on.
            /// </summary>
            /// <remarks>
            /// Override in subclass to run on a fixed source of its own, such as the physics one. The default
            /// is the game logic source.
            /// </remarks>
            public virtual uint FixedTimeSource()
            {
                return TimeSourceHash.Gameplay;
            }

            protected virtual void RegisterMessages() { }

            internal void AttachMessages()
            {
                this.RegisterMessages();
            }

            protected void RegisterMessage<T>() where T : Msg
            {
                if (this.entity != null)
                {
                    this.entity.RegisterMessage<T>((IMessageHandler<T>)this);
                }
            }

            internal void DetachMessages()
            {
                if (this.entity != null)
                {
                    this.entity.UnregisterMessages(this);
                }
            }

            /// <summary>
            /// Called when a message is sent to the property's associated entity
            /// </summary>
            /// <remarks>
            /// Only called for a property in a world that is simulating, which is the same rule the update
            /// methods have
            /// </remarks>
            public virtual void OnMessage(in Msg msg)
            {
                // Override in subclass
            }

            /// <summary>
            /// String representation of this property
            /// </summary>
            public override string ToString()
            {
                return this.entity != null ? this.GetType().ToString() + " -> Entity: " + this.entity.ToString() : this.GetType().ToString();
            }

            /// <summary>
            /// Called when the property is attached to an entity, or when Property.Active is changed to true.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// This only happens in a world that is simulating. A property in a world the editor is only
            /// authoring is not running, so its activation waits for the world to start simulating.
            /// </remarks>
            public virtual void OnActivate() { }

            /// <summary>
            /// Called when the property is detached from an entity, or when Property.Active is changed to false.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// Only called for a property that was activated, and when its world stops simulating.
            /// </remarks>
            public virtual void OnDeactivate() { }

            /// <summary>
            /// Called once every frame at the beginning of the frame.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// The property AcceptedEvents must return the Event code for this method to be called at all,
            /// and the property has to be active, which only happens in a world that is simulating.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            public virtual void OnBeginFrame() { }

            /// <summary>
            /// Called once for every fixed step the time source has available this frame.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// The property AcceptedEvents must return the Event code for this method to be called at all,
            /// and the property has to be active, which only happens in a world that is simulating. A source
            /// with no fixed step gives one call per frame.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            /// <seealso cref="FixedTimeSource()"/>
            public virtual void OnFixedFrame() { }

            /// <summary>
            /// Called once every frame
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// The property AcceptedEvents must return the Event code for this method to be called at all,
            /// and the property has to be active, which only happens in a world that is simulating.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            public virtual void OnFrame() { }

            /// <summary>
            /// Called once every frame at the end of the frame.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// The property AcceptedEvents must return the Event code for this method to be called at all,
            /// and the property has to be active, which only happens in a world that is simulating.
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
                if (this.IsValid())
                {
                    this.Deactivate();
                }
                this.requestedActive = false;
                this.entity = null;
            }
        }

        //--------------------------------------------------------------------------
        /**
        */
        public static class PropertyExtensions
        {
            //--------------------------------------------------------------------------
            /** <summary>
                Checks if the property is valid (attached to an entity)

                This is safe to call even if the property == null.
                </summary>
            */
#nullable enable
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            public static bool IsValid([NotNullWhen(true)] this Property? prop)
            {
                return prop != null && prop.Entity.IsValid();
            }
        }
    }
}
