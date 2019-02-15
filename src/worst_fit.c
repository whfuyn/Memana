#include <stdio.h>
#include <assert.h>
#include "memana.h"

#define dp(p) printf(#p " = %p\n", (p))
#define dd(d) printf(#d " = %I64d\n", (d))
#define ABS(size) ((size) >= 0 ? (size) : -(size))
#define PREV(pBlock) ((pBlock)->node.prev)
#define NEXT(pBlock) ((pBlock)->node.next)
#define BLOCK_MIN_SIZE (sizeof(Block) + sizeof(BlockSize_t))

// best_fit和worst_fit的差别只在这个排序上
// best_fit将空闲链表排列为从小到大
// worst_fit将空闲链表排列为从大到小


// 将块重排序，使得空闲链表内的块按空间大小从小到大排列
// static表示不导出符号
static void ReorderBlockDescending(void* space, Block* curr);


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
        // 然后调整块的位置
        // 注意重新排序块的函数可能但不一定会设置链表表头
        // (只有在因为重新排列使得某个块成为表头的时候才会设置表头)
        // 所以我们必须先将表头设置好，否则表头可能处于一种不正确的状态
        if(prev == NULL)
            *GetPtrToHeadPtr(space) = q;

        // 将块重排序，使得空闲链表内的块按空间大小从小到大排列
        ReorderBlockDescending(space, q);
    }
    else
    {
        // 不插入新节点，直接将这个Block给出去
        // 这样摘下不会改变空闲链表的有序性
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
    // 由于合并后空间会变大，我们重新调整它的位置
    // 如果它没有与前面的块合并，那么我们需要把它挂回空闲链表
    Block* pMergedBlock = MergeAdjacentBlocks(space, curr);
    if(pMergedBlock != curr)
    {
        ReorderBlockDescending(space, pMergedBlock);
        return;
    }

    Block* head = *GetPtrToHeadPtr(space);

    // 直接挂载到头部
    if(head){
        assert(PREV(head) == NULL);
        PREV(head) = curr;
        NEXT(curr) = head;
        PREV(curr) = NULL;

        // 注意重新排序块的函数可能但不一定会设置链表表头
        // (只有在因为重新排列使得某个块成为表头的时候才会设置表头)
        // 所以我们必须先将表头设置好，否则表头可能处于一种不正确的状态
        *GetPtrToHeadPtr(space) = curr;

        ReorderBlockDescending(space, curr);
    }
    else
    {
        PREV(curr) = NULL;
        NEXT(curr) = NULL;
        *GetPtrToHeadPtr(space) = curr;
    }
}



static void ReorderBlockDescending(void* space, Block* curr)
{
    Block* head = *GetPtrToHeadPtr(space);
    assert(PREV(head) == NULL);

    // 先向前调整块的位置
    Block* prev = PREV(curr);
    while(prev && prev->size < curr->size)
    {
        if(PREV(prev))
            NEXT(PREV(prev)) = curr;
        if(NEXT(curr))
            PREV(NEXT(curr)) = prev;

        PREV(curr) = PREV(prev);
        NEXT(prev) = NEXT(curr);

        NEXT(curr) = prev;
        PREV(prev) = curr;

        // 如果当前块被移动到链表首部，则将当前块置为表头
        if(PREV(curr) == NULL)
            *GetPtrToHeadPtr(space) = curr;

        // 继续向前调整
        prev = PREV(curr);
    }

    // 向后调整块的位置
    Block* next = NEXT(curr);
    while(next && next->size > curr->size)
    {
        if(PREV(curr))
            NEXT(PREV(curr)) = next;
        if(NEXT(next))
            PREV(NEXT(next)) = curr;

        PREV(next) = PREV(curr);
        NEXT(curr) = NEXT(next);

        NEXT(next) = curr;
        PREV(curr) = next;

        // 如果下一块被移动到链表首部，则将它置为表头
        if(PREV(next) == NULL)
            *GetPtrToHeadPtr(space) = next;

        // 继续向后调整
        next = NEXT(curr);
    }
}





