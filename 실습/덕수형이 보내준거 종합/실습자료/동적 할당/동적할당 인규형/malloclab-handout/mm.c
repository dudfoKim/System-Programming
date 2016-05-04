/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  Blocks are never coalesced or reused.  The size of
 * a block is found at the first aligned word before the block (we need
 * it for realloc).
 *
 * This code is correct and blazingly fast, but very bad usage-wise since
 * it never frees anything.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define SIZE_PTR(p)  ((size_t*)(((char*)(p)) - SIZE_T_SIZE))



/* Basic constants and macros */
#define WSIZE 	4 				/* word size (bytes) */
#define DSIZE	8 				/* doubleword size (bytes) */
#define CHUNKSIZE (1<<5) 		/* initial heap size (bytes) */
#define OVERHEAD 8 				/* overhead of header and footer (bytes) */
#define MAX	(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) 	((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)			(*(size_t *)(p))
#define PUT(p, val) 	(*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) 	(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


unsigned char* heap_listp = NULL;			// heap 포인터.

void*	find_fit(size_t asize);
void*	coalesce(void* bp);					//	이웃 블럭 합치기.
void	split(void* bp, size_t asize);		//	empty space를 분할 시킨다.
void*	extend_heap(size_t size);			// 	heap 확장.
void	log_heap();							//	힙의 상태 출력.

/*
 * mm_init - Called when a new trace starts.
 */

int mm_init(void)
{
	/* create the initial empty heap */
	mem_init();
	
	if (NULL == (heap_listp = mem_sbrk(CHUNKSIZE + DSIZE)))
	{
		return -1;
	}

	// 할당받은 포인터는 8의 배수이어야 한다.. 
	PUT(heap_listp, (int)0);								// heap의 최상위 부분을 0으로 처리.
	PUT(heap_listp + CHUNKSIZE + WSIZE, (int)0);			// Heap의 최하위 부부늘 0으로 처리.
	
	heap_listp += WSIZE;
	PUT(heap_listp, PACK(CHUNKSIZE, 0)); 						/* prologue header */
	PUT(heap_listp + CHUNKSIZE - WSIZE, PACK(CHUNKSIZE, 0));	/* prologue footer */
	
	heap_listp += WSIZE;
	return 0;
}

/*
 * malloc - Allocate a block by incrementing the brk pointer.
 *      Always allocate a block whose size is a multiple of the alignment.
 */
void *malloc(size_t size)
{
	// malloc 할 크기 계산.
	size_t malloc_size = ALIGN(size + ALIGNMENT);
	
	// 해당 fit 찾기.
	unsigned char* bp = find_fit(malloc_size);


	if (NULL != bp)
	{
		// 블럭을 쪼갠다.
		split(bp, malloc_size);
	}
	else
	{
		// 힙의 크기가 부족하면힙을 확장 시킨다.
		bp = extend_heap(malloc_size);

		// 블럭 합치기.
		coalesce(bp);
 
		// 사이즈에 맞는 블럭을 찾는다.
		bp = find_fit(malloc_size);
		
		// 블럭을 쪼갠다..
		split(bp, malloc_size);
	}

	return bp;
}

/*
 * free - We don't know how to free a block.  So we ignore this call.
 *      Computers have big memories; surely it won't be a problem.
 */
void free(void *ptr)
{
	if (NULL != ptr)
	{
		PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0x0));
		PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0x0));

		if (ptr < heap_listp)
		{
			heap_listp =  ptr;
		}
		// free 블록이 현재 heap_listp 보다 앞에있으면 heap_listp 재생신..
		unsigned char* bp = coalesce(ptr);

		if (bp < heap_listp)
		{
			heap_listp = bp;
		}
	}
}


/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.  I'm too lazy
 *      to do better.
 */
void *realloc(void *oldptr, size_t size)
{
	size_t oldsize;
	void *newptr;

 	 /* If size == 0 then this is just free, and we return NULL. */
  	if(size == 0) 
	{
		free(oldptr);
		return 0;
	}

	/* If oldptr is NULL, then this is just malloc. */
	if(oldptr == NULL)
	{
		return malloc(size);
	}

	newptr = malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if(!newptr)
	{
		return 0;
	}

	/* Copy the old data. */
	oldsize = GET_SIZE(HDRP(oldptr));
	if(size < oldsize) oldsize = size;
	memcpy(newptr, oldptr, oldsize);

	/* Free the old block. */
	free(oldptr);

	return newptr;
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size)
{
  size_t bytes = nmemb * size;
  void *newptr;

  newptr = malloc(bytes);
  memset(newptr, 0, bytes);

  return newptr;
}

/*
 * mm_checkheap - There are no bugs in my code, so I don't need to check,
 *      so nah!
 */
void mm_checkheap(int verbose)
{
		
}

// asize 크기에 맞는 빈블럭을 찾는다.
void *find_fit(size_t asize)
{
	void *bp = NULL;

	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
	{
		if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
		{
			return bp;
		}
	}

	return NULL; /* no fit */
}


// bg 가 가르키는 블럭을 asize만큼 분할 시킨다.
void    split(void* bp, size_t asize)     //  empty space를 분할 시킨다.
{
	// 빈공간에 할당후 남은 공간 계산.
	size_t empty_space = GET_SIZE(HDRP(bp)) - asize;


	// 빈공간이 16bit보다 작으면 쪼개지않고 해당 블럭을 그냥 통채로 사용한다..	
	// 너무 작은 조각으로 나눠지면 단편화 심해진다.
	if (empty_space < 16)
	{
		asize = GET_SIZE(HDRP(bp));
		empty_space = 0;
	}

	PUT(HDRP(bp), PACK(asize, 0x1));      /* alloc header */
	PUT(FTRP(bp), PACK(asize, 0x1));      /* alloc footer */

	// 남은 공간이 있으면....
	if (0 < empty_space)
	{
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(empty_space, 0x0));      /* prologue header */
		PUT(FTRP(bp), PACK(empty_space, 0x0));      /* prologue footer */

		heap_listp = bp;
	}
}


// 기존 힙을 size 만큼 확장한다.
void*	extend_heap(size_t size)
{
	unsigned char* bp = mem_sbrk(size);

	// 기존 힙의 마지막 부분의 0을 제거.
	bp -= WSIZE;	
	PUT(bp, PACK(size, 0));
	PUT(bp + size - WSIZE, PACK(size, 0));
	PUT(bp + size, (int)0);                 // heap의 최하위 부분을 0으로 처리.
    return bp + WSIZE;
}

// bp의 이웃 empty block을 합친다.
void* coalesce(void* bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	// CASE 1 [ 앞, 뒤 블록이 할당 된경우 ]
	if ((1 == prev_alloc) && (1 == next_alloc))
	{
		return bp;
	}
	// CASE 2 [ 앞의 블록만 할당 된 경우 ]
	else if (1 == prev_alloc)
	{
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

		PUT(HDRP(bp), PACK(size, 0));
		PUT(HDRP(bp) + size - WSIZE, PACK(size, 0));
		return bp;
	}
	// CASE 3 [ 뒤의 블록만 할당 된 경우 ]
	else if (1 == next_alloc)
	{
		if (bp != PREV_BLKP(bp))
		{
			bp = PREV_BLKP(bp);
			size += GET_SIZE(HDRP(bp));

			PUT(HDRP(bp), PACK(size, 0));
			PUT(HDRP(bp) + size - WSIZE, PACK(size, 0));
			return bp;
		}
	}
	// CASE 4 [ 모두다 할당된 경우 ]
	else
	{
		if ((bp != PREV_BLKP(bp) && (bp != NEXT_BLKP(bp))))
		{
			size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
						
			bp = PREV_BLKP(bp);

			PUT(HDRP(bp), PACK(size, 0));
			PUT(HDRP(bp) + size - WSIZE, PACK(size, 0));
			return bp;
		}
	}

	return bp;
}

void log_heap()
{
	// 현재 힙의 상태를 출력한다.
	printf("-------------- heap log--------------\n");

	void *bp = mem_heap_lo() + DSIZE;
	for ( ; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
	{
		printf("size : %d [ alloc %d ] p : %p\n", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), bp);

    }

	printf("------------------------------------\n");
}
