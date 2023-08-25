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

#ifndef TRM_INTERNAL_H
#define TRM_INTERNAL_H

#include "Termite.h"

#define TRM_HANDLE(type) ((struct Trm##type##_T*)h##type) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))

#define TRM_MEMORY_POOL TRM_HANDLE(MemoryPool)
#define TRM_BUFFER      TRM_HANDLE(Buffer)

#ifdef _WIN32
    #include <Windows.h>
#else 
    #include <pthread.h>
    #include <signal.h>
#endif

/* ================================ *
 *             MEMORY               *
 * ================================ */

struct TrmMemoryBlock_T
{
    uint64_t size; // in BYTES, not in 4-byte words like in dflMemoryBlockInit
    uint64_t used; // in BYTES, not in 4-byte words like in dflMemoryBlockInit
    void* startingAddress;

    uint64_t slots[TRM_MAX_ITEM_COUNT * 2]; // for 2n and (2n + 1), where n natural number, slots[2n] is the first available slot and slots[2n + 1] is the last available slot in a contiguous free part of the memory block.
    // this does mean that if there are more than 64 free non-contiguous parts, then the memory block will not be able to use some of them (forever), but I *think* it's unlikely that 
    // a user will free 64 non-contiguous parts of memory one after the other without allocating anything in between.

    VkDeviceMemory hMemoryHandle;
    VkBuffer       hBufferHandle; // the buffer associated with the block (if a device is used). Command buffers to place data into the buffer are created on allocation, as it's then when 
    // we know the size of what we want to allocate.
    VkDevice       hDevice; // the device associated with the block (if a device is used)
    void* miniBuff; // if memory is unmappable, then this will be a small (4 bytes) buffer that will be used to copy data to the vulkan buffer.

    struct DflMemoryBlock_T* next; // the next memory block
};

struct TrmMemoryPool_T
{
    uint64_t size; // in BYTES, not in 4-byte words like in dflMemoryPoolInit
    uint64_t used; // in BYTES, not in 4-byte words like in dflMemoryPoolInit

    struct DflMemoryBlock_T* firstBlock; // the first memory block

    int error;
};

struct TrmBufferChunk_T // chunks concern individual memory blocks and are part of a buffer
{
    struct TrmMemoryBlock_T* associatedBlock; // the memory block that this chunk is part of
    uint64_t size; // the size of the chunk
    uint64_t offset; // the offset of the chunk in the memory block

    VkCommandBuffer transferOp; // the tranfer operation used for unmapped buffers responsible for copying the data from the starting address to the vulkan buffer. 
    VkEvent         hostCanGetNextPart; // since the small host buffer for unmapped buffer is just 4 bytes, we need to signal to the host each time 4 bytes are transferred to the vulkan buffer so the host can copy the next 4 bytes to the small host buffer.
    VkFence         isTransferDone; // the fence will be signaled when the transfer operation is done.
};

struct TrmBuffer_T
{
    uint64_t size;

    // A buffer can create DFL_MAX_ITEM_COUNT chunks in total. A chunk is associated with a "slot" of available memory in blocks of the memory pool.
    // If the number of slots the buffer needs is more than DFL_MAX_ITEM_COUNT, then the buffer will only have DFL_MAX_ITEM_COUNT chunks and will be smaller than the requested size.
    // In such a case, the memory pool will report an error, although the buffer creation will, technically, be successful.
    struct TrmBufferChunk_T chunks[TRM_MAX_ITEM_COUNT];

    // DflBuffers won't have an error field, the memory pool will have buffer related errors reported in its error field instead.
};

/* ================================ *
 *            THREADS               *
 * ================================ */

struct TrmThread_T
{
   struct TrmThreadInfo info;
   
#ifdef _WIN32
   HANDLE hThread;
#else
   pthread_t hThread;
   int       id;
#endif

   int error;
};

#endif