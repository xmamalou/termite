/*
   Copyright 2023 Christopher-Marios Mamaloukas

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "../Internal.h"

#include <stdlib.h>

/* -------------------- *
 *       INTERNAL       *
 * -------------------- */

static int _dflBlockReserveMemory(struct TrmMemoryPoolInfo* pInfo, struct TrmMemoryBlock_T* pMemoryBlock);
static int _dflBlockReserveMemory(struct TrmMemoryPoolInfo* pInfo, struct TrmMemoryBlock_T* pMemoryBlock)
{
    if (pInfo->device == NULL)
        pMemoryBlock->startingAddress = calloc(pMemoryBlock->size, sizeof(char)); // chars are 1 byte long. I think having the memory initialized as zeroes is a good idea.
    else
    {
        pMemoryBlock->hDevice = pInfo->device;

        for (int i = 0; i < ((pInfo->useShared == false) ? pInfo->localHeapCount : pInfo->sharedHeapCount); i++) // we are searching the local heaps for available memory
        {
            // we will now create the buffer and assign the memory to it 
            VkBufferCreateInfo buffInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = pMemoryBlock->size,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };
            if (vkCreateBuffer(pInfo->device, &buffInfo, NULL, &pMemoryBlock->hBufferHandle) != VK_SUCCESS)
                return TRM_VULKAN_DEVICE_BUFFER_CREATION_ERROR;

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(pInfo->device, pMemoryBlock->hBufferHandle, &memRequirements);

            VkMemoryAllocateInfo memInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = (pInfo->useShared == false) ? pInfo->pLocalHeaps[i].memoryHeapIndex : pInfo->pLocalHeaps[i].memoryHeapIndex,
            };

            if (vkAllocateMemory(pInfo->device, &memInfo, NULL, &pMemoryBlock->hMemoryHandle) != VK_SUCCESS)
                continue;

            if (vkBindBufferMemory(pInfo->device, pMemoryBlock->hBufferHandle, pMemoryBlock->hMemoryHandle, 0) != VK_SUCCESS)
            {
                vkDestroyBuffer(pInfo->device, pMemoryBlock->hBufferHandle, NULL);
                continue;
            }

            if ((pInfo->useShared == false) ? (pInfo->pLocalHeaps[i].isHostVisible == true) : (pInfo->pLocalHeaps[i].isHostVisible == true))
            {
                vkMapMemory(pInfo->device, pMemoryBlock->hMemoryHandle, 0, memRequirements.size, NULL, &pMemoryBlock->startingAddress);
                if (pMemoryBlock->startingAddress == NULL)
                    return TRM_VULKAN_DEVICE_UNMAPPABLE_MEMORY_ERROR;
                break;

            }
            else
            {
                // if the memory cannot be mapped, we will create a command buffer that will
                // copy the data from the following 4-byte buffer to the buffer that is on the 
                // device's memory when the user attempts to put data in some allocated memory.
                // Since the buffer's size will be known at allocation time, both it and the 
                // command buffer will be created at `trmAllocate`.
                pMemoryBlock->miniBuff = calloc(1, sizeof(int));
                break;
            }
        }
    }

    if ((pMemoryBlock->startingAddress == NULL) && (pMemoryBlock->hMemoryHandle == NULL))
        return TRM_GENERIC_OOM_ERROR; // if hMemoryHandle isn't NULL, it's not unlikely that the memory just isn't mapped (which would make the startingAddress NULL). But that's not a problem, necessarily.

    if ((pMemoryBlock->hDevice != NULL) && (pMemoryBlock->hMemoryHandle == NULL))
        return TRM_VULKAN_DEVICE_NO_MEMORY_ERROR;

    if ((pMemoryBlock->hDevice != NULL) && (pMemoryBlock->hBufferHandle == NULL))
        return TRM_VULKAN_DEVICE_BUFFER_CREATION_ERROR;

    return TRM_SUCCESS;
}

int _dflBufferChunkCreateCommandBuff(struct TrmBufferInfo* pInfo, struct TrmBufferChunk_T* pChunk);
int _dflBufferChunkCreateCommandBuff(struct TrmBufferInfo* pInfo, struct TrmBufferChunk_T* pChunk)
{
    VkCommandBufferAllocateInfo commInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pInfo->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(pInfo->device, &commInfo, &pChunk->transferOp) != NULL)
        return TRM_VULKAN_DEVICE_COMMAND_CREATION_ERROR;

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    if (vkBeginCommandBuffer(pChunk->transferOp, &beginInfo) != VK_SUCCESS)
        return TRM_VULKAN_DEVICE_COMMAND_CREATION_ERROR;

    // how the command works:
    // 1. wait until the host has copied the data to minibuff
    // 2. copy the data from minibuff to the buffer on the device's memory
    // 3. signal the host that the copy is done
    // 4. go back to step 1
    // This is repeated `size` times (as minibuff is 4 bytes long)

    for (int i = 0; i < ((pChunk->size) / 4); i++)
    {
        vkCmdWaitEvents(pChunk->transferOp, 1, &pChunk->hostCanGetNextPart, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0, NULL, 0, NULL, 0, NULL);
        vkCmdUpdateBuffer(pChunk->transferOp, pChunk->associatedBlock->hBufferHandle, pChunk->offset + (i * 4), 4, pChunk->associatedBlock->miniBuff);
        //vkCmdSetEvent(pChunk->transferOp, pChunk->hostCanGetNextPart, );
    }

    if (vkEndCommandBuffer(pChunk->transferOp) != VK_SUCCESS)
        return TRM_VULKAN_DEVICE_COMMAND_CREATION_ERROR;

    return TRM_SUCCESS;
}

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

TrmMemoryPool trmMemoryPoolCreate(struct TrmMemoryPoolInfo* pInfo)
{
    struct TrmMemoryPool_T* memoryPool = calloc(1, sizeof(struct TrmMemoryPool_T));

    if (memoryPool == NULL)
        return NULL;

    memoryPool->size = pInfo->size * 4; // transform from 4-byte words to bytes
    memoryPool->used = 0;
    memoryPool->error = TRM_SUCCESS;

    struct TrmMemoryBlock_T* block = calloc(1, sizeof(struct TrmMemoryBlock_T));
    if (block == NULL)
    {
        memoryPool->error = TRM_MEMORY_UNAVAILABLE_BLOCKS_ERROR;
        return (TrmMemoryPool)memoryPool;
    }

    block->size = pInfo->size * 4; // transform from 4-byte words to bytes
    block->used = 0;
    block->next = NULL;

    block->slots[0] = 0;
    block->slots[1] = block->size; // the first slot of a newly created block is the whole block

    for (int i = 1; i < TRM_MAX_ITEM_COUNT; i++)
        block->slots[2 * i] = 1; // slots[2*i + 1] = 0, which means that the slot is marked as used.

    memoryPool->error = _dflBlockReserveMemory(pInfo, block);

    memoryPool->firstBlock = block;

    return (TrmMemoryPool)memoryPool;
}

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

void trmMemoryPoolExpand(struct TrmMemoryPoolInfo* pInfo, TrmMemoryPool hMemoryPool)
{
    TRM_MEMORY_POOL->size += pInfo->size * 4; // transform from 4-byte words to bytes

    struct TrmMemoryBlock_T* block = TRM_MEMORY_POOL->firstBlock;
    while (block->next != NULL)
    {
        block = block->next;
        continue;
    }

    struct TrmMemoryBlock_T* newBlock = calloc(1, sizeof(struct TrmMemoryBlock_T));
    if (newBlock == NULL)
    {
        TRM_MEMORY_POOL->error = TRM_MEMORY_UNAVAILABLE_BLOCKS_ERROR;
        return;
    }

    newBlock->size = pInfo->size * 4; // transform from 4-byte words to bytes
    newBlock->used = 0;
    newBlock->next = NULL;

    newBlock->slots[0] = 0;
    newBlock->slots[1] = block->size; // the first slot of a newly created block is the whole block

    for (int i = 1; i < TRM_MAX_ITEM_COUNT; i++)
        block->slots[2 * i] = 1; // slots[2*i + 1] = 0, which means that the slot is marked as used.

    TRM_MEMORY_POOL->error = _dflBlockReserveMemory(pInfo, newBlock);

    block->next = newBlock;
}

TrmBuffer trmAllocate(struct TrmBufferInfo* pBufferInfo, TrmMemoryPool hMemoryPool)
{
    // allocation happens this way: Termite checks if a memory block has available space. If it has,
    // then it will start filling chunks, up to TRM_MAX_ITEM_COUNT. Each chunk's size depends on how big
    // the slots of available memory in each block of the memory pool are.
    if (TRM_MEMORY_POOL->used >= TRM_MEMORY_POOL->size)
    {
        TRM_MEMORY_POOL->error = TRM_MEMORY_OOM_ERROR;
        return NULL;
    }

    struct TrmBuffer_T* buffer = calloc(1, sizeof(struct TrmBuffer_T));
    if (buffer == NULL)
    {
        TRM_MEMORY_POOL->error = TRM_GENERIC_OOM_ERROR;
        return NULL;
    }

    uint64_t remainingSize = pBufferInfo->size;
    for (int i = 0; i < TRM_MAX_ITEM_COUNT; i++) // chunks
    {
        struct TrmMemoryBlock_T* block = TRM_MEMORY_POOL->firstBlock;
        while (block != NULL)
        {
            if ((block->size - block->used) < buffer->size)
            {
                block = block->next;
                continue;
            }

            buffer->chunks[i].associatedBlock = block;

            int bestSlot = 0; // the first slot is set up when the memory pool is created
            // search for the best slot (AKA the slot with the biggest size so as to minimize the amount of chunks needed for a buffer)
            for (int j = 0; j < TRM_MAX_ITEM_COUNT; j++) // slots
            {
                // if slots[2*j] > slots[2*j + 1], then the result is negative, hence always smaller 
                // than the current best slot (even if it's 0 and filled, then it'd be - 1 > - 1 : false, hence no point in 
                // changing the best slot index
                if ((block->slots[2 * j + 1] - block->slots[2 * j]) > (block->slots[2 * bestSlot + 1] - block->slots[2 * bestSlot]))
                    bestSlot = j;
            }

            // the process remains unchanged even if we are dealing with unmapped device memory. 
            // The command buffer will simply use the offsets of the chunks to emulate the structure of the memory pool.
            buffer->chunks[i].size = (remainingSize > (block->slots[2 * bestSlot + 1] - block->slots[2 * bestSlot])) ? \
                ((block->slots[2 * bestSlot + 1] - block->slots[2 * bestSlot]) * 4) : (remainingSize * 4);
            buffer->chunks[i].offset = 4 * block->slots[2 * bestSlot];
            remainingSize -= (remainingSize > (block->slots[2 * bestSlot + 1] - block->slots[2 * bestSlot])) ? \
                (block->slots[2 * bestSlot + 1] - block->slots[2 * bestSlot]) : (remainingSize);

            if (block->hDevice != NULL)
            {
                VkEventCreateInfo eventInfo = {
                    .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
                    .pNext = NULL,
                };
                vkCreateEvent(block->hDevice, &eventInfo, NULL, &buffer->chunks[i].hostCanGetNextPart);

                TRM_MEMORY_POOL->error = _dflBufferChunkCreateCommandBuff(&buffer->chunks[i], block->hDevice);
            }

            block->slots[2 * bestSlot] += buffer->chunks[i].size;
            block->used += buffer->chunks[i].size * 4;
        }

        if (buffer->chunks[i].associatedBlock == NULL)
        {
            TRM_MEMORY_POOL->error = TRM_MEMORY_OOM_ERROR;
            return NULL;
        }
    }

    TRM_MEMORY_POOL->used += (pBufferInfo->size - remainingSize) * 4; // it's possible that the buffer couldn't be fully allocated

    return (TrmBuffer)buffer;
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

int trmMemoryPoolSizeGet(TrmMemoryPool hMemoryPool)
{
    return TRM_MEMORY_POOL->size;
}

inline int trmMemoryPoolBlockCountGet(TrmMemoryPool hMemoryPool)
{
    int blockCount = 1; // every memory pool has at least one block

    struct TrmMemoryBlock_T* block = TRM_MEMORY_POOL->firstBlock;

    while (block->next != NULL)
    {
        blockCount++;
        block = block->next;
    }

    return blockCount;
}

int trmMemoryPoolErrorGet(TrmMemoryPool hMemoryPool)
{
    return TRM_MEMORY_POOL->error;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void trmMemoryPoolDestroy(TrmMemoryPool hMemoryPool)
{
    // TODO: Check if the device is still using the memory pool. If it is, then wait for it to finish.

    struct TrmMemoryBlock_T* block = TRM_MEMORY_POOL->firstBlock;
    struct TrmMemoryBlock_T* nextBlock = NULL;
    while (block != NULL)
    {
        nextBlock = block->next;
        if (block->hMemoryHandle == NULL)
            free(block->startingAddress);
        else
        {
            if (block->startingAddress != NULL)
                vkUnmapMemory(block->hDevice, block->hMemoryHandle);

            if (block->miniBuff != NULL)
                free(block->miniBuff);

            vkFreeMemory(block->hDevice, block->hMemoryHandle, NULL);

            if (block->hBufferHandle != NULL)
                vkDestroyBuffer(block->hDevice, block->hBufferHandle, NULL);
        }
        free(block);
        block = nextBlock;
    }
    free(TRM_MEMORY_POOL);
}
