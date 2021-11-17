//
// Created by Ruiying on 2021/10/26.
//

#ifndef VULKANBASICS_BASETEXTURE_H
#define VULKANBASICS_BASETEXTURE_H
#include <vulkan/vulkan.h>

class BaseTexture {
public:
    BaseTexture(const char* textureFile);
    void CreateTexture(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue& queue);
    void DestroyTexture(VkDevice& device);

    const VkImageView* GetTextureImageView() const;

    const VkSampler* GetTextureSampler() const;

private:
    // create texture image
    void CreateTextureImage(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue& queue);

    // create texture image view
    void CreateTextureImageView(VkDevice& device);

    // create the texture sampler to make shader access the texture
    // also sample the texture (over-sampling or down-sampling)
    void CreateTextureSampler(VkDevice& device, VkPhysicalDevice& physicalDevice);

private:
    // texture file
    const char* m_textureFile;

    // texture image and its memory and image view
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;

    VkImageView m_textureImageView;

    // texture sampler
    VkSampler m_textureSampler;
};


#endif //VULKANBASICS_BASETEXTURE_H
