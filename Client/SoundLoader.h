#pragma once
#include "ResourceLoader.h"

class SoundLoader : public ResourceLoader {
public:
    bool CanLoadExtension(const std::string& extension) const override {
        static const std::unordered_set<std::string> supportedExtensions = {
            ".wav", ".ogg", ".mp3"
        };
        return supportedExtensions.contains(extension);
    }

    std::unique_ptr<Resource> CreateResource(
        const std::string& name,
        const std::string& extension) override {
        return std::make_unique<SoundResource>(name);
    }
};
