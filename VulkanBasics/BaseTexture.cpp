//
// Created by Ruiying on 2021/10/26.
//

#include "BaseTexture.h"
#include "VulkanHelperFunctions.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

BaseTexture::BaseTexture(const char *textureFile) {
    m_textureFile = textureFile;
}

void BaseTexture::CreateTexture(VkDevice &device, VkPhysicalDevice &physicalDevice, VkCommandPool &commandPool,
                                VkQueue &queue) {
    // create texture image (after creating command pool) and its image view
    CreateTextureImage(device, physicalDevice, commandPool, queue);
    CreateTextureImageView(device);
    CreateTextureSampler(device, physicalDevice);
}

void BaseTexture::DestroyTexture(VkDevice &device) {
        // destroy the texture image, texture image view and texture memory, texture sampler
        vkDestroySampler(device, m_textureSampler, nullptr);
        vkDestroyImageView(device, m_textureImageView, nullptr);
        vkDestroyImage(device, m_textureImage, nullptr);
        vkFreeMemory(device, m_textureImageMemory, nullptr);
}

void BaseTexture::CreateTextureImage(VkDevice &device, VkPhysicalDevice &physicalDevice, VkCommandPool &commandPool,
                                     VkQueue &queue) {
    // load the image
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(m_textureFile, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }
    // copy pixels data to stage buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanHelperFunctions::CreateBuffer( device, physicalDevice,imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    // free pixels data
    stbi_image_free(pixels);

    // create image object
    VulkanHelperFunctions::CreateImage(device, physicalDevice, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);
    // transit image layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    VulkanHelperFunctions::TransitionImageLayout(device, commandPool, queue, m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // copy staging buffer data to image object
    VulkanHelperFunctions::CopyBufferToImage(device, stagingBuffer,commandPool, queue, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    // transit image layout optimal for shader access
    VulkanHelperFunctions::TransitionImageLayout(device, commandPool, queue, m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // destroy the logical device
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void BaseTexture::CreateTextureImageView(VkDevice &device) {
    VulkanHelperFunctions::CreateImageView(device, m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, m_textureImageView);
}

void BaseTexture::CreateTextureSampler(VkDevice &device, VkPhysicalDevice &physicalDevice) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // over sampling
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    // addressMode determines what happens when you try to read texels outside the image
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // down sampling
    // check the GPU the limits the amount of texel samples that can be used to calculate the final color
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // normalized uv coordinates(range: [0,1))
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    // mipmap
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler!");
    }

}

const VkImageView* BaseTexture::GetTextureImageView() const {
    const VkImageView* imageView = &m_textureImageView;
    return imageView;
}

const VkSampler* BaseTexture::GetTextureSampler()  const {
    const VkSampler* sampler = &m_textureSampler;
    return sampler;
}


