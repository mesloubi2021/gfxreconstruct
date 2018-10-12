/*
** Copyright (c) 2018 LunarG, Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef BRIMSTONE_VULKAN_CONSUMER_BASE_H
#define BRIMSTONE_VULKAN_CONSUMER_BASE_H

#include "vulkan/vulkan.h"

#include "util/defines.h"
#include "format/descriptor_update_template_decoder.h"
#include "format/platform_types.h"
#include "format/pointer_decoder.h"
#include "format/string_decoder.h"
#include "format/struct_pointer_decoder.h"
#include "format/struct_pointer_decoder_nvx.h"

BRIMSTONE_BEGIN_NAMESPACE(brimstone)
BRIMSTONE_BEGIN_NAMESPACE(format)

class VulkanConsumerBase
{
public:
    VulkanConsumerBase() {}

    virtual ~VulkanConsumerBase() {}

    virtual void ProcessDisplayMessageCommand(const std::string& message) {}

    virtual void ProcessFillMemoryCommand(uint64_t memory_id, uint64_t offset, uint64_t size, const uint8_t* data) {}

    virtual void ProcessResizeWindowCommand(HandleId surface_id, uint32_t width, uint32_t height) {}

    virtual void Process_vkUpdateDescriptorSetWithTemplate(HandleId device,
                                                           HandleId descriptorSet,
                                                           HandleId descriptorUpdateTemplate,
                                                           const DescriptorUpdateTemplateDecoder& pData) = 0;

    virtual void Process_vkCmdPushDescriptorSetWithTemplateKHR(HandleId commandBuffer,
                                                               HandleId descriptorUpdateTemplate,
                                                               HandleId layout,
                                                               uint32_t set,
                                                               const DescriptorUpdateTemplateDecoder& pData) = 0;

    virtual void Process_vkUpdateDescriptorSetWithTemplateKHR(HandleId device,
                                                              HandleId descriptorSet,
                                                              HandleId descriptorUpdateTemplate,
                                                              const DescriptorUpdateTemplateDecoder& pData) = 0;

    virtual void
    Process_vkRegisterObjectsNVX(VkResult                                                   returnValue,
                                 HandleId                                                   device,
                                 HandleId                                                   objectTable,
                                 uint32_t                                                   objectCount,
                                 const StructPointerDecoder<Decoded_VkObjectTableEntryNVX>& ppObjectTableEntries,
                                 const PointerDecoder<uint32_t>&                            pObjectIndices) = 0;
};

BRIMSTONE_END_NAMESPACE(format)
BRIMSTONE_END_NAMESPACE(brimstone)

#endif // BRIMSTONE_VULKAN_CONSUMER_BASE_H