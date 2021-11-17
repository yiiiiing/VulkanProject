//
// Created by Ruiying on 2021/10/25.
//

#ifndef VULKANBASICS_BASEOBJECT_H
#define VULKANBASICS_BASEOBJECT_H
#include <vector>
#include "Vertex.h"
#include <optional>
#include "BaseTexture.h"


enum class ObjectType{FixedTriangle, FixedRectangle, OBJ_Model, DefaultMax};

struct UniformBufferObject {
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 transformMatrix;
};

#define WINDOW_EDGE_MIN -1.f
#define WINDOW_EDGE_MAX 1.f
#define EPSILON 0.001f
enum class WindowEdge{
    left = 0,
    right,
    bottom,
    top,
    defaultMax
};

struct Triangle{
    glm::vec3 vertPos0;
    glm::vec3 vertPos1;
    glm::vec3  vertPos2;
    glm::vec3 centerPos;
    std::optional<WindowEdge> collisionEdge;
    // (xmin, xmax, ymin, ymax)
    glm::vec4 GetBoundingBox2D(){
        std::pair<float, float> xMinMax = std::minmax({vertPos0.x, vertPos1.x, vertPos2.x});
        std::pair<float, float> yMinMax = std::minmax({vertPos0.y, vertPos1.y, vertPos2.y});
        return glm::vec4(xMinMax.first, xMinMax.second, yMinMax.first, yMinMax.second);
    }
    bool IsOutOfScreen(){
        glm::vec4 boundingBox = GetBoundingBox2D();
        if (boundingBox[2] < WINDOW_EDGE_MIN + EPSILON) { collisionEdge = WindowEdge::top;}
        else if (boundingBox[3] > WINDOW_EDGE_MAX - EPSILON) { collisionEdge = WindowEdge::bottom;}
        if(boundingBox[0] < WINDOW_EDGE_MIN + EPSILON) { collisionEdge = WindowEdge::left;}
        else if (boundingBox[1] > WINDOW_EDGE_MAX - EPSILON) { collisionEdge = WindowEdge::right;}

        if (collisionEdge.has_value()) {return true;}
        return false;
    }
    void UpdateTrianglePosition(const glm::float32& moveAmount, const glm::vec3& direction){
        centerPos += direction * moveAmount;
        vertPos0 += direction * moveAmount;
        vertPos1 += direction * moveAmount;
        vertPos2 += direction * moveAmount;
    }
};

class BaseObject {
public:
    BaseObject(ObjectType objectType, const char* objectFile);

    void CreateObject(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue queue, const uint32_t& swapChainImageSize, const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent);
    void DestroyObject(VkDevice& device);

    // update uniform buffer
    void UpdateUniformBuffer(VkDevice &device, float duration, uint32_t currentImage);

    inline void SetTexture(BaseTexture* texture){m_texture = texture;}

private:
    // create triangle (task1)
    void CreateTriangle();
    // create rectangle (task2)
    void CreateRectangle();
    // create OBJ object
    void CreateOBJ(const char* objectFile);

    // create shader module
    VkShaderModule CreateShaderModule(VkDevice& device, const std::vector<char>& shaderCode);

    // create graphics pipeline layout and pipeline
    void CreateGraphicsPipeline(VkDevice& device, const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent);

    // create descriptor set layout for uniform buffer (before graphics pipeline)
    void CreateDescriptorSetLayout(VkDevice& device);

    // create descriptor pool
    void CreateDescriptorPool(VkDevice& device, const uint32_t& swapChainImageSize);

    // create descriptor sets
    void CreateDescriptorSets(VkDevice& device, const uint32_t& swapChainImageSize);

    // create vertex buffer
    void CreateVertexBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue queue);

    // create index buffer
    void CreateIndexBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue queue);

    // create uniform buffers
    void CreateUniformBuffers(VkDevice& device, VkPhysicalDevice& physicalDevice, const uint32_t& swapChainImageSize);

    /*transform the object*/
    // translate the object
    glm::mat4 TranslateObject(float rate);
    // rotate the object
    glm::mat4 RotateObject(float rate);

    // update triangle moving direction if collided with window
    void UpdateTriMovingDirection();

public:
    // pipeline layout
    VkPipelineLayout m_pipelineLayout;
    // graphics Pipeline;
    VkPipeline m_graphicsPipeline;
    std::vector<VkDescriptorSet> m_descriptorSets;
    // index buffer
    VkBuffer m_indexBuffer;
    // vertex buffer
    VkBuffer m_vertexBuffer;
    // indices
    std::vector<uint32_t> m_indices;

private:
    ObjectType m_objectType;

    VkExtent2D m_swapChainExtent;

    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;

    // memory for vertex buffer
    VkDeviceMemory m_vertexBufferMemory;

    VkDeviceMemory m_indexBufferMemory;

    // uniform buffers and memories for them
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;

    // vertices
    std::vector<Vertex> m_vertices;

    BaseTexture* m_texture = nullptr;

    // triangle, used for triangle collision detection in task1
    Triangle m_triangle;
    // triangle initial moving direction (normalized direction)
    glm::vec3 m_triMoveDirection;
    // triangle initial moving speed (e.g. 0.5 unit/s using normalized device coordinates)
    glm::float32 m_triMoveSpeed;
};


#endif //VULKANBASICS_BASEOBJECT_H
