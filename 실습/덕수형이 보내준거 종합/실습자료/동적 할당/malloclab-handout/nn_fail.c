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
#include <dlfcn.h>

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
/* rounds up to the nearest multiple of ALIGNMENT */
#define WSIZE 4 
#define DSIZE 8 
#define CHUNKSIZE (1<<8)
#define OVERHEAD 8 
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define PACK(size,alloc) ((size)|(alloc))
									 
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))
									 
#define GET_SIZE(p)  (GET(p) & ~0x7)								 
#define GET_ALLOC(p) (GET(p) & 0x1)
		 
#define HDRP(bp)       ((char *)(bp) - WSIZE)
									 
#define FLPB(bp)       ((char *)(bp) + 2*WSIZE)		/*프리 블럭에서 쓰임*/
#define FLSB(bp)       ((char *)(bp) +   WSIZE)
			
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
									 
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
									 
#define AD(bp) ( ( (char*)bp - start)/8) 


static char *heap_listp=NULL;
char *pred=NULL;
static char *last=NULL;
static char *start;// = mem_sbrk(0);

static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *extend_heap(size_t words,int i);
static void *coalesce(void *bp);
static void print(void *bp);
void free_sort(void *bp);
int get_free_count();
int first = 0;

/*
 * mm_init - Called when a new trace starts.
 */
int mm_init(void)
{
//	dbg_printf("\t\t\t\t\t\t\t\t\t\tnew trace\n");
//	dbg_printf("heap_start : %d\n",start);
//	dbg_printf("heap_last : %d\n",last);
	if ( last == NULL )
	{
		if ( start != NULL )
		{
			memcpy(start,0,AD(last));
			pred = NULL;
			first = 0;	
		}
	}
	else
	{
		pred = NULL;	
		first = 0;
		last = NULL;
	}
	return 0;
}
/*
 * malloc - Allocate a block by incrementing the brk pointer.
 *      Always allocate a block whose size is a multiple of the alignment.
 */


void *malloc(size_t size)
{
//	dbg_printf("\nmalloc\n");
//	dbg_printf("free block 's count : %d\n",get_free_count());
//	dbg_printf("last heap location : %d\n",AD(last));
//	print(pred);
	
	size_t asize;
	size_t extendsize;
	char *bp;
	if ( size <= 0 )
		return 0;
	if ( size <= DSIZE )
		asize = DSIZE + OVERHEAD;
	else
		asize = DSIZE * (( size + (OVERHEAD) + (DSIZE-1))/DSIZE);
	
//	dbg_printf("request size : %d\n",asize);
	if ( (bp = find_fit(asize))!= NULL ) 
	{
//		dbg_printf("find end\n");
//		dbg_printf("exist such as request size in free block \n");
		size = GET_SIZE(HDRP(bp));	//free_block's size;
		size = size - asize;		//extra size;
		
		void *tmp = bp; //tmp = bp;
		void *p = GET(FLPB(tmp));	//tmp's preview free_block
		//dbg_printf("prev : %d\n",AD(p));
		void *n = GET(FLSB(tmp));	//tmp's next free_block
//		dbg_printf("next : %d\n",AD(n));
		place(bp,asize);			//place 
		bp = NEXT_BLKP(bp);			//free_block 
		PUT(HDRP(bp),PACK(size,0));	//free state
		PUT(FTRP(bp),PACK(size,0)); //free state
		
//		dbg_printf("size : %d\n",size);
//		dbg_printf("&&&&&&&\n");
//		dbg_printf("tmp : %d\n",AD(tmp));
//		dbg_printf("tmp : %p\n",tmp);
//		dbg_printf("prev : %d\n",AD(p));
//		dbg_printf("prev : %p\n",p);
//		dbg_printf("next : %d\n",AD(n));
//		dbg_printf("next : %p\n",n);
//		dbg_printf("current %d\n",AD(bp));
//		dbg_printf("current %p\n",bp);
//		dbg_printf("&&&&&&&\n");
		
		//block have splited 
		if ( n == NULL )		//one free_block
		{
//			dbg_printf("*************************\n");
			PUT(FLSB((bp)),n);			//bp -> NULL;
			PUT(FLPB((bp)),(p));			//pred <- bp
			PUT(GET(FLSB(p)),bp);			            //pred -> bp -> NULL; , pred <- bp 
			if ( p == &pred)
			{
//				dbg_printf("!!!!!!!!!!!!!!!!!!\n");
				PUT(p,bp);
				PUT(FLPB(bp),&pred);
			}
			else
			{
//				dbg_printf("@@@@@@@@@@@@@@@@@\n");
//				dbg_printf("address %d\n",AD(bp));
//				dbg_printf("next %d\n",AD(GET(FLSB(bp))));
//				dbg_printf("prev %d\n",AD(p));
				PUT(FLSB((p)),bp);
//				dbg_printf("next %d\n",AD(GET(FLSB(bp))));
				
//				dbg_printf("pred %d\n",AD(pred));
			}
		}						
		else								//more two free_blocks
		{
//			dbg_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			PUT(FLSB(bp),(n));				
			PUT(FLPB(n),bp);
			PUT(FLPB(bp),(p));
			if ( p != &pred)
			{
//				dbg_printf("################\n");
				PUT((FLSB(p)),bp);
//				print(pred);
			}
			else
			{
//				dbg_printf("$$$$$$$$$$$$$$$$$$\n");
//				dbg_printf("bp : %d\n",AD(bp));
				pred = bp;
//				print(pred);
			}
			
		}
		return tmp;
	}
//	dbg_printf("none count of free_block or request size execss more than each free_block'size\n");	
//	dbg_printf("need size\n");
	bp = extend_heap(asize,1);				//request new heap
//	dbg_printf("request address : %d\n",AD(bp));
	place(bp,asize);
	return bp;
}
/*
 * free - We don't know how to free a block.  So we ignore this call.
 *      Computers have big memories; surely it won't be a problem.
 */
void free(void  *bp)
{
	if (bp )
	{
	
//		dbg_printf("\nfree\n");
		
//		dbg_printf("free_list \n");
//		print(pred);
//		dbg_printf("free_list end\n");
		
		size_t size = GET_SIZE(HDRP(bp));
//		memset(bp-WSIZE,0,size+DSIZE);
	//	dbg_printf("prew alloc %d , next alloc : %d\n",GET_ALLOC(HDRP(PREV_BLKP(bp))),GET_ALLOC(HDRP(NEXT_BLKP           (bp))));
		
	//	dbg_printf("          free size  : %d\n",size);
		memset(bp,0,size-WSIZE);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	//	dbg_printf("where is %d\n",AD(bp));
	//	dbg_printf("prew alloc : %d, next alloc : %d\n",GET_ALLOC(HDRP(PREV_BLKP(bp))),GET_ALLOC(HDRP(NEXT_BLKP(bp))));
		coalesce(bp);
//		dbg_printf("free list \n");
//		print(pred);
//		dbg_printf("free list end\n\n");
//		dbg_printf("free success\n");
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
//  dbg_printf("\nrealloc start\n");
  // If size == 0 then this is just free, and we return NULL. 
	if(size <=  0) {
    free(oldptr);
    return 0;
  }
  // If oldptr is NULL, then this is just malloc. 
  if(oldptr == NULL) {
    return malloc(size);
  }
  oldsize = GET_SIZE(HDRP(oldptr));
  if ( size == oldsize)
	  return oldptr;
  
  newptr = malloc(size);
  if(!newptr) {
    return 0;
   }
  // Copy the old data. 
  
  oldsize = GET_SIZE(HDRP(oldptr));
  if(size < oldsize) 
	  oldsize = size;
  memcpy(newptr, oldptr, oldsize);
  
  // Free the old block. 
  free(oldptr);
  
//  dbg_printf("realloc end\n");
  return newptr;
  //
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size)
{
	if ( size <= 0 || nmemb <= 0 )
		return 0;
	size_t tmp = nmemb + size;
	
	void *ptr;
	ptr = malloc(tmp);
	
	if ( ptr != 0)
		memset(ptr,0,tmp);
	else
		return 0;
	return ptr;
}
static void place(void *bp, size_t asize)
{
	if ( ( asize) >= DSIZE+OVERHEAD )	
	{ /*메모리가 약간 쪼개짐*/
	//	dbg_printf("             place\n");
	//	dbg_printf("allocation size : %d\n",asize);
		PUT(HDRP(bp),PACK(asize,1));
		PUT(FTRP(bp),PACK(asize,1));
	//	dbg_printf("			place end\n");
	}
}
static void *find_fit(size_t asize)
{
	void *bp;
	if ( pred == NULL )
		return NULL;
//	int i = 0 ;
	for ( bp = pred ; (bp)!=NULL  ; bp = GET(FLSB(bp)) )
	{
	//	dbg_printf("\tprev_address : %5d,\tfree Address : %5d\t, size : %5d,next free_Address %d\n",AD(GET(FLPB(bp))),AD(bp),GET_SIZE(HDRP(bp)),AD(GET(FLSB(bp))));
		if ((asize < GET_SIZE(HDRP(bp))))
			return bp;
	}
	return NULL;
}
static void *extend_heap(size_t words,int i)
{	
	//can't more than correct
	char *bp;
	size_t size;
	if ( first == 0 )
	{
		if ((int)(bp = mem_sbrk(DSIZE+words+DSIZE))<0)
			return NULL;
		first = 1;
		PUT(HDRP(bp),PACK(DSIZE,1));
		PUT(FTRP(bp),PACK(DSIZE,1));
		bp = NEXT_BLKP(bp);
		start =bp;
		PUT(HDRP(bp),PACK(words,0));
		PUT(FTRP(bp),PACK(words,0));
		PUT(HDRP(NEXT_BLKP(bp)),PACK(DSIZE,1));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(DSIZE,1));
		last = start + words;		//마지막 위치
		return bp;
	}
	else
	{
//		dbg_printf("last : %d\n",AD(last));
		if ((int)(bp = mem_sbrk(words)) < 0)
			return NULL;
//		dbg_printf("FLPB(last) : %d\n",AD(FLPB(last)));
//		dbg_printf("FLPB(last) : %d\n",AD(GET(FLPB(last-8))));
//		dbg_printf("alloc : %d\n",GET_ALLOC(HDRP(last)));
//		dbg_printf("alloc 2 : %d\n",GET_ALLOC(HDRP(GET(FLPB(last)))));
//		dbg_printf("GET_ALLOC(HDRP(last) : %d\n",GET_ALLOC(HDRP(last)));
//		dbg_printf("GET_ALLOC(HDRP(last+8) : %d\n",GET_ALLOC(HDRP(last+8)));
//		dbg_printf("last+8: %d\n",AD(last+8));
//		dbg_printf("last : %d\n",AD(last));;
//		dbg_printf("last free ? %d\n",GET_ALLOC(HDRP(last)));
//		dbg_printf("last-8: %d\n",AD(last-8));
//		dbg_printf("next : %d\n",AD(GET(FLSB(last+8))));
//		dbg_printf("prev : %d\n",AD((PREV_BLKP(last))));
//		dbg_printf("size : %d\n",GET_SIZE(HDRP(PREV_BLKP(last))));
//		dbg_printf("alloc : %d\n",GET_ALLOC(HDRP(PREV_BLKP(last))));
//		dbg_printf("alloc : %d\n",GET_ALLOC(HDRP(PREV_BLKP(PREV_BLKP(last)))));
//		void *z;
//		z = PREV_BLKP(last);
//		if ( GET_ALLOC(HDRP(last+8)) == 0)
//		{
//			PUT(HDRP(last+words),PACK(DSIZE,1));
//			PUT(FTRP(last+words),PACK(DSIZE,1));
//			bp = coalesce(bp);
//			dbg_printf("free");
//			void *p = (FLPB(last+8));
//			void *n = (FLSB(last+8));
//			if ( n == NULL )
//			{
///				dbg_printf("null\n");
//				print(pred);
//				PUT(FLSB(p),last+8+words);
//				PUT(FLSB(last+8+words),NULL);
///				PUT(FLPB(last+8+words),p);
//				print(pred);
///			}
//			else 
///			{
//				dbg_printf("not null\n");
//				print(pred);
//				PUT(FLSB(p),last+8+words);
//				PUT(FLPB(last+8+words),p);
//				PUT(FLSB(last+8+words),FLSB(last+8));
//				PUT(FLPB(last+8),last+8+words);
///				print(pred);
//			}
//		}
		PUT(HDRP(last), PACK(words, 0)); 
		PUT(FTRP(last), PACK(words, 0)); 
		bp = last;
		last = last + words;		//마지막 위치 
		PUT(HDRP(last),PACK(DSIZE,1));
		PUT(FTRP(last),PACK(DSIZE,1));
//		dbg_printf("z : %d\n",AD(z));
//		dbg_printf("bp : %d\n",AD(bp));
//		if ( z == bp )
//			dbg_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~zZZZZZZZZZZZZZ");
		return bp;
//		return coalesce(bp);
	}
}

static void *coalesce(void *bp)
{
//	dbg_printf("\ncall coalesce\n");
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	int flag = 0;
	void *current;
	void *next;
	void *prev;
 	if (prev_alloc==1  && (next_alloc == 1 || last == NEXT_BLKP(bp) ))//next_alloc == 2))
	{	//이 경우는 양쪽 모두 할당 되어있으니까 bp를 루트로 하는 구조로 한다
		//case1 완성 
//		dbg_printf("case1\n");
		if ( pred == NULL )		//pred->NULL
		{						//create new free_block list
			PUT(FLSB(bp),pred);
			PUT(FLPB(bp),&pred);
			pred = bp;
		}
		else					//exist one more free_block list
		{
			next = pred;	
			PUT(FLSB(bp),pred);	//bp->free_block
			PUT(FLPB(bp),&pred);//&pred<-bp
			PUT(FLPB(next),bp); //bp <- free_block
			pred = bp;			//pred -> bp;
		}
		return (bp);
	}
	else if ( prev_alloc==1 && !next_alloc)
	{
		//OK
		//이 경우는 오른쪽에 위치한 SB와 PB를 옮긴다.
//		dbg_printf("case2\n");
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		current = NEXT_BLKP(bp); //current means nearest free_block
		prev = GET(FLPB(current)); //prev means nearest prev free_block 
		next = GET(FLSB(current)); //next means neraest next free_block
		
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP((bp)), PACK(size,0));
//		dbg_printf("size : %d\n\n",size);
		
		if ( pred == NULL ) //처음 작업
		{
			PUT(FLPB(bp),&pred);
			PUT(FLSB(bp),NULL);
			pred = bp;
		}
		else
		{
			if ( next == NULL )
			{
				PUT(FLSB(bp),NULL);
				PUT(FLPB(bp),prev);
				if ( prev != &pred)
					PUT(FLSB(prev),bp);
				else
					PUT(prev,bp);
			}
			else
			{
				PUT(FLSB(bp),next);
				PUT(FLPB(bp),prev);
				if ( prev != &pred)
					PUT(FLSB(prev),bp);
				else
					PUT(prev,bp);
				PUT(FLPB(next),bp);
			}
				
		}
			return (bp);
	}
	else if (prev_alloc==0 && next_alloc == 1)
	{
		//이경우에는 블록 설정을 할 필요 가 없다 왜냐하면 왼쪽에 위치 한거는 어떻것을 가르키므로 따라서 뭘 할필요가 없다
		//case3 ok
//		dbg_printf("case3\n");
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		prev = PREV_BLKP(bp);
		PUT(FTRP(bp),PACK(size,0));
		PUT(HDRP(prev),PACK(size,0));
		
//		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		return  (PREV_BLKP(bp));
	}
	else if (prev_alloc == 0 && next_alloc == 0)
	{
		//최악의 경우로서 현재 블록의 갯수 만큼 선회해서 리스트를 다시 만들어야 함
		
//		dbg_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!case4!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
//		dbg_printf("prev %d,current %d,next %d\n",AD(PREV_BLKP(bp)),AD(bp)),AD(NEXT_BLKP(bp));
//		dbg_printf("prev %p,current %p,next %p\n",PREV_BLKP(bp),bp,NEXT_BLKP(bp));
//		dbg_printf("size %d\n",size);
//		dbg_printf("next size : %d\n",GET_SIZE(HDRP(NEXT_BLKP(bp))));
//		dbg_printf("prev size : %d\n",GET_SIZE(HDRP(PREV_BLKP(bp))));
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		free_sort(bp);		
		return  (PREV_BLKP(bp));
	}
//	dbg_printf("case 5\n");
	return bp;
}
static void print(void *bp)
{
	if ( bp != NULL  )
	{
//		dbg_printf("alloc : %d,prve : %d, current : %d next :%d, current_size  : %d\n",GET_ALLOC(HDRP(bp)),AD(GET(FLPB(bp))),AD(bp),AD(GET(FLSB(bp))),GET_SIZE(HDRP(bp)));
		print( GET(FLSB(bp)) );
	}
}	

void free_sort(void *bp)
{
//	dbg_printf("free_sort\n");
	void *tmp = NEXT_BLKP(start);				//첫번째 블럭
	void *tmp2 = NEXT_BLKP(tmp);	//두번째 블럭
	void *free;

	PUT(FLPB(tmp),&pred);
	int f = 1;
	int flag = 0;
//	dbg_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~free_block~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	while ( last != tmp )//GET_ALLOC(HDRP(tmp)) != 1)
	{
	if ( GET_ALLOC(HDRP(tmp)) == 0 ) //다음 블록이 프리 인 경우
		{
			while( last != tmp2 )//GET_ALLOC(HDRP(tmp2)) != 1)
			{
				if ( GET_ALLOC(HDRP(tmp2)) == 0 )
					break;
				else
					tmp2 = NEXT_BLKP(tmp2);
			}
			if ( tmp2 == tmp )
			{
				tmp2 = NEXT_BLKP(tmp2);
			}

			else
			{
				
				if ( flag == 0 )
				{
					flag = 1;
					PUT(FLPB(tmp),&pred);
					pred = tmp;
				}
				if ( (tmp2) != 0 )
				{
//					dbg_printf("size : %d, ad : %d\n",GET_SIZE(HDRP(tmp)),AD(tmp));
//					dbg_printf("size : %d, ad : %d\n\n",GET_SIZE(HDRP(tmp2)),AD(tmp2));
					PUT(FLSB(tmp),tmp2);
					PUT(FLPB(tmp2),tmp);
				}
				if ( tmp2 == last )
					break;
					tmp  = GET(FLSB(tmp));
					tmp2 = NEXT_BLKP(tmp2);
				
			}
		}
		else
			tmp = NEXT_BLKP((tmp));
	}
	
//	dbg_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
//	dbg_printf("free : %d\n",AD((free)));
//	dbg_printf("2:%d\n",AD(GET(tmp2)));
	if ( flag == 0 ) // means exist two block
	{
//		dbg_printf("prev bp : %d\n",AD(PREV_BLKP(bp)));
		void *prev = (PREV_BLKP(bp));
		PUT(FLSB(prev),NULL);
		PUT(FLPB(prev),&pred);
		pred = prev;

	}
	PUT(FLSB(tmp),NULL);
//	dbg_printf("sort end\n");
}
int get_free_count()
{
	int a = 0;
	void *bp = pred;
	if ( bp == NULL)
		return 0;
	while(bp != NULL)
	{
		a++;
		bp = GET(FLSB(bp));
	}
	return a;
}
		
