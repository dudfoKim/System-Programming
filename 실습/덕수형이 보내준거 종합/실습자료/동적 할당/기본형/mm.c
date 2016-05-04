/*
 * mm-implicit.c - an empty malloc package
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * @id : 200701960 
 * @name : KimDeakSoo
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
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/* Basic constants and macros */
#define WSIZE 4 /* word size (bytes) */
#define DSIZE 8 /* doubleword size (bytes) */
#define CHUNKSIZE (1<<12) /* initial heap size (bytes) */
#define OVERHEAD 8 /* overhead of header and footer (bytes) */
#define MAX(x, y) ((x) > (y)? (x) : (y))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
/* Read and write a word at address p */
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))
/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp; // function prototypes for internal helper routines

static void *extend_heap(size_t words);	// extend heap size
static void place(void *bp, size_t asize); // 
static void *find_fit(size_t asize);	// search free block
static void *coalesce(void *bp);	// coalesce free block

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {	// create empty heap
	if((heap_listp = mem_sbrk(4*WSIZE)) == NULL){ 
	// if lack memory place to create heap
		return -1;	// error
	}
	PUT(heap_listp, 0);				// alignment padding
	PUT(heap_listp+WSIZE, PACK(OVERHEAD, 1));	// prologue header
        PUT(heap_listp+DSIZE, PACK(OVERHEAD, 1));	// prologue footer
        PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));	// epilogue header
	
	// extend empty heap with a free block of CHUNKSIZE bytes.
	heap_listp += DSIZE;
	if(extend_heap(CHUNKSIZE/WSIZE)==NULL){	
	// if don't create heap
		return -1;	// error
	}
	return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
	size_t asize;	// Adjusted block size
	size_t extendsize;	// Amount to extend heap if no fit
	char *bp;	

	if(size == 0){	// (zero size) == (not memory allocation)
		return NULL;
	}
	// Adjust block size to include overhead and alignment reqs.
	if(size <= DSIZE){
		asize = DSIZE+OVERHEAD;
	}
	else{
		asize = DSIZE*((size+(OVERHEAD)+(DSIZE-1))/DSIZE);
	}

	// Search the free list for a fit
	if((bp=find_fit(asize))!=NULL){
		place(bp, asize);
		return bp;
	}

	// No fit found. Get more memory and place the block
	extendsize = MAX(asize, CHUNKSIZE);	// store bigger size
	if((bp = extend_heap(extendsize/WSIZE)) == NULL){
		return NULL;
	}
	place(bp, asize);
	return bp;
}

/*
 * free
*/
void free (void *bp) {
	size_t size;

	if(bp==NULL){	// bp==NULL mean not exist memory(heap).
		return ;
	}	
	size = GET_SIZE(HDRP(bp));

	// change alloc value (1->0)
	PUT(HDRP(bp), PACK(size,0));
	PUT(FTRP(bp), PACK(size,0));
	coalesce(bp);	// coalesce block close by free blocks.
}

/*
 * realloc - you may want to look at mm-naive.c
*/
void *realloc(void *oldptr, size_t size) {
	size_t oldsize;
	void *newptr;

	// if size==0 then this is just free, and we return NULL
	if(size==0){
		free(oldptr);
		return 0;
	}
	
	// if oldptr is NULL, then this is just malloc.
	if(oldptr==NULL){
		return malloc(size);
	}

	newptr = malloc(size);

	// if realloc() fails the original block is left untouched
	if(!newptr){
		return 0;
	}

	// Copy the old data.
	oldsize = GET_SIZE(oldptr);
	if(size<oldsize){
		oldsize = size;
	}
	memcpy(newptr, oldptr, oldsize);

	// Free the old block.
	free(oldptr);

	return newptr;
}

/* Extend heap */
static void *extend_heap(size_t words){
	char *bp;
	size_t size;

	// Allocate an even number of words to maintain alignment
	size = (words%2) ? (words+1)*WSIZE : words*WSIZE ;
	if((long)(bp = mem_sbrk(size)) == -1){
		return NULL;
	}

	// Initialize free block header/footer and the epilogue header
	PUT(HDRP(bp), PACK(size,0));	// free block header
	PUT(FTRP(bp), PACK(size,0));	// free block footer
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));	// new epilogue header

	// Coalesce if the previous block was free
	return coalesce(bp);
}

/* coalesce */
static void *coalesce(void *bp){
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size;
	size = GET_SIZE(HDRP(bp));

	if(prev_alloc && next_alloc){	// prev=alloc, next=allco
		return bp;	// just return block pointer
	}
	else if(prev_alloc && !next_alloc){	// prev=alloc, next=free
		// extend size(+next block size)
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size,0));
		PUT(FTRP(bp), PACK(size,0));
		return bp;
	}
	else if(!prev_alloc && next_alloc){	// prev=free, next=alloc
		// extend size(+prev block size)
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
		bp = PREV_BLKP(bp);
		return bp;
	}
	else{	// prev=free, next=free
		// extend size(+prev block and next block size)
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		size +=	GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
		bp = PREV_BLKP(bp);
		return bp;
	}
}

/* place */
// place block of asize bytes at start of free block bp
// and split if remainder would be at least minimum block size
static void place(void *bp, size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));
	if((csize - asize) >= (DSIZE + OVERHEAD)){
		PUT(HDRP(bp), PACK(asize,1));
		PUT(FTRP(bp), PACK(asize,1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
	}
	else{
		PUT(HDRP(bp), PACK(csize,1));
		PUT(FTRP(bp), PACK(csize,1));
	}
}

/* find_fit */
// find a fit for a block with asize bytes
static void* find_fit(size_t asize){
	void *bp;
	//first fit
	for(bp=heap_listp; GET_SIZE(HDRP(bp))>0; bp=NEXT_BLKP(bp)){
		if(!GET_ALLOC(HDRP(bp)) && (asize<=GET_SIZE(HDRP(bp)))){
			return bp;
		}
	}
	return NULL;
}

/* check block */
static void checkblock(void* bp){
	if((size_t)bp%8){	// check alignment memory size
		printf("Error: %p is not doubleword aligned\n",bp);
	}
	if(GET(HDRP(bp))!=GET(FTRP(bp))){ // check header and footer
		printf("Error: header does not match footer\n");
	}
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
    return NULL;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p < mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int verbose) {
}
