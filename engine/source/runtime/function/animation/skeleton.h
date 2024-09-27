#pragma once

#include "runtime/resource/res_type/components/animation.h"
#include "runtime/function/animation/node.h"

namespace Piccolo
{
    class SkeletonData;
    class BlendStateWithClipData;

    class AnimationPose
    {
    public:
        std::vector<Transform> m_bone_poses;
        BoneBlendWeight m_weight;

        void blend(const AnimationPose& other);
    };

    class Skeleton
    {
    private:
        bool  m_is_flat {false};
        int   m_bone_count {0};
        Bone* m_bones {nullptr};

    public:
        ~Skeleton();

        void            buildSkeleton(const SkeletonData& skeleton_definition);
        void            applyAnimation(const BlendStateWithClipData& blend_state);
        AnimationResult outputAnimationResult();
        void            resetSkeleton();
        const Bone*     getBones() const;
        int32_t         getBonesCount() const;
    };
} // namespace Piccolo