#pragma once
#include "Resource.h"

class ResourceLoader {
public:
	virtual ~ResourceLoader() = default;
    virtual bool CanLoadExtension(const std::string& extension) const = 0;
    virtual std::unique_ptr<Resource> CreateResource(const std::string& name, const std::string& extension) = 0;
};

