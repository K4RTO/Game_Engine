#include "runtime/function/animation/animation_system.h"
#include "runtime/core/math/vector3.h"
#include "runtime/core/math/quaternion.h"

#include "runtime/function/animation/animation_loader.h"
#include "runtime/function/animation/skeleton.h"
#include "runtime/function/animation/utilities.h"

namespace Piccolo
{
    std::map<std::string, std::shared_ptr<SkeletonData>>  AnimationManager::m_skeleton_definition_cache;
    std::map<std::string, std::shared_ptr<AnimationClip>> AnimationManager::m_animation_data_cache;
    std::map<std::string, std::shared_ptr<AnimSkelMap>>   AnimationManager::m_animation_skeleton_map_cache;
    std::map<std::string, std::shared_ptr<BoneBlendMask>> AnimationManager::m_skeleton_mask_cache;

    std::shared_ptr<SkeletonData> AnimationManager::tryLoadSkeleton(std::string file_path)
    {
        std::shared_ptr<SkeletonData> res;
        AnimationLoader               loader;
        auto                          found = m_skeleton_definition_cache.find(file_path);
        if (found == m_skeleton_definition_cache.end())
        {
            res = loader.loadSkeletonData(file_path);
            m_skeleton_definition_cache.emplace(file_path, res);
        }
        else
        {
            res = found->second;
        }
        return res;
    }

    std::shared_ptr<AnimationClip> AnimationManager::tryLoadAnimation(std::string file_path)
    {
        std::shared_ptr<AnimationClip> res;
        AnimationLoader                loader;
        auto                           found = m_animation_data_cache.find(file_path);
        if (found == m_animation_data_cache.end())
        {
            res = loader.loadAnimationClipData(file_path);
            m_animation_data_cache.emplace(file_path, res);
        }
        else
        {
            res = found->second;
        }
        return res;
    }

    std::shared_ptr<AnimSkelMap> AnimationManager::tryLoadAnimationSkeletonMap(std::string file_path)
    {
        std::shared_ptr<AnimSkelMap> res;
        AnimationLoader              loader;
        auto                         found = m_animation_skeleton_map_cache.find(file_path);
        if (found == m_animation_skeleton_map_cache.end())
        {
            res = loader.loadAnimSkelMap(file_path);
            m_animation_skeleton_map_cache.emplace(file_path, res);
        }
        else
        {
            res = found->second;
        }
        return res;
    }

    std::shared_ptr<BoneBlendMask> AnimationManager::tryLoadSkeletonMask(std::string file_path)
    {
        std::shared_ptr<BoneBlendMask> res;
        AnimationLoader                loader;
        auto                           found = m_skeleton_mask_cache.find(file_path);
        if (found == m_skeleton_mask_cache.end())
        {
            res = loader.loadSkeletonMask(file_path);
            m_skeleton_mask_cache.emplace(file_path, res);
        }
        else
        {
            res = found->second;
        }
        return res;
    }

    BlendStateWithClipData AnimationManager::getBlendStateWithClipData(const BlendState& blend_state)
    {
        for (auto animation_file_path : blend_state.blend_clip_file_path)
        {
            tryLoadAnimation(animation_file_path);
        }
        for (auto anim_skel_map_path : blend_state.blend_anim_skel_map_path)
        {
            tryLoadAnimationSkeletonMap(anim_skel_map_path);
        }
        for (auto skeleton_mask_path : blend_state.blend_mask_file_path)
        {
            tryLoadSkeletonMask(skeleton_mask_path);
        }

        BlendStateWithClipData blend_state_with_clip_data;
        blend_state_with_clip_data.clip_count  = blend_state.clip_count;
        blend_state_with_clip_data.blend_ratio = blend_state.blend_ratio;
        for (const auto& iter : blend_state.blend_clip_file_path)
        {
            blend_state_with_clip_data.blend_clip.push_back(*m_animation_data_cache[iter]);
        }
        for (const auto& iter : blend_state.blend_anim_skel_map_path)
        {
            blend_state_with_clip_data.blend_anim_skel_map.push_back(*m_animation_skeleton_map_cache[iter]);
        }
        std::vector<std::shared_ptr<BoneBlendMask>> blend_masks;
        for (auto& iter : blend_state.blend_mask_file_path)
        {
            blend_masks.push_back(m_skeleton_mask_cache[iter]);
            tryLoadAnimationSkeletonMap(m_skeleton_mask_cache[iter]->skeleton_file_path);
        }
        size_t skeleton_bone_count = m_skeleton_definition_cache[blend_masks[0]->skeleton_file_path]->bones_map.size();
        blend_state_with_clip_data.blend_weight.resize(blend_state.clip_count);
        for (size_t clip_index = 0; clip_index < blend_state.clip_count; clip_index++)
        {
            blend_state_with_clip_data.blend_weight[clip_index].blend_weight.resize(skeleton_bone_count);
        }
        for (size_t bone_index = 0; bone_index < skeleton_bone_count; bone_index++)
        {
            float sum_weight = 0;
            for (size_t clip_index = 0; clip_index < blend_state.clip_count; clip_index++)
            {
                if (blend_masks[clip_index]->enabled[bone_index])
                {
                    sum_weight += blend_state.blend_weight[clip_index];
                }
            }
            if (fabs(sum_weight) < 0.0001f)
            {
                // LOG_ERROR
            }
            for (size_t clip_index = 0; clip_index < blend_state.clip_count; clip_index++)
            {
                if (blend_masks[clip_index]->enabled[bone_index])
                {
                    blend_state_with_clip_data.blend_weight[clip_index].blend_weight[bone_index] =
                        blend_state.blend_weight[clip_index] / sum_weight;
                }
                else
                {
                    blend_state_with_clip_data.blend_weight[clip_index].blend_weight[bone_index] = 0;
                }
            }
        }
        return blend_state_with_clip_data;
    }

    void AnimationManager::blendPoses(const AnimationPose& pose1, const AnimationPose& pose2, float blendFactor, AnimationPose& outPose)
    {
        outPose.m_bone_poses.resize(pose1.m_bone_poses.size());
        outPose.m_weight.blend_weight.resize(pose1.m_weight.blend_weight.size());

        for (size_t i = 0; i < pose1.m_bone_poses.size(); ++i)
        {
            const auto& trans1 = pose1.m_bone_poses[i];
            const auto& trans2 = pose2.m_bone_poses[i];

            outPose.m_bone_poses[i].m_position = Vector3::lerp(trans1.m_position, trans2.m_position, blendFactor);
            outPose.m_bone_poses[i].m_scale = Vector3::lerp(trans1.m_scale, trans2.m_scale, blendFactor);
            outPose.m_bone_poses[i].m_rotation = Quaternion::sLerp(blendFactor, trans1.m_rotation, trans2.m_rotation, true);

            outPose.m_weight.blend_weight[i] = (1 - blendFactor) * pose1.m_weight.blend_weight[i] + blendFactor * pose2.m_weight.blend_weight[i];
        }
    }

    bool AnimationManager::updateAnimationFSM(const std::map<std::string, bool>& signals)
    {
        static AnimationFSM::States currentState = AnimationFSM::States::_idle;
        AnimationFSM::States newState = AnimationFSM::updateState(signals);

        if (newState != currentState)
        {
            currentState = newState;
            return true;
        }
        return false;
    }

    AnimationFSM::States AnimationFSM::updateState(const std::map<std::string, bool>& signals)
    {
        static States m_state = States::_idle;
        States last_state = m_state;
        bool is_clip_finish = tryGetBool(signals, "clip_finish", false);
        bool is_jumping = tryGetBool(signals, "jumping", false);
        float speed = tryGetFloat(signals, "speed", 0.0f);
        bool is_moving = speed > 0.01f;

        switch (m_state)
        {
        case States::_idle:
            if (is_jumping)
                m_state = States::_jump_start_from_idle;
            else if (is_moving)
                m_state = States::_walk_start;
            break;
        case States::_walk_start:
            if (is_clip_finish)
                m_state = States::_walk_run;
            break;
        case States::_walk_run:
            if (is_jumping)
                m_state = States::_jump_start_from_walk_run;
            else if (!is_moving)
                m_state = States::_walk_stop;
            break;
        case States::_walk_stop:
            if (is_clip_finish)
                m_state = States::_idle;
            break;
        case States::_jump_start_from_idle:
        case States::_jump_start_from_walk_run:
            if (is_clip_finish)
                m_state = is_jumping ? (m_state == States::_jump_start_from_idle ? States::_jump_loop_from_idle : States::_jump_loop_from_walk_run) : States::_idle;
            break;
        case States::_jump_loop_from_idle:
        case States::_jump_loop_from_walk_run:
            if (!is_jumping)
                m_state = m_state == States::_jump_loop_from_idle ? States::_jump_end_from_idle : States::_jump_end_from_walk_run;
            break;
        case States::_jump_end_from_idle:
        case States::_jump_end_from_walk_run:
            if (is_clip_finish)
                m_state = is_moving ? States::_walk_run : States::_idle;
            break;
        }

        return m_state;
    }

    std::string AnimationFSM::getCurrentClipBaseName()
    {
        // Implementation of getCurrentClipBaseName
        // This function should return the base name of the current animation clip
        // based on the current state of the FSM.
        // For now, I'll leave it as a placeholder. You may need to implement this
        // based on your specific requirements.
        return "placeholder_clip_name";
    }

} // namespace Piccolo