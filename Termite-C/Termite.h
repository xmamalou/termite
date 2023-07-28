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

#ifndef TRM_TERMITE_H
#define TRM_TERMITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>

#define TRM_VERSION(major, minor, patch) (((unsigned int)major << 16) | ((unsigned int)minor << 8) | (unsigned int)patch)

#define TRM_LIBRARY_VERSION TRM_VERSION(0, 1, 0)

/* -------------------- *
 *   ERROR CODES        *
 * -------------------- */

#define TRM_SUCCESS 0 // operation was successful

// errors are < 0.
/*
* Errors are a 2-byte integer, laid out in hex as follows:
*
* The first digit (from the left) is the general category of the error. For example, 0x1XXX is a generic error, 0x2XXX is a GLFW error, etc.
* The second digit is a more specified category of the error.
*/
#define TRM_GENERIC_OOM_ERROR -0x1001
#define TRM_GENERIC_NO_SUCH_FILE_ERROR -0x1002
#define TRM_GENERIC_OUT_OF_BOUNDS_ERROR -0x1003 // attempted to create more items than the maximum allowed
#define TRM_GENERIC_ALREADY_INITIALIZED_ERROR -0x1004 // the item is already initialized

#define TRM_MEMORY_UNAVAILABLE_BLOCKS_ERROR -0x2101 // no more memory blocks available
#define TRM_MEMORY_OOM_ERROR -0x2102 // no more memory available from pool

#define TRM_VULKAN_DEVICE_NO_MEMORY_ERROR -0x3201 // vulkan device has no memory available for allocation
#define TRM_VULKAN_DEVICE_UNMAPPABLE_MEMORY_ERROR -0x3201 // vulkan device couldn't map memory to host.
#define TRM_VULKAN_DEVICE_BUFFER_CREATION_ERROR -0x3201 // vulkan device couldn't create buffer
#define TRM_VULKAN_DEVICE_COMMAND_CREATION_ERROR -0x3201 // vulkan device couldn't create command buffer


// other definitions
#define TRM_MAX_CHAR_COUNT 256 // the maximum number of characters that can be used in a string
#define TRM_MAX_ITEM_COUNT 64 // the maximum number of items that can be used in an array (it's a safe amount for objects that are likely to not be numerous in an application)

/* -------------------- *
 *       HANDLES        *
 * -------------------- */
#define TRM_MAKE_HANDLE(type) typedef struct type##_HND* type; // a handle to a type

// opaque handle for a TrmMemoryPool_T object
TRM_MAKE_HANDLE(TrmMemoryPool);
// opaque handle for a TrmBuffer_T object
TRM_MAKE_HANDLE(TrmBuffer);

/* ================================ *
 *             MEMORY               *
 * ================================ */

 /* -------------------- *
  *   TYPES              *
  * -------------------- */

struct TrmVkDeviceMemInfo
{
    VkDeviceSize size; // size of memory, in bytes
    uint32_t     memoryHeapIndex; // the index of the heap the memory belongs to
    bool         isHostVisible; // is the memory visible to the host? 
};

struct TrmMemoryPoolInfo
{
    uint64_t size; // size of pool, in 4-byte words

    VkDevice device; // set to point to a Vulkan device if the memory pool should be allocated from the device
    bool useShared; // set to true if the memory pool shouldn't be local to the device

    uint32_t                   localHeapCount; // how many local heaps there are
    struct TrmVkDeviceMemInfo* pLocalHeaps; // the local heaps

    uint32_t                   sharedHeapCount; // how many shared heaps there are
    struct TrmVkDeviceMemInfo* pSharedHeaps; // the shared heaps
};

struct TrmQueueInfo
{
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    VkQueue* pQueues;
};

struct TrmBufferInfo
{
    uint64_t size; // size of buffer, in 4-byte words

    VkDevice             device;
    VkCommandPool        commandPool;
    struct TrmQueueInfo* pQueuesInfo;
};

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

/*
* @brief Initialize a memory pool.
*/
TrmMemoryPool trmMemoryPoolCreate(struct TrmMemoryPoolInfo* pInfo);

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

/*
* @brief Expand a memory pool.
*/
void trmMemoryPoolExpand(struct TrmMemoryPoolInfo* pInfo, TrmMemoryPool hMemoryPool);

/*
* @brief Allocate memory from a pool for a buffer.
*
* @param size: The size of the buffer, in 4-byte words
*/
TrmBuffer trmAllocate(struct TrmBufferInfo* pBufferInfo, TrmMemoryPool hMemoryPool);

/*
* @brief Reallocate memory from a pool for a buffer.
*
* @param hBuffer: The buffer to reallocate memory for.
*/
void trmReallocate(struct TrmBufferInfo* pBufferInfo, TrmBuffer hBuffer, TrmMemoryPool hMemoryPool);

/*
* @brief Free the memory of a buffer.
*
*/
void trmFree(TrmBuffer hBuffer, TrmMemoryPool hMemoryPool);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

/*
* @brief Get the size of a memory pool in BYTES.
*/
extern inline int trmMemoryPoolSizeGet(TrmMemoryPool hMemoryPool);

/*
* @brief Get the amount of blocks in a memory pool.
*/
extern inline int trmMemoryPoolBlockCountGet(TrmMemoryPool hMemoryPool);

extern inline int trmMemoryPoolErrorGet(TrmMemoryPool hMemoryPool);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

/*
* @brief Destroy a memory pool. Also frees the memory allocated for it.
* BEWARE! If the memory pool has been created or expanded using a Vulkan device, you must destroy the pool BEFORE destroying the device.
* Doing otherwise WILL result in a crash.
*
* @param hMemoryPool: The memory pool to destroy.
*/
void trmMemoryPoolDestroy(TrmMemoryPool hMemoryPool);

#ifdef __cplusplus
}
#endif

#endif