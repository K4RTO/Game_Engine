#pragma once

#include "runtime/resource/res_type/data/animation_clip.h"
#include "runtime/resource/res_type/data/animation_skeleton_node_map.h"
#include "runtime/resource/res_type/data/blend_state.h"
#include "runtime/resource/res_type/data/skeleton_data.h"
#include "runtime/resource/res_type/data/skeleton_mask.h"
#include "runtime/core/math/math_headers.h" 
#include "runtime/function/animation/skeleton.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "json11.hpp"

namespace Piccolo
{
    class AnimationManager
    {
    private:
        static std::map<std::string, std::shared_ptr<SkeletonData>>  m_skeleton_definition_cache;
        static std::map<std::string, std::shared_ptr<AnimationClip>> m_animation_data_cache;
        static std::map<std::string, std::shared_ptr<AnimSkelMap>>   m_animation_skeleton_map_cache;
        static std::map<std::string, std::shared_ptr<BoneBlendMask>> m_skeleton_mask_cache;

    public:
        static std::shared_ptr<SkeletonData>  tryLoadSkeleton(std::string file_path);
        static std::shared_ptr<AnimationClip> tryLoadAnimation(std::string file_path);
        static std::shared_ptr<AnimSkelMap>   tryLoadAnimationSkeletonMap(std::string file_path);
        static std::shared_ptr<BoneBlendMask> tryLoadSkeletonMask(std::string file_path);
        static BlendStateWithClipData         getBlendStateWithClipData(const BlendState& blend_state);

        static void blendPoses(const AnimationPose& pose1, const AnimationPose& pose2, float blendFactor, AnimationPose& outPose);
        static bool updateAnimationFSM(const std::map<std::string, bool>& signals);

        AnimationManager() = default;
    };

    class AnimationFSM
    {
    public:
        enum class States
        {
            _idle,
            _walk_start,
            _walk_run,
            _walk_stop,
            _jump_start_from_idle,
            _jump_loop_from_idle,
            _jump_end_from_idle,
            _jump_start_from_walk_run,
            _jump_loop_from_walk_run,
            _jump_end_from_walk_run,
            _count
        };

        static States updateState(const std::map<std::string, bool>& signals);
        static std::string getCurrentClipBaseName();
    };
     class AnimationSystem
    {
    public:
        void tick(float delta_time);
        void playAnimation(const std::string& clip_name);
        void updateAnimation(float delta_time);

    private:
        std::map<std::string, std::shared_ptr<AnimationClip>> m_animation_clips;
        AnimationFSM::States m_current_state = AnimationFSM::States::_idle;
        std::string m_current_clip_name = "idle_anim";
    };
} // namespace Piccolo