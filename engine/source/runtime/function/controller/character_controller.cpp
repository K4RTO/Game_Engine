#include "runtime/function/controller/character_controller.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/framework/component/motor/motor_component.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/physics/physics_scene.h"

namespace Piccolo
{
    CharacterController::CharacterController(const Capsule& capsule) : m_capsule(capsule)
    {
        m_rigidbody_shape                                    = RigidBodyShape();
        m_rigidbody_shape.m_geometry                         = PICCOLO_REFLECTION_NEW(Capsule);
        *static_cast<Capsule*>(m_rigidbody_shape.m_geometry) = m_capsule;

        m_rigidbody_shape.m_type = RigidBodyShapeType::capsule;

        Quaternion orientation;
        orientation.fromAngleAxis(Radian(Degree(90.f)), Vector3::UNIT_X);

        m_rigidbody_shape.m_local_transform =
            Transform(Vector3(0, 0, capsule.m_half_height + capsule.m_radius), orientation, Vector3::UNIT_SCALE);
    }

    Vector3 CharacterController::move(const Vector3& current_position, const Vector3& displacement)
    {
        std::shared_ptr<PhysicsScene> physics_scene = g_runtime_global_context.m_world_manager->getCurrentActivePhysicsScene().lock();
        ASSERT(physics_scene);

        Vector3 final_position = current_position;
        Vector3 remaining_displacement = displacement;

        // Vertical pass
        Vector3 vertical_displacement = Vector3(0, 0, remaining_displacement.z);
        std::vector<PhysicsHitInfo> hits;
        if (physics_scene->sweep(m_rigidbody_shape, 
                                Transform(final_position, Quaternion::IDENTITY, Vector3::UNIT_SCALE).getMatrix(),
                                vertical_displacement.normalisedCopy(),
                                vertical_displacement.length(),
                                hits))
        {
            final_position += hits[0].hit_distance * vertical_displacement.normalisedCopy();
            m_is_touch_ground = (hits[0].hit_normal.z > 0.7f);
            
            // If we hit a ceiling while jumping, set vertical velocity to zero
            if (hits[0].hit_normal.z < -0.7f && vertical_displacement.z > 0)
            {
                remaining_displacement.z = 0;
            }
        }
        else
        {
            final_position += vertical_displacement;
            m_is_touch_ground = false;
        }

        // Horizontal pass
        Vector3 horizontal_displacement = Vector3(remaining_displacement.x, remaining_displacement.y, 0);
        hits.clear();
        if (physics_scene->sweep(m_rigidbody_shape,
                                Transform(final_position, Quaternion::IDENTITY, Vector3::UNIT_SCALE).getMatrix(),
                                horizontal_displacement.normalisedCopy(),
                                horizontal_displacement.length(),
                                hits))
        {
            Vector3 slide_plane_normal = hits[0].hit_normal;
            Vector3 slide_vector = horizontal_displacement - slide_plane_normal * horizontal_displacement.dotProduct(slide_plane_normal);
            
            final_position += hits[0].hit_distance * horizontal_displacement.normalisedCopy();
            
            // Try to slide along the wall
            if (slide_vector.length() > 0.01f)
            {
                hits.clear();
                if (!physics_scene->sweep(m_rigidbody_shape,
                                        Transform(final_position, Quaternion::IDENTITY, Vector3::UNIT_SCALE).getMatrix(),
                                        slide_vector.normalisedCopy(),
                                        slide_vector.length(),
                                        hits))
                {
                    final_position += slide_vector;
                }
                else
                {
                    final_position += hits[0].hit_distance * slide_vector.normalisedCopy();
                }
            }
        }
        else
        {
            final_position += horizontal_displacement;
        }
        

        return final_position;
    }
} // namespace Piccolo