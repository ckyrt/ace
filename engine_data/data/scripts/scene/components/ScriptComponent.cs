using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ace
{
namespace Core
{
    /// <summary>
    /// Represents a script component that provides lifecycle hooks and event handling for an entity.
    /// </summary>
    public abstract class ScriptComponent : Component
    {
        private string SourceFilePath { get; }
        protected ScriptComponent([CallerFilePath] string file = "")
        {
            SourceFilePath = file;
        }

        /// <summary>
        /// Called when the script is created. Override this method to initialize resources or data.
        /// </summary>
        public virtual void OnCreate()
        {
        }

        /// <summary>
        /// Called when the script starts execution. Override this method to set up logic at the start.
        /// </summary>
        public virtual void OnStart()
        {
        }

        /// <summary>
        /// Called when the script is destroyed. Override this method to clean up resources or data.
        /// </summary>
        public virtual void OnDestroy()
        {
        }

        /// <summary>
        /// Called when another entity enters a sensor attached to this entity.
        /// </summary>
        /// <param name="e">The entity that entered the sensor.</param>
        public virtual void OnSensorEnter(Entity e)
        {
        }

        /// <summary>
        /// Called when another entity exits a sensor attached to this entity.
        /// </summary>
        /// <param name="e">The entity that exited the sensor.</param>
        public virtual void OnSensorExit(Entity e)
        {
        }

        /// <summary>
        /// Called when this entity begins a collision with another entity.
        /// </summary>
        /// <param name="collision">Details of the collision, including the other entity and contact points.</param>
        public virtual void OnCollisionEnter(Collision collision)
        {
        }

        /// <summary>
        /// Called when this entity ends a collision with another entity.
        /// </summary>
        /// <param name="collision">Details of the collision, including the other entity and contact points.</param>
        public virtual void OnCollisionExit(Collision collision)
        {
        }

        /// <summary>
        /// Called on every frame update. Override this method to implement frame-based logic.
        /// </summary>
        public virtual void OnUpdate()
        {
        }

        /// <summary>
        /// Internal method invoked when the script is created. Calls <see cref="OnCreate"/> and subscribes <see cref="OnUpdate"/> to the update system.
        /// </summary>
        private void internal_n2m_on_create()
        {
            OnCreate();
            SystemManager.OnUpdate += OnUpdate;
        }

        /// <summary>
        /// Internal method invoked when the script starts. Calls <see cref="OnStart"/>.
        /// </summary>
        private void internal_n2m_on_start()
        {
            OnStart();
        }

        /// <summary>
        /// Internal method invoked when the script is destroyed. Calls <see cref="OnDestroy"/> and unsubscribes <see cref="OnUpdate"/> from the update system.
        /// </summary>
        private void internal_n2m_on_destroy()
        {
            SystemManager.OnUpdate -= OnUpdate;
            OnDestroy();
        }

        /// <summary>
        /// Internal method invoked when another entity enters a sensor attached to this entity. Calls <see cref="OnSensorEnter"/>.
        /// </summary>
        /// <param name="entity">The entity that entered the sensor.</param>
        private void internal_n2m_on_sensor_enter(Entity entity)
        {
            OnSensorEnter(entity);
        }

        /// <summary>
        /// Internal method invoked when another entity exits a sensor attached to this entity. Calls <see cref="OnSensorExit"/>.
        /// </summary>
        /// <param name="entity">The entity that exited the sensor.</param>
        private void internal_n2m_on_sensor_exit(Entity entity)
        {
            OnSensorExit(entity);
        }

        /// <summary>
        /// Internal method invoked when this entity begins a collision. Converts contact data and calls <see cref="OnCollisionEnter"/>.
        /// </summary>
        /// <param name="entity">The other entity involved in the collision.</param>
        /// <param name="contactData">The serialized contact data for the collision.</param>
        private void internal_n2m_on_collision_enter(Entity entity, byte[] contactData)
        {
            Collision collision = new Collision
            {
                entity = entity,
                contacts = contactData.ToStructArray<ContactPoint>()
            };

            OnCollisionEnter(collision);
        }

        /// <summary>
        /// Internal method invoked when this entity ends a collision. Converts contact data and calls <see cref="OnCollisionExit"/>.
        /// </summary>
        /// <param name="entity">The other entity involved in the collision.</param>
        /// <param name="contactData">The serialized contact data for the collision.</param>
        private void internal_n2m_on_collision_exit(Entity entity, byte[] contactData)
        {
            Collision collision = new Collision
            {
                entity = entity,
                contacts = contactData.ToStructArray<ContactPoint>()
            };

            OnCollisionExit(collision);
        }
    }
}
}
