#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "memana.h"
#define maxn 1123456
#define PATH "data\\input.txt"
#define dbg(x) printf(#x " = %p\n", (x))
#define db(x) printf(#x " = %llu\n", (x))

#define BLOCK_MIN_SIZE (sizeof(Block) + sizeof(BlockSize_t))

typedef struct
{
    long long s; // arrived time
    long long t; // use time
    long long m;  // memory needed
    void * ptr;

} Request;

Request requests[maxn];


int main(void)
{
    puts("start");
    long long n, L;
    FILE* input = fopen(PATH, "r");
    assert(input != NULL);
    //FILE* input = stdin;
    fscanf(input, "%I64d %I64d", &n, &L);

    void * space = malloc(L * sizeof(char));
    assert(space != NULL);

    Initialize(space,  L * sizeof(char));

    long long tot = 0;
    for(int i = 0; i < n; i++)
    {
        Request * req = &requests[i];
        fscanf(input, "%I64d %I64d %I64d", &req->s, &req->t, &req->m);
        req->ptr = NULL;
        tot += req->m;
    }
    unsigned long long t = 0;
    bool finished = false;
    puts("start do");
    while(!finished)
    {
        finished = true;
        for(int i = 0; i < n; i++)
        {
            Request * req = &requests[i];
            if(req->m == 0) continue; // finished
            finished = false;
            if(req->ptr == NULL && t >= req->s){
                req->s = t;
                req->ptr = Malloc(space, req->m);
            }
            if(req->ptr != NULL && t >= req->s + req->t){
                Free(space, req->ptr), req->m = 0;
            }
        }
        t++;
    }
    t--;
    printf("time: %llu\n", t);

    return 0;
}


