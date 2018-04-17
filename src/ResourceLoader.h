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

    static GLuint loadDDS(const std::string &path) {
        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
        // http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/#compressed-textures
        struct dds_header {
            uint32_t dwSize, dwFlags, dwHeight, dwWidth, dwPitchOrLinearSize;
            uint32_t dwDepth, dwMipMapCount, dwReserved1[11];
            struct dds_pixelformat {
                uint32_t dwSize, dwFlags, dwFourCC, dwRGBBitCount;
                uint32_t dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
            } ddspf;
            uint32_t dwCaps, dwCaps2, dwCaps3, dwCaps4, dwReserved2;
        } hdr;

        FILE *fp = fopen(path.c_str(), "rb");
        if (fp == nullptr) {
            LOG_ERROR("Failed to load dds file ", path, ": ", strerror(errno));
            return 0;
        }

        char filecode[4];
        fread(filecode, 1, 4, fp);
        if (strncmp(filecode, "DDS ", 4) != 0) {
            fclose(fp);
            LOG_ERROR("Failed to load dds file ", path, ": invalid filecode '", filecode, "'");
            return 0;
        }

        fread(&hdr, 124, 1, fp);

        unsigned int bufsize = hdr.dwMipMapCount > 1 ? hdr.dwPitchOrLinearSize * 2 : hdr.dwPitchOrLinearSize;
        unsigned char *buffer = (unsigned char *)malloc(bufsize * sizeof(unsigned char));
        fread(buffer, 1, bufsize, fp);

        fclose(fp);
        
        unsigned int components, format;
        if (strncmp((char *)&hdr.ddspf.dwFourCC, "DXT1", 4) == 0) {
            components = 3;
            format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        }
        else if (strncmp((char *)&hdr.ddspf.dwFourCC, "DXT3", 4) == 0) {
            components = 4;
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        }
        else if (strncmp((char *)&hdr.ddspf.dwFourCC, "DXT5", 4) == 0) {
            components = 4;
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        }
        else {
            free(buffer);
            return 0;
        }

        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        if (GLAD_GL_EXT_texture_filter_anisotropic) {
            float maxAnisotropy = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
        }

        unsigned int blockSize = (format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ? 8 : 16;
        unsigned int offset = 0;
        unsigned int width = hdr.dwWidth, height = hdr.dwHeight;
        for (unsigned int level = 0; level < hdr.dwMipMapCount && (width || height); ++level) {
            unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
            glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height, 0, size, buffer + offset);
            offset += size;
            width /= 2;
            height /= 2;
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        free(buffer);
        return texture_id;
    }

private:
    ResourceLoader() {}
};

#endif
