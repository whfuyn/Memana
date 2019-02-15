#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "memana.h"

#define dp(p) printf(#p " = %p\n", (p))
#define dd(d) printf(#d " = %I64d\n", (d))
#define ABS(size) ((size) >= 0 ? (size) : -(size))
#define PREV(pBlock) ((pBlock)->node.prev)
#define NEXT(pBlock) ((pBlock)->node.next)
#define BLOCK_MIN_SIZE (sizeof(Block) + sizeof(BlockSize_t))


// 初始化内存，在内存头部写入可用内存大小和空闲链表表头地址
void Initialize(void* space, BlockSize_t size)
{
    assert(size >= sizeof(Block*) + 2 * sizeof(BlockSize_t) + sizeof(Block));

    // 写入可用内存大小（去掉开头的两个元数据后所剩大小)
    BlockSize_t* pSpaceSize = (BlockSize_t*) Seek(space, sizeof(Block*));
    *pSpaceSize = size - sizeof(Block*) - sizeof(BlockSize_t);

    // 写入第一个Block的地址，作为空闲链表表头
    Block** pHeadPtr = GetPtrToHeadPtr(space);
    *pHeadPtr = (Block*) Seek(space, sizeof(Block*) + sizeof(BlockSize_t));

    // 初始化第一个Block
    Block* pBlock = *pHeadPtr;
    PREV(pBlock) = NULL;
    NEXT(pBlock) = NULL;

    // 这个Block的可用大小要扣掉头尾两个存放size的空间
    BlockSize_t* pHeadSize = SeekHeadSize(pBlock);
    *pHeadSize = *pSpaceSize - 2 * sizeof(BlockSize_t);

    // 设置尾部的size，头尾size始终要相同
    BlockSize_t* pTailSize = SeekTailSize(pBlock);
    *pTailSize = *pHeadSize;

}


void* Malloc(void* space, BlockSize_t size)
{
    // 查找第一个空间足够的空闲块
    Block* head = *GetPtrToHeadPtr(space);
    Block* p = head;
    while(p && p->size < size)
        p = NEXT(p);
    if(p == NULL)
        return NULL;

    // 如果这个空闲块拥有的空间多于所需空间+插入一个新节点所用空间,
    // 就插入一个新的节点把这个块拆分成两个。
    // 否则将整个块给出去。
    Block *prev = PREV(p), *next = NEXT(p);
    if(p->size >= size + BLOCK_MIN_SIZE)
    {

        BlockSize_t* pHeadSize = SeekHeadSize(p);
        BlockSize_t* pTailSize = SeekTailSize(p);
        assert(*pHeadSize == *pTailSize);

        // 将尾部的size设置为新节点所拥有的空间
        // 然后通过它来找到新节点头部size的位置，并同步大小
        // 这样新的节点就创建完成了
        // 但要分配出去的这个节点还没有处理完
        *pTailSize = p->size - size - 2 * sizeof(BlockSize_t);
        *SeekHeadSizeFromTailSize(pTailSize) = *pTailSize;

        // 处理这个要分配出去的节点
        // 将它头部size大小设置为请求的size大小
        // 同步它的尾部size
        // 将这个块设置为已使用(即头尾size都置为负数)
        *pHeadSize = size;
        *SeekTailSize(p) = *pHeadSize;
        SetBlockUsed(p);

        // 用这个新的节点取代原先节点在空闲链表中的位置
        Block* q = SeekBlockFromTailSize(pTailSize);

        if(prev)
            NEXT(prev) = q;
        if(next)
            PREV(next) = q;

        PREV(q) = prev;
        NEXT(q) = next;

        // 如果这个被取代的节点是首节点，则把新节点置为首节点
        if(prev == NULL)
            *GetPtrToHeadPtr(space) = q;
    }
    else
    {
        // 不插入新节点，直接将这个Block给出去
        // 分两步：
        // 1.将这个块置为已使用
        // 2.将这个块从空闲链表上取下
        SetBlockUsed(p);
        TakeOffBlock(space, p);
    }
    // 将头部size和尾部size中间的内存作为分配给用户的内存
    return (void*) p->data;
}

void Free(void* space, void* ptr)
{
    if(ptr == NULL)
        return;

    // 找到分配出去的这个块，并设置为未使用
    Block* curr = SeekBlockFromData(ptr);
    SetBlockUnused(curr);

    // 将这个块与内存上连续的前后相邻的未使用块合并
    // MergeAdjacentBlocks返回合并后的块
    // 如果这个块和前面的空闲块合并，那么释放已经完成
    // 如果它没有与前面的块合并，那么我们需要把它挂回空闲链表
    if(MergeAdjacentBlocks(space, curr) != curr)
        return;

    Block* head = *GetPtrToHeadPtr(space);

    // 直接挂载到头部
    if(head){
        assert(PREV(head) == NULL);
        PREV(head) = curr;
        NEXT(curr) = head;
        PREV(curr) = NULL;
    }
    else{
        PREV(curr) = NULL;
        NEXT(curr) = NULL;
    }
    // 将这个块设置为空闲链表表头
    *GetPtrToHeadPtr(space) = curr;
}





