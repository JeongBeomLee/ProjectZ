#pragma once
#include "ResourceLoader.h"

class TextureLoader : public ResourceLoader {
public:
    bool CanLoadExtension(const std::string& extension) const override {
        static const std::unordered_set<std::string> supportedExtensions = {
            ".dds", ".png", ".jpg", ".jpeg", ".tga"
        };
        return supportedExtensions.contains(extension);
    }

    std::unique_ptr<Resource> CreateResource(
        const std::string& name,
        const std::string& extension) override {
        return std::make_unique<TextureResource>(name);
    }
};