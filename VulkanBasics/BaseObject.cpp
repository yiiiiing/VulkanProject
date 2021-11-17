//
// Created by Ruiying on 2021/10/25.
//

#include "BaseObject.h"
#include <map>
#include <unordered_map>
#include <random>
#include <time.h>
#include "VulkanHelperFunctions.h"
#include "Vertex.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

BaseObject::BaseObject(ObjectType objectType, const char *objectFile)
{
    m_objectType = objectType;
    switch (m_objectType) {
        case ObjectType::FixedTriangle:
            CreateTriangle();
            break;
        case ObjectType::FixedRectangle:
            CreateRectangle();
            break;
        case ObjectType::OBJ_Model:
            if (!objectFile){throw std::runtime_error("OBJ model must have OBJ file");}
            CreateOBJ(objectFile);
            break;
        case ObjectType::DefaultMax:
            break;
    }

}

void BaseObject::CreateObject(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue queue, const uint32_t& swapChainImageSize, const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent) {
    m_swapChainExtent = swapChainExtent;
    // create descriptor set layout for uniform buffer (before graphics pipeline)
    CreateDescriptorSetLayout(device);
    // create graphics pipeline
    CreateGraphicsPipeline(device, renderPass, swapChainExtent);
    // create vertex buffer(must before creating command buffers)
    CreateVertexBuffer(device, physicalDevice, commandPool, queue);
    CreateIndexBuffer(device, physicalDevice, commandPool, queue);
    CreateUniformBuffers(device, physicalDevice, swapChainImageSize);
    CreateDescriptorPool(device, swapChainImageSize);
    CreateDescriptorSets(device, swapChainImageSize);
}

void BaseObject::DestroyObject(VkDevice& device) {
    // destroy pipeline
    vkDestroyPipeline(device, m_graphicsPipeline, nullptr);
    // destroy pipeline layout
    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);

    // destroy the vertex buffer and free memory for the vertex buffer
    vkDestroyBuffer(device, m_vertexBuffer, nullptr);
    vkFreeMemory(device, m_vertexBufferMemory, nullptr);
    // destroy the index buffer
    vkDestroyBuffer(device, m_indexBuffer, nullptr);
    vkFreeMemory(device, m_indexBufferMemory, nullptr);
    // destroy the uniform buffer
    size_t size = m_uniformBuffers.size();
    for (size_t i = 0; i < size; i++) {
        vkDestroyBuffer(device, m_uniformBuffers[i], nullptr);
        vkFreeMemory(device, m_uniformBuffersMemory[i], nullptr);
    }

    // destroy descriptor pool
    vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
}

void BaseObject::CreateDescriptorSetLayout(VkDevice& device) {
    // uniform buffer object binding (for vertex shader)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    // texture sampler binding (for fragment shader)
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // array of two bindings
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

    // create descriptor set layout
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void BaseObject::CreateDescriptorPool(VkDevice &device, const uint32_t& swapChainImageSize) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    // pool size for uniform buffer
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = swapChainImageSize;
    // pool size for image sampler
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = swapChainImageSize;

    // create descriptor pool
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = swapChainImageSize;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void BaseObject::CreateDescriptorSets(VkDevice &device, const uint32_t& swapChainImageSize) {
    std::vector<VkDescriptorSetLayout> layouts(swapChainImageSize, m_descriptorSetLayout);
    // using descriptor pool and descriptor set layout to create descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = swapChainImageSize;
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(swapChainImageSize);
    if (vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < swapChainImageSize; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (m_texture)
        {
            if (m_texture->GetTextureImageView() && m_texture->GetTextureSampler())
            {
                imageInfo.imageView = *(m_texture->GetTextureImageView());
                imageInfo.sampler = *(m_texture->GetTextureSampler());

            }
        }else {
            imageInfo.imageView = VK_NULL_HANDLE;
            imageInfo.sampler = VK_NULL_HANDLE;
        }

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}

void BaseObject::CreateGraphicsPipeline(VkDevice& device, const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent) {
    std::vector<char> vertShaderCode = VulkanHelperFunctions::ReadBinaryFile("shaders/vert.spv");
    std::vector<char> fragShaderCode;
    if (m_objectType != ObjectType::FixedTriangle)
    {
        fragShaderCode = VulkanHelperFunctions::ReadBinaryFile("shaders/fragTexture.spv");
    }
    else
    {
        fragShaderCode = VulkanHelperFunctions::ReadBinaryFile("shaders/fragNoTexture.spv");
    }
    // shader module
    VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);

    /* shader stage creation*/
    // vertex shader stage
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    // frag shader stage
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // vertex input stage
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // input assembly stage
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // viewport and scissors stage
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // rasterizer stage
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // counter if using projection matrix
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // multisampling stage
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // TODO depth and stencil testing

    // color blending stage
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    // TODO dynamic state

    /*pipeline layout*/
    // specify the uniform values by creating VkPipelineLayout object
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // bind with descriptor set layout for uniform buffer
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // destroy shader module
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

}

VkShaderModule BaseObject::CreateShaderModule(VkDevice& device, const std::vector<char> &shaderCode) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    return shaderModule;
}

void BaseObject::CreateVertexBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue queue) {
    // Buffer size (size of input vertices data)
    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    // create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    // create buffer (specify the buffer size, buffer usage, memory property flag, creating buffer and creating buffer memory)
    VulkanHelperFunctions::CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // copy vertices data to staging buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // create vertex buffer
    VulkanHelperFunctions::CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

    // copy data from staging buffer to vertex buffer
    VulkanHelperFunctions::CopyBuffer(device, commandPool, queue, stagingBuffer, m_vertexBuffer, bufferSize);

    // destroy staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void BaseObject::CreateIndexBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue queue) {
    VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanHelperFunctions::CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // create index buffer and allocate the index memory
    VulkanHelperFunctions::CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

    VulkanHelperFunctions::CopyBuffer(device, commandPool, queue, stagingBuffer, m_indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void BaseObject::CreateUniformBuffers(VkDevice& device, VkPhysicalDevice& physicalDevice, const uint32_t& swapChainImageSize) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    // create uniform buffer for each image in the swap chain
    m_uniformBuffers.resize(swapChainImageSize);
    m_uniformBuffersMemory.resize(swapChainImageSize);

    for (size_t i = 0; i < swapChainImageSize; i++) {
        VulkanHelperFunctions::CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }
}

void BaseObject::UpdateUniformBuffer(VkDevice &device, float duration, uint32_t currentImage) {
    UniformBufferObject ubo;
    // TODO OnCollision() callback, Update(float deltaTime), Begin(), like game engine
    switch (m_objectType) {
        case ObjectType::FixedTriangle :
            ubo.projectionMatrix = glm::mat4(1.0);
            ubo.projectionMatrix[1][1] *= -1;
            ubo.transformMatrix = ubo.projectionMatrix * TranslateObject(m_triMoveSpeed);
            UpdateTriMovingDirection();
            break;
        case ObjectType::FixedRectangle :
            ubo.projectionMatrix = glm::mat4(1.0);
            ubo.projectionMatrix[1][1] *= -1;
            ubo.modelMatrix = RotateObject(duration);
            ubo.transformMatrix = ubo.projectionMatrix * ubo.modelMatrix;
            break;
        case ObjectType::OBJ_Model :
            // view matrix
            ubo.viewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            // projection matrix
            ubo.projectionMatrix = glm::perspective(glm::radians(45.0f), m_swapChainExtent.width / (float) m_swapChainExtent.height, 0.1f, 10.0f);
            ubo.projectionMatrix[1][1] *= -1;
            ubo.modelMatrix = RotateObject(0.5f * duration);
            ubo.transformMatrix =  ubo.projectionMatrix * ubo.viewMatrix;
            break;
        case ObjectType::DefaultMax :
            break;
    }
    // copy ubo data to uniform buffer
    void* data;
    vkMapMemory(device, m_uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, m_uniformBuffersMemory[currentImage]);
}

void BaseObject::CreateTriangle() {
    m_vertices = {
            {{-0.2f, -0.2f, 0.f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.2f, -0.2f, 0.f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.f, 0.2f, 0.f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            };
    m_indices  = { 0, 1, 2};

    m_triangle.vertPos0 = m_vertices[0].pos;
    m_triangle.vertPos1 = m_vertices[1].pos;
    m_triangle.vertPos2 = m_vertices[2].pos;
    m_triangle.centerPos = glm::vec3(0.f);
    // initialize direction and move speed (random and obey normal distribution)
    std::default_random_engine generator;
    generator.seed(time(NULL));
    std::normal_distribution<float> distribution(-1.f,1.f);
    m_triMoveDirection = glm::normalize(glm::vec3(distribution(generator), distribution(generator), 0.0f));
    // distance/frame, but it's not stable.
    m_triMoveSpeed = std::abs(distribution(generator) / 50.f);


    // TODO stable way: speed(distance/seconds) * deltaTime(seconds/frame) = distance/frame
    // TODO measure the deltaTime
}

void BaseObject::CreateRectangle() {
    m_vertices = {
            {{-0.5f, -0.5f, 0.f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f, -0.5f, 0.f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
            };
    m_indices = {0, 1, 2, 2,3,0};
}

void BaseObject::CreateOBJ(const char *objectFile) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objectFile)) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.textureCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            vertex.normals = {
                    attrib.normals[3 * index.normal_index+ 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
            };

            // check if this vertex has been added to uniqueVertices
            if(uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }
            m_indices.push_back(uniqueVertices[vertex]);
        }
    }

}

glm::mat4 BaseObject::TranslateObject(float rate) {
    m_triangle.UpdateTrianglePosition(rate,m_triMoveDirection);
    return glm::translate(glm::mat4(1.0f), m_triangle.centerPos);
}

glm::mat4 BaseObject::RotateObject(float rate) {
    // model matrix
    return glm::rotate(glm::mat4(1.0f), rate * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

std::map<WindowEdge, const glm::vec3> WindowEdgeNormal{
    {WindowEdge::left, glm::vec3(WINDOW_EDGE_MAX, 0.f, 0.f)},
    {WindowEdge::right, glm::vec3(WINDOW_EDGE_MIN, 0.f, 0.f)},
    {WindowEdge::bottom, glm::vec3(0.f, WINDOW_EDGE_MIN,  0.f)},
    {WindowEdge::top, glm::vec3(0.f, WINDOW_EDGE_MAX, 0.f)},
    };

void BaseObject::UpdateTriMovingDirection() {
    if (!m_triangle.IsOutOfScreen()) return;
    glm::vec3 normalVector = WindowEdgeNormal[m_triangle.collisionEdge.value()];
    m_triMoveDirection = 2.f * glm::dot(m_triMoveDirection, normalVector) * normalVector - m_triMoveDirection;
    m_triMoveDirection *= -1.f;
    m_triangle.collisionEdge.reset();
}












