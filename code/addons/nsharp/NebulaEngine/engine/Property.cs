using System;
using System.Collections.Generic;
using System.Reflection;
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
            OnPhysicsFrame, // Called once for every step the physics time source has available, from inside the physics frame event after the step (called after OnFrame)
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

            // The event methods in the order the FrameEvent enum lists them, so the index of a name is the
            // event it belongs to. These are the names the detection looks up on a property type
            private static readonly string[] eventMethodNames =
            [
                nameof(OnBeginFrame),
                nameof(OnFixedFrame),
                nameof(OnFrame),
                nameof(OnPhysicsFrame),
                nameof(OnEndFrame)
            ];

            // What a property type overrides, resolved the first time a property of that type is registered
            // and shared by every property of it afterwards. The keys are weak because a type from a retired
            // hot reload assembly has to be collectible, and a strong cache would root it
            private static readonly ConditionalWeakTable<Type, FrameEvent[]> detectedEvents = new();

            /// <summary>
            /// Activate or deactivate property
            /// This is not trivial and can potentially be costly
            /// </summary>
            /// <remarks>
            /// The value is whether the property is really running, which is the result of OnActivate having
            /// been called.
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
            /// Runs OnActivate and records that it has run. Called by the entity's activation pass when the
            /// world is simulating, and by the world when its mode changes to simulating
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
            /// <remarks>
            /// The default is the event methods this class overrides, resolved once per type and shared by
            /// every property of it. An override here is only needed to name an event the class has no
            /// method for, or to take away one of the events the detection finds.
            /// </remarks>
            public virtual Nebula.Game.FrameEvent[] AcceptedEvents()
            {
                return DetectEvents(this.GetType());
            }

            /// <summary>
            /// The events this property is registered for, which is what the property manager calls it for
            /// </summary>
            /// <remarks>
            /// Asked once per property, at registration, and not on the frame path. A type that has not been
            /// seen before is the only time the detection runs.
            /// </remarks>
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
            /// The event methods a property type overrides
            /// </summary>
            /// <remarks>
            /// A method declared somewhere below Property is an override the class made, and an inherited
            /// override is still the one the first class in the chain made. A method that only hides the
            /// base one is not a virtual slot, so the call still goes to Property and it does not count.
            /// </remarks>
            private static FrameEvent[] DetectEvents(Type type)
            {
                if (detectedEvents.TryGetValue(type, out FrameEvent[] cached))
                {
                    return cached;
                }

                List<FrameEvent> events = [];
                for (int i = 0; i < eventMethodNames.Length; i++)
                {
                    MethodInfo method = type.GetMethod(eventMethodNames[i]);
                    if (method != null && method.IsVirtual && method.DeclaringType != typeof(Property))
                    {
                        events.Add((FrameEvent)i);
                    }
                }

                FrameEvent[] detected = events.ToArray();
                detectedEvents.Add(type, detected);
                return detected;
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
            /// If this method is overridden, it will be called automatically if the property is active.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            public virtual void OnBeginFrame() { }

            /// <summary>
            /// Called once for every fixed step the time source has available this frame.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// If this method is overridden, it will be called automatically if the property is active.
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
            /// If this method is overridden, it will be called automatically if the property is active.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            public virtual void OnFrame() { }

            /// <summary>
            /// Called once for every step the physics time source has available this frame.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// If this method is overridden, it will be called automatically if the property is active.
            /// This is called from inside the physics frame event, after the step has been fetched, which is
            /// the point where the physics engine allows a body to be read. A write here is picked up by the
            /// next step. The event only exists when the physics feature is linked, so a property that
            /// subscribes to it in a project without that feature is never called.
            /// </remarks>
            /// <seealso cref="AcceptedEvents()"/>
            public virtual void OnPhysicsFrame() { }

            /// <summary>
            /// Called once every frame at the end of the frame.
            /// </summary>
            /// <remarks>
            /// Override in subclass.
            /// If this method is overridden, it will be called automatically if the property is active.
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
