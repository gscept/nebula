using System.Collections.Generic;

namespace Nebula
{
    namespace Game
    {
        /// <summary>
        /// A property that an ActionQueue runs.
        /// </summary>
        /// <remarks>
        /// <para>
        /// It is active only while it is the front of its entity's queue. The queue does not run the property: it says which
        /// property is at the front, and the entity's activation pass is what activates it. An implementation says
        /// its action is done by answering true to ShouldDeactivate, and the queue moves on once the property is
        /// no longer running.
        /// </para>
        /// <para>
        /// A property that is taken off its entity while it is at the front of the queue leaves the queue when the
        /// queue next looks at it.
        /// </para>
        /// </remarks>
        public class ActionProperty : Property
        {
            private bool isFrontOfQueue;

            /// <summary>
            /// Whether this property is the action at the front of its queue.
            /// </summary>
            public bool IsFrontOfQueue => this.isFrontOfQueue;

            /// <summary>
            /// A property in a queue runs when the queue says it is at the front.
            /// </summary>
            public override bool ShouldActivate()
            {
                return this.isFrontOfQueue;
            }

            /// <summary>
            /// Called by the queue when this property becomes the action at the front of it.
            /// </summary>
            public void BecomeFrontOfQueue()
            {
                this.isFrontOfQueue = true;
                this.OnBecomeFrontOfQueue();
            }

            /// <summary>
            /// Called by the queue when this property is no longer the action at the front of it.
            /// </summary>
            public void RemovedFromQueue()
            {
                this.isFrontOfQueue = false;
                this.OnRemovedFromQueue();
            }

            /// <summary>
            /// Override in subclass for what the property does when it becomes the front of the queue.
            /// </summary>
            public virtual void OnBecomeFrontOfQueue()
            {
                // Override in subclass
            }

            /// <summary>
            /// Override in subclass for what the property does when it is taken out of the queue.
            /// </summary>
            public virtual void OnRemovedFromQueue()
            {
                // Override in subclass
            }
        }
    }
}
