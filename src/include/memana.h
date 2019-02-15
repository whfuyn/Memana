#ifndef MEMANA_H_
#define MEMANA_H_


typedef long long BlockSize_t;

struct Block_;

typedef struct Block_ Block;

struct Node
{
    Block* prev;
    Block* next;
};


struct Block_
{
    BlockSize_t size;
    union{
        struct Node node;
        char data[1];
    };
};

Block** GetPtrToHeadPtr(void* space);
BlockSize_t GetSpaceSize(void* space);

void* Seek(void* ptr, BlockSize_t offset);

BlockSize_t* SeekHeadSize(Block* pBlock);
BlockSize_t* SeekTailSize(Block* pBlock);

BlockSize_t* SeekHeadSizeFromTailSize(BlockSize_t* pTailSize);

Block* SeekBlockFromTailSize(BlockSize_t* pTailSize);
Block* SeekBlockFromData(void* ptr);


Block* SeekNextBlock(void* space, Block* curr);
Block* SeekPrevBlock(void* space, Block* curr);

void SetBlockUnused(Block* curr);
void SetBlockUsed(Block* curr);


void TakeOffBlock(void* space, Block* curr);

Block* MergeAdjacentBlocks(void* space, Block* curr);

void Initialize(void* space, BlockSize_t size);
void* Malloc(void* space, BlockSize_t size);
void Free(void* space, void* ptr);



#endif

