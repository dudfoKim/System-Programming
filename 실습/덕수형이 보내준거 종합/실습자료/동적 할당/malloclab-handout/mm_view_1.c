#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include "mm.h"
#include "memlib.h"


#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


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
#define AD(bp) ( ((char*)bp - start)/8 )		


void *extend(size_t asize);
void *place(void *bp,size_t asize);
void *malloc(size_t size);
static void *find_fit(size_t asize);
static void merge(void *bp , size_t size, int state);
static void merge_case(void *bp,int state);
static void free_sort();
static void print(void *bp);

void *init;			//list
void *last;			//last
void *start;
int free_count;
void *real_last;

//Init
int mm_init(void)
{
//	dbg_printf("\n\t\t\t\t\t\tinit\n");
	void *bp;
	start = mem_sbrk(0);
	free_count = 0;
	init = NULL;
	bp = mem_sbrk(DSIZE*2);
	PUT(HDRP(bp),PACK(DSIZE*1,1));
	PUT(FTRP(bp),PACK(DSIZE*1,1));
	bp = NEXT_BLKP(bp);
	last = bp;
	PUT(HDRP(bp),PACK(DSIZE*1,1));
	PUT(FTRP(bp),PACK(DSIZE*1,1));
	bp = NEXT_BLKP(bp);
	real_last = bp;
//	dbg_printf("start : %d\n",start);
//	dbg_printf("init address : %d\n",&init);
}
void *malloc(size_t size)
{
//	dbg_printf("\n\t\t\t\t\t\tmalloc\n");
	void *bp;
	size_t asize;
	if ( size <= 0 )
		return 0;
	if ( size <= DSIZE)
		asize = DSIZE+OVERHEAD;
	else
		asize = DSIZE * (( size + (OVERHEAD) + (DSIZE-1))/DSIZE);
//	dbg_printf("request size : %d\n",asize);
	while(1)
	{
		if( free_count )			//exist
		{
			if ( ( bp = find_fit(asize)) != NULL )
			{
				bp =  place(bp,asize);
				return bp;
			}
			else
				extend(asize);
		}
		else
			extend(asize);
	}
}
void *find_fit(size_t asize)
{
	void *bp;
//	dbg_printf("find_fit\n");
	for ( bp = init ; bp != NULL ; bp = GET(FLSB(bp))) 
	{
//		dbg_printf("alloc : %d\tpre : %d\taddress : %d\tnext : %d\tsize : %d\n",GET_ALLOC(HDRP(bp)),GET(FLPB(bp)),bp,GET(FLSB(bp)),GET_SIZE(HDRP(bp)));
		if ( GET_SIZE(HDRP(bp)) >= asize)
			return bp;
	}
		return NULL;
}
void *place(void *bp,size_t asize)
	
{
//	dbg_printf("place\n");
	void *p = GET(FLPB(bp));		//preview free_block
	void *n = GET(FLSB(bp));		//next free_block
	void *return_bp = bp;
//	dbg_printf(" p : %d\tbp : %d\tn : %d\n",p,bp,n);
	size_t size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp),PACK(asize,1));
	PUT(FTRP(bp),PACK(asize,1));
	size -=asize;
	bp = NEXT_BLKP(bp);				//split block
	if ( size == 0 )
	{
		if ( free_count >=2 )
		{
			PUT(FLSB(p),n);
			if ( n != NULL )
				PUT(FLPB(n),p);
//			dbg_printf("true\n");
		}
		else
		{
			init = NULL;
		}
		
		free_count--;
		if ( free_count == -1 )
			free_count = 0;
	}
	else
	{
		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
//		dbg_printf("p : %d\tbp : %d\tn : %d\n",p,bp,n);
		if ( p == &init)
		{
			PUT(FLPB(bp),&init);
			PUT(FLSB(bp),n);
			init = bp;
		}
		else
		{
			PUT(FLSB(bp),n);
			PUT(FLPB(bp),p);
			PUT(FLSB(p),bp);
		}
		if ( n != NULL )
			PUT(FLPB(n),bp);
	}
	return return_bp;
	

void *extend(size_t asize)
{
//	dbg_printf("extend\n");
	void *cp;
	void *bp;
	bp = mem_sbrk(asize);
	PUT(HDRP(bp),PACK(asize,0));
	PUT(FTRP(bp),PACK(asize,0));
	cp = PREV_BLKP(bp);
	PUT(HDRP(cp),PACK(asize,0));
	PUT(FTRP(cp),PACK(asize,0));
	bp = NEXT_BLKP(cp);
	PUT(HDRP(bp),PACK(DSIZE*1,1));			//dumy block
	PUT(FTRP(bp),PACK(DSIZE*1,1));			//dumy block
	last = last+asize;
	real_last = real_last + asize;
	bp = cp;
	cp = PREV_BLKP(cp);
	if ( GET_ALLOC(HDRP(cp)) )
		merge(bp,asize,1);			//AXA
	else
		merge(bp,asize,3);			//FXA
	cp = NEXT_BLKP(cp);
	return cp;
}
void *realloc(void *oldptr, size_t size)
{
//	dbg_printf("realloc\n");
	if ( oldptr == NULL)
		return malloc(size);
	size_t oldsize=GET_SIZE(HDRP(oldptr));
	void *newptr;
	if ( size <= 0 )
	{
		free(oldptr);
		return 0;
	}
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
//	dbg_printf("\nfree\n");
	if ( !bp )
		return NULL;
	void *prev = PREV_BLKP(bp);		//prev block 
	void *next = NEXT_BLKP(bp);		//next block 
	int next_alloc_state = GET_ALLOC(HDRP(next));		//decide block state
	int prev_alloc_state = GET_ALLOC(HDRP(prev));	   //decide block state
	int size = GET_SIZE(HDRP(bp));

	if ( GET_ALLOC(HDRP(bp)) == 0 )
		return NULL;

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
//	dbg_printf("merge\n");
	switch (state)		//X means current blcok
	{
	case 1:		//AXA
		{
//			dbg_printf("case : AXA\n");
			PUT(HDRP(bp),PACK(size,0));
			PUT(FTRP(bp),PACK(size,0));
			merge_case(bp,1);
			return;
		}
	case 2:		//AXF
		{
//			dbg_printf("case2 : AXF\n");
			PUT(HDRP(bp),PACK(size,0));
			PUT(FTRP(bp),PACK(size,0));
			merge_case(bp,2);
			return ;
		}
	case 3:		//FXA
		{
//			dbg_printf("case3 : FXA\n");
			PUT(HDRP(bp),PACK(size,0));
			PUT(FTRP(bp),PACK(size,0));
			merge_case(bp,3);
			return ;
		}
	case 4:		//FXF
		{
//			dbg_printf("case4 : FXF\n");
			PUT(HDRP(bp),PACK(size,0));
			PUT(FTRP(bp),PACK(size,0));
			merge_case(bp,4);
			return ;
		}
	}
	
}
static void merge_case(void *current_block,int state)
{
//	dbg_printf("merge_case\n");
	void *prev_block = PREV_BLKP(current_block);
	void *next_block = NEXT_BLKP(current_block);
	int prev_size = GET_SIZE(HDRP(prev_block));
	int next_size = GET_SIZE(HDRP(next_block));
	int current_size = GET_SIZE(HDRP(current_block));
	switch ( state )
	{
	case 1://AXA
		{	//no correct	
//			dbg_printf("case1 : AXA\n");
//			dbg_printf("init : %d\n",init);
			int a;
			if ( init == NULL )
			{
				
				PUT(FLPB(current_block),&init);
				PUT(FLSB(current_block),init);
			}
			else
			{
				//v = init;
//				dbg_printf("else\n");
			//	print(init);
	//			dbg_printf("\n");
				next_block = init;
				if ( next_block != NULL )
				{
//					dbg_printf("treu");
					PUT(FLSB(current_block),next_block);
					PUT(FLPB(next_block),current_block);
				
				}
				else
					PUT(FLSB(current_block),NULL);
				PUT(FLPB(current_block),&init);
//				dbg_printf("else end\n");
//				a = 10;
			}
			free_count++;
			init = current_block;
//			if ( a == 10 )
//				print(init);
			
			return ;
		}
	case 2://AXF
		{	//prev_block=not free , next_block=free
//			print(init);
//			dbg_printf("case2 : AXF\n");
			PUT(HDRP(current_block),PACK(next_size+current_size,0));
			PUT(FTRP(next_block),PACK(next_size+current_size,0));
			
			prev_block = GET(FLPB(next_block));	//prev_block == free_block
			next_block = GET(FLSB(next_block));	//next_block == free_block 
			
			PUT(FLSB(current_block),next_block);
			PUT(FLPB(current_block),prev_block);
			if ( prev_block == &init)
			{
//				dbg_printf("xx");
				init = current_block;
			}
			else
			{
				PUT(FLSB(prev_block),current_block);
				
//				dbg_printf("zz");
				
			}
			if ( next_block != NULL )
			{
//				dbg_printf("aa");
				PUT(FLPB(next_block),current_block);
			}
//			print(init);
			return ;
		}
	case 3://FXA
		{	//no correct
//			dbg_printf("case3 : FXA\n");
			PUT(HDRP(prev_block),PACK(prev_size+current_size,0));
			PUT(FTRP(current_block),PACK(prev_size+current_size,0));
			return ;
		}
	case 4://FXF
		{
//			dbg_printf("case4 : FXF\n");
//			dbg_printf("prev : %d\tcurrent : %d\tnext : %d\n",prev_block,current_block,next_block);
//			dbg_printf("prev_alloc : %d\tcurrent_alloc : %d\tnext_alloc : %d\n",GET_ALLOC(HDRP(prev_block)),GET_ALLOC(HDRP(current_block)),GET_ALLOC(HDRP(next_block)));
//			dbg_printf("prev_size : %d\tcurrent_size :%d\tnext_size :%d\n",GET_SIZE(HDRP(prev_block)),GET_SIZE(HDRP(current_block)),GET_SIZE(HDRP(next_block)));
			
			PUT(HDRP(prev_block),PACK(prev_size+current_size+next_size,0));
			PUT(FTRP(prev_block),PACK(0,0));
			PUT(HDRP(current_block),PACK(0,0));
			PUT(FTRP(current_block),PACK(0,0));
			PUT(HDRP(next_block),PACK(0,0));
			PUT(FTRP(next_block),PACK(prev_size+current_size+next_size,0));
			free_count--;
			free_sort();
			return ;
		}
	}
}

void free_sort()
{
	void *first,*second;
//	dbg_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~free_sort~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
//	print(init);
//	dbg_printf("\n");
	
	first = start;
	first = NEXT_BLKP(first);
	second = NEXT_BLKP(first);
//	dbg_printf("second %d\n",second);



//	while(first != last)
//	{
//		dbg_printf("prev : %d\tcurrent : %d\tnext : %d\tsize : %d\n",PREV_BLKP(first),first,NEXT_BLKP(first),GET_SIZE(HDRP(first)));
//		first = NEXT_BLKP(first);
//	}

//	dbg_printf("\n");

		
	first = start;
	first = NEXT_BLKP(first);
	while(1)
	{
		if ( GET_ALLOC(HDRP(first)) == 0 )
			break;
		first = NEXT_BLKP(first);
	}
	init = first;
	PUT(FLPB(first),&init);
//	dbg_printf("last : %d\n",last);
//	dbg_printf("alloc : %d\tprev : %d\tcurrent : %d\tsize : %d\n\n",GET_ALLOC(HDRP(first)),GET(FLPB(first)),first,GET_SIZE(HDRP(first)));
	while(first != NULL )
	{
		if ( GET_ALLOC(HDRP(first)) == 0 )
		{
//			dbg_printf("prev : %d\tcurrent : %d\tsize : %d\n",GET(FLPB(first)),first,GET_SIZE(HDRP(first)));
			second = NEXT_BLKP(first);
			while(second != last)
			{
				if ( GET_ALLOC(HDRP(second)) == 0 )
					break;
				else
					second = NEXT_BLKP(second);
			}
//			dbg_printf("second : %d\n",second);
			PUT(FLSB(first),second);
			if ( second == last )
			{
//				dbg_printf("xx");
				break;
			}
			PUT(FLPB(second),first);
			first = GET(FLSB(((first))));
		}
		else
			first = NEXT_BLKP(first);
	}
	PUT(FLSB(first),NULL);
//	dbg_printf("\n");
//	dbg_printf("prev : %d\tcurrent : %d\tsize : %d\n",GET(FLPB(first)),first,GET_SIZE(HDRP(first)));
//	dbg_printf("\n");
//	print(init);
//	dbg_printf("free_sort_end\n");
}
void print(void *bp)
{
	if( bp )
	{
		dbg_printf("prev : %d\tcurrent : %d\tnext : %d\tsize : %d\n",GET(FLPB(bp)),bp,GET(FLSB(bp)),GET_SIZE(HDRP(bp)));

	print(GET(FLSB(bp)));
	}
}
