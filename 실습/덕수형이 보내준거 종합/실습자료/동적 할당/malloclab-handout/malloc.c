#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include "mm.h"
#include "memlib.h"

#ifdef DRIVER
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif 

#define WSIZE 4
#define OVERHEAD  8
#define DSIZE 8
#define PACK(size,alloc) ((size)|(alloc))
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define FLSB(bp)       ((char *)(bp) )
#define FLPB(bp)       ((char *)(bp) + WSIZE)
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
#define AD(bp) ( ( (char*)bp - start)/8)		


void extend(size _t asize);
void place(void *bp,size_t asize);
void *malloc(size_t size);
static void find_fit(size_t asize);
static void merge(void *bp , size_t size, int state);
static void free(void *bp);
static void merge_case(void *bp,int state);

void *init;			//list
void *last;			//last
void *start;
int free_count;

//Init
int mm_init(void)
{
	start = mem_sbrk(0);
	free_count = 0;
	init = NULL;
	bp = mem_sbrk(DSIZE*4);
	PUT(HDRP(bp),PACK(DSIZE*2,1));
	PUT(FTRP(bp),PACK(DSIZE*2,1));
	bp = NEXT_BLKP(bp);
	last = bp;
	PUT(HDRP(bp),PACK(DSIZE*2,1));
	PUT(FTRP(bp),PACK(DSIZE*2,1));
}
void *malloc(size_t size)
{
	while(1)
	{
		if( free_count )			//exist
		{
			if (  bp = find_fit(asize) )
				return place(bp,asize);
			else
				extend(asize);
		}
		else
			extend(asize);
	}
}
void find_fit(size_t asize)
{
	void *bp;
	for ( bp = init ; bp != NULL ; bp = GET(FLSB(bp))) )
	{
		if ( GET_SIZE(HDRP(bp)) >= asize)
			return bp;
	}
		return NULL;
}
void place(void *bp,size_t asize)
{
	void *p = GET(FLPB(bp));		//preview free_block
	void *n = GET(FLSB(bp));		//next free_block

	PUT(HDRP(bp),PACK(asize,0));
	PUT(FTRP(bp),PACK(asize,0));
	bp = NEXT_BLKP(bp);				//split block

	if ( p == &prev)
	{
		PUT(FLSB(bp),prev);
		PUT(FLPB(bp),&prev);
		prev = bp;
	}
	else
	{
		PUT(FLSB(bp),n);
		PUT(FLPB(bp),p);
		PUT(FLSB(p),bp);
	}
	if ( n == NULL )
		PUT(FLSB(bp),NULL);
	else
		PUT(FLPB(n),bp);
}
void extend(size _t asize)
{
	void *cp;
	bp = mem_sbrk(asize);
	PUT(HDRP(bp-DSIZE),PACK(asize-DSIZE*2,0));
	PUT(FTRP(bp-DSIZE),PACK(asize-DSIZE*2,0));
	cp = PREV_BLKP(bp-DSIZE);
	bp = NEXT_BLKP(bp-DSIZE);
	PUT(HDRP(bp),PACK(DSIZE*2,1));			//dumy block
	PUT(FTRP(bp),PACK(DSIZE*2,1));			//dumy block
	if ( GET_ALLOC(HDRP(cp)) )
		merge(bp,asize,1);			//AXA
	else
		merge(bp,asize,3);			//FXA
}
void *realloc(void *oldptr, size_t size)
(
	size_t oldsize=GET_SIZE(HDRP(oldptr));
	void *newptr;
	if ( size <= 0 )
	{
		free(oldptr);
		return 0;
	}
	if ( oldptr == NULL)
		return malloc(size);
	if (size == oldsize)
		return oldptr;
	newptr = malloc(size);
	if( size < oldsize)
		oldsize = size;

	memcpy(newptr,oldptr,oldsize);
	free(oldptr);
	return newptr;
	
}
void *calloc(size_t nmemb, size_t size)
{
	return NULL;
}

void free(void *bp)
{
	void *prev = PREV_BLKP(bp);		//prev block 
	void *next = NEXT_BLKP(bp);		//next block 
	int next_alloc_state = GET_ALLOC(HDRP(next));		//decide block state
	int prev_alloc_state = GET_ALLOC(HDRP(prev));	   //decide block state
	int size = GET_SIZE(HDRP(bp)));

	if ( prev_alloc_state && next_alloc_state )
	{
		merge(bp,size,1);
		return ;
	}
	else if ( prev_alloc_state && !next_alloc_state)
	{
		merge(bp,size,2);
	return ;
	}
	else if ( !prev_alloc_state && next_alloc_state)
	{
		merge(bp,size,3);
		return ;
	}
	else if ( !prev_alloc_state && !next_alloc_state)
	{
		merge(bp,size,4);
		return ;
	}
}
void merge(void *bp , size_t size, int state)
{
//	void *prev_block = PREV_BLKP(bp);
//	void *next_block = NEXT_BLKP(bp);
	switch (state)		//X means current blcok
	{
	case 1:		//AXA
		{
			PUT(HDRP(bp),PACK(size,0));
			PUT(FTRP(bp),PACK(size,0));
			merge_case(bp,1);
			return;
		}
	case 2:		//AXF
		{
			PUT(HDRP(bp),PACK(size,0));
			PUT(FTRP(bp),PACK(size,0));
			merge_case(bp,2);
			return ;
		}
	case 3:		//FXA
		{
			PUT(HDRP(bp),PACK(size,0));
			PUT(FTRP(bp),PACK(size,0));
			merge_case(bp,3);
			return ;
		}
	case 4:		//FXF
		{
			PUT(HDRP(bp),PACK(size,0));
			PUT(FTRP(bp),PACK(size,0));
			merge_case(bp,4);
			return ;
		}
	}
}
static void merge_case(void *current_block,int state)
{
	void *prev_block = PREV_BLKP(current_block);
	void *next_block = NEXT_BLKP(current_block);
	int prev_size = GET_SIZE(HDRP(prev_block));
	int next_size = GET_SIZE(HDRP(next_block));
	int current_size = GET_SIZE(HDRP(current_block));
	switch ( state )
	{
	case 1://AXA
		{	
			PUT(FLPB(current_block),&init);
			PUT(FLSB(current_block),init);
			if ( free_count )
				PUT(FLPB(GET(init)),current_block);
			init = current_block;
			return ;
		}
	case 2://AXF
		{
			PUT(HDRP(current_block),PACK(next_size+current_size,0));
			PUT(FTRP(next_block),PACK(next_size+current_size,0));
			return ;
		}
	case 3://FXA
		{
			PUT(HDRP(prev_block),PACK(prev_size+current_size,0));
			PUT(FTRP(current_block),PACK(prev_size+current_size,0));
			return ;
		}
	case 4://FXF
		{
			PUT(HDRP(prev_block),PACK(prev_size+current_size+next_size,0));
			PUT(FTRP(next_block),PACK(prev_size+current_size+next_size,0));
			return ;
		}
	}
}

	


		
	






