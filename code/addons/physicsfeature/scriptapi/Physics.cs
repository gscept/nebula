using System;
using System.Runtime.InteropServices;

using Nebula;
using Nebula.Game;

namespace PhysicsFeature
{
    /// <summary>
    /// The managed side of the physics feature.
    /// </summary>
    /// <remarks>
    /// <para>
    /// A body is named by the uint the PhysicsActor component holds, which is an id into the native
    /// actor pool. A script can hold an id across a rebuild and the entity it named can be gone by the
    /// time the call comes back, so the calls answer with an invalid value rather than asserting.
    /// </para>
    /// <para>
    /// PhysX only allows reads of a body before simulate() and writes after fetchResults(), which is
    /// what the physics frame event is. The managed phase runs inside that event, after the step, so a
    /// read here sees this step and a write is picked up by the next one. A property that wants this
    /// phase has to list FrameEvent.OnPhysicsFrame in AcceptedEvents.
    /// </para>
    /// </remarks>
    public static class Physics
    {
        public const uint InvalidActorId = 0xFFFFFFFF;
        public const uint InvalidScene = 0xFFFFFFFF;

        // The scene the default world has, resolved on the first use. A script that works in one world
        // asks for the same scene every call, so this is cached
        private static uint defaultScene = InvalidScene;

        /// <summary>
        /// The physics scene the default world has.
        /// </summary>
        public static uint DefaultScene
        {
            get
            {
                if (defaultScene == InvalidScene)
                {
                    defaultScene = Native.GetSceneForWorld(World.DEFAULT_WORLD);
                }

                return defaultScene;
            }
        }

        // ------------------------------------------------------------------------------------------
        // Actor state
        // ------------------------------------------------------------------------------------------

        /// <summary>
        /// Whether the id is in the actor pool. A static, kinematic or dynamic actor is in it, and an
        /// entity that has a PhysicsActor component but has not been built yet is not.
        /// </summary>
        public static bool IsActorValid(uint actorId)
        {
            return Native.IsActorValid(actorId);
        }

        /// <summary>
        /// The mass the body has.
        /// </summary>
        /// <remarks>
        /// This is the mass PhysX resolved, which is not the same as the Rigidbody component's mass: the
        /// component defaults to 0 and the mass is then derived from the shape volumes and the material
        /// densities.
        /// </remarks>
        public static float GetMass(uint actorId)
        {
            return Native.GetMass(actorId);
        }

        public static bool IsSleeping(uint actorId)
        {
            return Native.IsSleeping(actorId);
        }

        public static void WakeUp(uint actorId)
        {
            Native.WakeUp(actorId);
        }

        public static void Sleep(uint actorId)
        {
            Native.Sleep(actorId);
        }

        // ------------------------------------------------------------------------------------------
        // Velocity
        // ------------------------------------------------------------------------------------------

        public static Vector3 GetVelocity(uint actorId)
        {
            return Native.GetVelocity(actorId);
        }

        public static void SetVelocity(uint actorId, Vector3 velocity)
        {
            Native.SetVelocity(actorId, velocity);
        }

        public static Vector3 GetAngularVelocity(uint actorId)
        {
            return Native.GetAngularVelocity(actorId);
        }

        public static void SetAngularVelocity(uint actorId, Vector3 angularVelocity)
        {
            Native.SetAngularVelocity(actorId, angularVelocity);
        }

        // ------------------------------------------------------------------------------------------
        // Forces
        // ------------------------------------------------------------------------------------------

        /// <summary>
        /// Newtons. The force is accumulated by the body and integrated by the next step, so it is not the
        /// same quantity as an impulse.
        /// </summary>
        public static void AddForce(uint actorId, Vector3 force)
        {
            Native.AddForce(actorId, force);
        }

        /// <summary>
        /// Mass times the velocity change. A call here is what a step applies to the body immediately,
        /// which is the quantity a jump or a bounce is usually written in.
        /// </summary>
        public static void AddImpulse(uint actorId, Vector3 impulse)
        {
            Native.AddImpulse(actorId, impulse);
        }

        /// <summary>
        /// Impulse at a point in world space, which is what gives the body angular velocity as well.
        /// </summary>
        public static void AddImpulseAtPoint(uint actorId, Vector3 impulse, Vector3 point)
        {
            Native.AddImpulseAtPoint(actorId, impulse, point);
        }

        public static void AddTorque(uint actorId, Vector3 torque)
        {
            Native.AddTorque(actorId, torque);
        }

        /// <summary>
        /// The physics scene a game world has.
        /// </summary>
        public static uint GetSceneForWorld(uint worldId)
        {
            return Native.GetSceneForWorld(worldId);
        }

        // ------------------------------------------------------------------------------------------
        // Queries
        // ------------------------------------------------------------------------------------------

        /// <summary>
        /// A filter with the values the native struct starts with.
        /// </summary>
        /// <remarks>
        /// A C# struct cannot give its fields default values, so a new QueryFilter() has mask 0 and
        /// excludeActorId 0. Native starts with every group accepted and no actor excluded, which is not
        /// what a fresh struct gives, so build a filter with this rather than with new.
        /// </remarks>
        public static QueryFilter NewQueryFilter(uint group = 0, uint mask = 0xFFFFFFFF, uint excludeActorId = InvalidActorId)
        {
            QueryFilter filter = new QueryFilter();
            filter.group = group;
            filter.mask = mask;
            filter.excludeActorId = excludeActorId;
            return filter;
        }

        /// <summary>
        /// Closest hit along a ray, on the default scene with a filter that accepts everything.
        /// </summary>
        /// <remarks>
        /// The direction has to be normalized. A ground check needs a filter that excludes its own actor,
        /// because the ray starts inside the shape the caller owns.
        /// </remarks>
        public static bool Raycast(Vector3 origin, Vector3 direction, float maxDistance, ref RaycastHit hit)
        {
            QueryFilter filter = NewQueryFilter();
            return Raycast(DefaultScene, origin, direction, maxDistance, ref hit, ref filter);
        }

        public static bool Raycast(Vector3 origin, Vector3 direction, float maxDistance, ref RaycastHit hit, ref QueryFilter filter)
        {
            return Raycast(DefaultScene, origin, direction, maxDistance, ref hit, ref filter);
        }

        public static bool Raycast(uint scene, Vector3 origin, Vector3 direction, float maxDistance, ref RaycastHit hit, ref QueryFilter filter)
        {
            return Native.Raycast(scene, origin, direction, maxDistance, ref hit, ref filter);
        }

        /// <summary>
        /// Closest hit along a segment, on the default scene with a filter that accepts everything.
        /// </summary>
        /// <remarks>
        /// The segment is the direction and the distance, so a caller that has two points does not have to
        /// normalize them first.
        /// </remarks>
        public static bool SegmentCast(Vector3 from, Vector3 to, ref RaycastHit hit)
        {
            QueryFilter filter = NewQueryFilter();
            return SegmentCast(DefaultScene, from, to, ref hit, ref filter);
        }

        public static bool SegmentCast(Vector3 from, Vector3 to, ref RaycastHit hit, ref QueryFilter filter)
        {
            return SegmentCast(DefaultScene, from, to, ref hit, ref filter);
        }

        public static bool SegmentCast(uint scene, Vector3 from, Vector3 to, ref RaycastHit hit, ref QueryFilter filter)
        {
            return Native.SegmentCast(scene, from, to, ref hit, ref filter);
        }

        private static class Native
        {
            [DllImport("__Internal", EntryPoint = "Physics_IsActorValid")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool IsActorValid(uint actorId);

            [DllImport("__Internal", EntryPoint = "Physics_GetMass")]
            public static extern float GetMass(uint actorId);

            [DllImport("__Internal", EntryPoint = "Physics_IsSleeping")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool IsSleeping(uint actorId);

            [DllImport("__Internal", EntryPoint = "Physics_WakeUp")]
            public static extern void WakeUp(uint actorId);

            [DllImport("__Internal", EntryPoint = "Physics_Sleep")]
            public static extern void Sleep(uint actorId);

            [DllImport("__Internal", EntryPoint = "Physics_GetVelocity")]
            public static extern Vector3 GetVelocity(uint actorId);

            [DllImport("__Internal", EntryPoint = "Physics_SetVelocity")]
            public static extern void SetVelocity(uint actorId, Vector3 velocity);

            [DllImport("__Internal", EntryPoint = "Physics_GetAngularVelocity")]
            public static extern Vector3 GetAngularVelocity(uint actorId);

            [DllImport("__Internal", EntryPoint = "Physics_SetAngularVelocity")]
            public static extern void SetAngularVelocity(uint actorId, Vector3 angularVelocity);

            [DllImport("__Internal", EntryPoint = "Physics_AddForce")]
            public static extern void AddForce(uint actorId, Vector3 force);

            [DllImport("__Internal", EntryPoint = "Physics_AddImpulse")]
            public static extern void AddImpulse(uint actorId, Vector3 impulse);

            [DllImport("__Internal", EntryPoint = "Physics_AddImpulseAtPoint")]
            public static extern void AddImpulseAtPoint(uint actorId, Vector3 impulse, Vector3 point);

            [DllImport("__Internal", EntryPoint = "Physics_AddTorque")]
            public static extern void AddTorque(uint actorId, Vector3 torque);

            [DllImport("__Internal", EntryPoint = "Physics_GetSceneForWorld")]
            public static extern uint GetSceneForWorld(uint worldId);

            [DllImport("__Internal", EntryPoint = "Physics_Raycast")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool Raycast(uint scene, Vector3 origin, Vector3 direction, float maxDistance, ref RaycastHit outHit, ref QueryFilter filter);

            [DllImport("__Internal", EntryPoint = "Physics_SegmentCast")]
            [return: MarshalAs(UnmanagedType.I1)]
            public static extern bool SegmentCast(uint scene, Vector3 from, Vector3 to, ref RaycastHit outHit, ref QueryFilter filter);
        }
    }
}
