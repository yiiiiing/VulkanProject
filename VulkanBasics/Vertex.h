//
// Created by Ruiying on 2021/10/25.
//

#ifndef VULKANBASICS_VERTEX_H
#define VULKANBASICS_VERTEX_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <vulkan/vulkan.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


struct Vertex{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 textureCoord;
    glm::vec3 normals;

    bool operator==(const Vertex& other) const{
        return pos == other.pos && color == other.color && textureCoord == other.textureCoord;
    }

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        // binding index
        bindingDescription.binding = 0;
        // number of bytes between data entries (sizeof(Vertex struct))
        bindingDescription.stride = sizeof(Vertex);
        // whether to move to the next data entry after each vertex or after each instance
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    // input attribute to vertex shader (pos and color)
    static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
        // pos attribute (vec2, 32bit float)
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0; // same as it in the vertex shader
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        // color attribute (vec3, 32bit float)
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        // texture coord attribute
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, textureCoord);

        // normal attribute
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, normals);

        return attributeDescriptions;
    }
};

// hash calculation for Vertex struct
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
            (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
            (hash<glm::vec2>()(vertex.textureCoord) << 1);
        }
    };
}





#endif //VULKANBASICS_VERTEX_H
