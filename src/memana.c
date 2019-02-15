#include <stdio.h>
#include <assert.h>
#include "memana.h"


#define dp(p) printf(#p " = %p\n", (p))
#define dd(d) printf(#d " = %I64d\n", (d))
#define ABS(size) ((size) >= 0 ? (size) : -(size))
#define PREV(pBlock) ((pBlock)->node.prev)
#define NEXT(pBlock) ((pBlock)->node.next)
#define BLOCK_MIN_SIZE (sizeof(Block) + sizeof(BlockSize_t))


// 返回指向空闲链表表头指针的指针
Block** GetPtrToHeadPtr(void* space)
{
    return (Block**) space;
}

// 返回可用内存大小
BlockSize_t GetSpaceSize(void* space)
{
    return *(BlockSize_t*) Seek(space, sizeof(Block*));
}

// 指针寻址
void* Seek(void* ptr, BlockSize_t offset)
{
    return (void*)((char*)ptr + offset);
}

// 返回指向该块首部size的指针
BlockSize_t* SeekHeadSize(Block* pBlock)
{
    return (BlockSize_t*) Seek(pBlock, 0);
}

// 返回指向该块尾部size的指针
BlockSize_t* SeekTailSize(Block* pBlock)
{
    return (BlockSize_t*) Seek(pBlock, sizeof(BlockSize_t) + ABS(pBlock->size));
}

// 从指向尾部size的指针反找到指向首部size的指针
BlockSize_t* SeekHeadSizeFromTailSize(BlockSize_t* pTailSize)
{
    return (BlockSize_t*) Seek(pTailSize, -(ABS(*pTailSize) + sizeof(BlockSize_t)));
}

// 从指向尾部size的指针反找到这个块
Block* SeekBlockFromTailSize(BlockSize_t* pTailSize)
{
    return (Block*) SeekHeadSizeFromTailSize(pTailSize);
}

// 从分配出去的内存指针找到它所属的块
Block* SeekBlockFromData(void* ptr)
{
    return (Block*) Seek(ptr, -sizeof(BlockSize_t));
}

// 找到当前块内存上连续相邻的前一个块
Block* SeekPrevBlock(void* space, Block* curr)
{
    char* begin = (char*) Seek(space, sizeof(Block*) + sizeof(BlockSize_t));
    // 判断是否已经是最前面的块
    BlockSize_t diff = (char*)curr - begin;
    if(diff == 0)
        return NULL;
    Block* prev = SeekBlockFromTailSize((BlockSize_t*)Seek(curr, -sizeof(BlockSize_t)));
    if(prev->size < 0)
        return NULL;
    return prev;
}

// 找到当前块内存上连续相邻的后一个块
Block* SeekNextBlock(void* space, Block* curr)
{
    char* begin = (char*) Seek(space, sizeof(Block*) + sizeof(BlockSize_t));
    // 判断是否已经是最后面的块
    BlockSize_t size = GetSpaceSize(space);
    BlockSize_t diff = (char*)curr - begin;
    if(size == diff + 2 * sizeof(BlockSize_t) + curr->size)
        return NULL;
    Block* next = (Block*) Seek(curr, 2 * sizeof(BlockSize_t) + curr->size);
    if(next->size < 0)
        return NULL;
    return next;
}

// 将当前块置为已使用
void SetBlockUnused(Block* curr)
{
    BlockSize_t* pHeadSize = SeekHeadSize(curr);
    BlockSize_t* pTailSize = SeekTailSize(curr);

    // size为负代表已使用
    assert(*pHeadSize <= 0);
    assert(*pHeadSize == *pTailSize);

    *pHeadSize = -*pHeadSize;
    *pTailSize = -*pTailSize;
}

// 将当前块置为未使用
void SetBlockUsed(Block* curr)
{
    BlockSize_t* pHeadSize = SeekHeadSize(curr);
    BlockSize_t* pTailSize = SeekTailSize(curr);

    // size为正代表已使用
    assert(*pHeadSize >= 0);
    assert(*pHeadSize == *pTailSize);

    *pHeadSize = -*pHeadSize;
    *pTailSize = -*pTailSize;
}

// 将当前块从空闲链表中摘下
void TakeOffBlock(void* space, Block* curr)
{
    Block* prev = PREV(curr);
    Block* next = NEXT(curr);
    if(prev)
        NEXT(prev) = next;
    if(next)
        PREV(next) = prev;
    if(prev == NULL)
        *GetPtrToHeadPtr(space) = next;
    // 循环链表
    if(prev == curr)
        *GetPtrToHeadPtr(space) = NULL;
}

// 将当前块与内存上连续前后相邻的块合并
Block* MergeAdjacentBlocks(void* space, Block* curr)
{
    Block* prev = SeekPrevBlock(space, curr);
    if(prev)
    {
        // 将前一个块的空间扩张，合并当前块
        // 首先设置块首部size的大小，然后让尾部size大小同步
        prev->size += 2 * sizeof(BlockSize_t) + curr->size;
        *SeekTailSize(prev) = prev->size;
        curr = prev;
    }
    Block* next = SeekNextBlock(space, curr);
    if(next)
    {
        // 将当前块的空间扩张，合并后一个块
        // 首先设置块首部size的大小，然后让尾部size大小同步
        TakeOffBlock(space, next);
        curr->size += 2 * sizeof(BlockSize_t) + next->size;
        *SeekTailSize(curr) = curr->size;
    }
    return curr;
}


