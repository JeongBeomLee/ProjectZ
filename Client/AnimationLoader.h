#pragma once
#include "ResourceLoader.h"
#include "AnimationResource.h"
class AnimationLoader : public ResourceLoader {
public:
    bool CanLoadExtension(const std::string& extension) const override {
        static const std::unordered_set<std::string> supportedExtensions = {
            ".anim", ".fbx", ".gltf", ".glb"  // �ִϸ��̼� �����͸� ������ �� �ִ� ���ĵ�
        };
        return supportedExtensions.contains(extension);
    }

    std::unique_ptr<Resource> CreateResource(
        const std::string& name,
        const std::string& extension) override {
        return std::make_unique<AnimationResource>(name);
    }
};
