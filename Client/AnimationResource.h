#pragma once
#include "Resource.h"
struct AnimationClip {
    std::string name;
    float duration;
    float ticksPerSecond;
    std::vector<std::vector<DirectX::XMFLOAT4X4>> keyFrames;
};

class AnimationResource : public Resource {
public:
    explicit AnimationResource(const std::string& name)
        : Resource(name) {}

    bool Load() override;
    void Unload() override;
    ResourceType GetType() const override { return ResourceType::Animation; }
    size_t GetMemoryUsage() const override;

    const std::vector<AnimationClip>& GetClips() const { return m_clips; }
    AnimationClip* FindClip(const std::string& name);

private:
    std::vector<AnimationClip> m_clips;
};