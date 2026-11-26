using System.Collections.Generic;

namespace Nebula
{
    namespace Game
    {
        /// <summary>
        /// A queue of actions that an runs one after the other.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The queue should be executed once per frame.
        /// An action that names a property runs for as long as that property is running (until ShouldDeactivate returns true).
        /// </para>
        /// <para>
        /// A queue does not survive a hot reload. The reload walks the plugin's object graph, and the queue is an
        /// engine object held by the entity, so nothing walks it, and the unload detaches every property from
        /// every entity before the reattach, which empties the queue. An action that was running is gone after the
        /// reload, and the property comes back on its entity as an ordinary property.
        /// </para>
        /// </remarks>
        public class ActionQueue
        {
            public delegate void EventCallback(float delta);
            public delegate bool LoopCallback(float delta);

            public enum ActionType
            {
                None,       // nothing is running
                Idle,       // does nothing for a number of seconds
                Event,      // runs its callback once
                Duration,   // runs its callback every frame for a number of seconds
                Property,   // runs a property for as long as that property is running
                Loop        // runs its callback every frame until it answers false
            }

            /// <summary>
            /// One entry of the queue. It is a struct so the queue's storage is one block of actions rather
            /// than an object per action.
            /// </summary>
            public struct Action
            {
                public ActionType type;
                public Entity owner;
                public EventCallback eventCallback;
                public LoopCallback loopCallback;
                public ActionProperty property;
                public float duration;
            }

            private const int DefaultCapacity = 16;

            private readonly List<Action> actions;
            private int head;
            private Action current;
            private bool hasCurrent;

            public ActionQueue(int capacity = DefaultCapacity)
            {
                this.actions = new List<Action>(capacity);
            }

            /// <summary>
            /// The actions that are still waiting. The action that is running is not one of them.
            /// </summary>
            public bool IsEmpty()
            {
                return this.head >= this.actions.Count;
            }

            /// <summary>
            /// The type of the action that is running, or None when nothing is.
            /// </summary>
            public ActionType CurrentType => this.hasCurrent ? this.current.type : ActionType.None;

            /// <summary>
            /// The property the action that is running names, or null when it is not a property action.
            /// </summary>
            public ActionProperty CurrentProperty => this.hasCurrent ? this.current.property : null;

            /// <summary>
            /// Does nothing for seconds.
            /// </summary>
            public void Idle(Entity owner, float seconds)
            {
                this.Enqueue(new Action { type = ActionType.Idle, owner = owner, duration = seconds });
            }

            /// <summary>
            /// Runs the callback once.
            /// </summary>
            public void Event(Entity owner, EventCallback callback)
            {
                this.Enqueue(new Action { type = ActionType.Event, owner = owner, eventCallback = callback });
            }

            /// <summary>
            /// Runs the callback every frame for seconds.
            /// </summary>
            public void Duration(float seconds, Entity owner, EventCallback callback)
            {
                this.Enqueue(new Action { type = ActionType.Duration, owner = owner, eventCallback = callback, duration = seconds });
            }

            /// <summary>
            /// Runs a property for as long as it is running. The property is only active while it is the front of
            /// this queue, which is what an ActionProperty's ShouldActivate answers with.
            /// </summary>
            public void Property(Entity owner, ActionProperty property)
            {
                Debug.Assert(property != null);
                this.Enqueue(new Action { type = ActionType.Property, owner = owner, property = property });
            }

            /// <summary>
            /// Runs the callback every frame until it answers false.
            /// </summary>
            public void Loop(Entity owner, LoopCallback callback)
            {
                this.Enqueue(new Action { type = ActionType.Loop, owner = owner, loopCallback = callback });
            }

            /// <summary>
            /// Loads the first action when nothing is running. Call this after adding an action to a queue that
            /// was empty, or the queue does not start until the next Execute.
            /// </summary>
            public void BeginExecution()
            {
                if (this.hasCurrent)
                {
                    return;
                }

                if (this.TryDequeue(out this.current))
                {
                    this.hasCurrent = true;
                    if (this.current.type == ActionType.Property)
                    {
                        this.current.property.BecomeFrontOfQueue();
                    }
                }
            }

            /// <summary>
            /// Advances one step of the queue.
            /// </summary>
            public void Execute(float delta)
            {
                if (!this.hasCurrent)
                {
                    if (!this.TryDequeue(out this.current))
                    {
                        return;
                    }

                    this.hasCurrent = true;
                    if (this.current.type == ActionType.Property)
                    {
                        this.current.property.BecomeFrontOfQueue();
                    }
                }

                bool newAction = false;

                switch (this.current.type)
                {
                    case ActionType.Idle:
                    {
                        this.current.duration -= delta;
                        if (this.current.duration <= 0.0f)
                        {
                            newAction = true;
                        }
                        break;
                    }
                    case ActionType.Event:
                    {
                        this.current.eventCallback?.Invoke(delta);
                        newAction = true;
                        break;
                    }
                    case ActionType.Duration:
                    {
                        this.current.eventCallback?.Invoke(delta);

                        this.current.duration -= delta;
                        if (this.current.duration <= 0.0f)
                        {
                            newAction = true;
                        }
                        break;
                    }
                    case ActionType.Property:
                    {
                        // The property is run by the entity's activation pass, not here. The action is done when
                        // the property is not running, which is what an implementation says by answering true to
                        // ShouldDeactivate. A property that was taken off its entity is not running either
                        ActionProperty property = this.current.property;
                        if (property == null || !property.IsValid() || !property.Active)
                        {
                            newAction = true;
                        }
                        break;
                    }
                    case ActionType.Loop:
                    {
                        newAction = this.current.loopCallback == null || !this.current.loopCallback.Invoke(delta);
                        break;
                    }
                }

                if (newAction)
                {
                    if (this.current.type == ActionType.Property && this.current.property != null)
                    {
                        this.current.property.RemovedFromQueue();
                    }

                    this.current = default;
                    this.hasCurrent = false;

                    if (this.TryDequeue(out this.current))
                    {
                        this.hasCurrent = true;
                        if (this.current.type == ActionType.Property)
                        {
                            this.current.property.BecomeFrontOfQueue();
                        }
                    }
                }
            }

            /// <summary>
            /// Takes everything out of the queue, including the action that is running.
            /// </summary>
            public void Clear()
            {
                if (this.hasCurrent && this.current.type == ActionType.Property && this.current.property != null)
                {
                    ActionProperty property = this.current.property;
                    if (property.Active)
                    {
                        property.Deactivate();
                    }

                    property.RemovedFromQueue();
                }

                // The entries that were still waiting never ran, but the properties they named have to know they
                // are no longer the front of the queue
                for (int i = this.head; i < this.actions.Count; i++)
                {
                    if (this.actions[i].type == ActionType.Property && this.actions[i].property != null)
                    {
                        this.actions[i].property.RemovedFromQueue();
                    }
                }

                this.current = default;
                this.hasCurrent = false;
                this.actions.Clear();
                this.head = 0;
            }

            private void Enqueue(Action action)
            {
                this.actions.Add(action);
            }

            private bool TryDequeue(out Action action)
            {
                if (this.head >= this.actions.Count)
                {
                    action = default;
                    return false;
                }

                action = this.actions[this.head];
                this.actions[this.head] = default;
                this.head++;

                if (this.head == this.actions.Count)
                {
                    // Everything has been taken, so the list starts over rather than keeping read slots
                    this.actions.Clear();
                    this.head = 0;
                }

                return true;
            }
        }
    }
}
