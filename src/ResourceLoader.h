#ifndef RESOURCE_LOADER_H
#define RESOURCE_LOADER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "common.h"
#include "Graphics/Mesh.h"

typedef std::shared_ptr<Mesh> MeshResource;

class ResourceLoader {
public:
    static MeshResource loadMesh(const std::string &meshname) {
        static std::unordered_map<std::string, MeshResource> meshes;

        if (meshes.count(meshname) == 0) {
            auto entry = std::make_pair(meshname, std::make_shared<Mesh>(meshname));
            meshes.insert(entry);
        }

        return meshes.at(meshname);
    }

private:
    ResourceLoader() {}
};

#endif
