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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)	// size공간을 8비트 단위(double word)로 세팅
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))	
#define SIZE_PTR(p)  ((size_t*)(((char*)(p)) - SIZE_T_SIZE))

/* bitfields */
/* unsigned 변수명 : 비트수; */
typedef struct controlBlock{
	unsigned allocated:1;
	unsigned size:31;
	unsigned Empty:32;
}Block;

/* 배열보다는 링크드 리스트 사용 */
struct Node{
	struct Node *Next;
	struct Node *Prev;
};

struct Node *root;

int nodeState( struct Node *pCurrentNode, struct Node *pNextNode );

/*
 * mm_init - Called when a new trace starts.
 */
int mm_init(void)
{
	mem_init();

	root = NULL;

	return 0;
}
/*
 * malloc - Allocate a block by incrementing the brk pointer.
 *      Always allocate a block whose size is a multiple of the alignment.
 */

void *malloc(size_t size)
{
	void *MemoryLocation = 0;

	Block *currentHeader, *nextHeader;
	
	struct Node *currentNode, *nextNode;
	
	unsigned oldSize;
	unsigned newSize = ALIGN( size + SIZE_T_SIZE ); 

	/* root 부터 마지막 Node 까지 검사 */
	/* 현재 memory control block은 현재위치의 -8 로 설정 */
	/* memory control block = Header로 명시 */
	/* 새로운 크기보다 크거나 같을 경우 */
	/* bestfit과 firstfit을 결정 */
	currentNode = root;

	while( currentNode != NULL )
	{
		currentHeader = ( Block * )SIZE_PTR( currentNode ); // 현재 헤더 주소

		if( ( currentHeader -> size ) >= newSize )
		{
			/* bestfit */
			/* 현재 memory control block 의 size 와 newsize 의 차이가
				8(Header의 크기)보다 작거나 같을 경우 */
			if( ( (currentHeader -> size ) - newSize ) <= 8 )
			{
				MemoryLocation = currentNode;

				currentHeader -> allocated = 1;
				
				/* Prev와 Next 가 모두 NULL 일 경우 root를 NULL 로 설정 */
				if( currentNode -> Prev == NULL && currentNode -> Next == NULL )
				{
					root = NULL;
				}

				/* Prev만 NULL 일 경우 root를 현재노드의 Next로 설정 Prev는 NULL 로 설정 */
				else if( currentNode -> Prev == NULL && currentNode -> Next != NULL )
				{
					root = currentNode -> Next;
					root -> Prev = NULL;
				}

				/* Next만 NULL 일 경우 Prev의 Next를 NULL로 설정 */
				else if( currentNode -> Prev != NULL && currentNode -> Next == NULL )
				{
					currentNode -> Prev -> Next = NULL;
				}

				/* Prev와 Next 모두 NULL 이 아닌 경우 Prev의 Next를 Next로 Next의 Prev를 Prev로 설정 */
				else if( currentNode -> Prev != NULL && currentNode -> Next != NULL )
				{
					currentNode -> Prev -> Next = currentNode -> Next;
					currentNode -> Next -> Prev = currentNode -> Prev;
				}
				break;

			}

			else if( newSize <= 64 )
			{
				MemoryLocation = currentNode;
				
				oldSize = currentHeader -> size;
				currentHeader -> size = newSize;
				
				currentHeader -> allocated = 1;

				nextNode = ( struct Node * )( ( char *)currentNode + newSize );

				nextHeader = ( Block * )SIZE_PTR( nextNode );
				nextHeader -> size = oldSize - newSize;
				nextHeader -> allocated = 0;

				nextNode -> Next = currentNode -> Next;
				nextNode -> Prev = currentNode -> Prev;

				/* 새로운 블럭을 설정 */
				/* 원래의 size를 현재 memory control block의 size로 설정 */
				/* 현재의 memory control block 의 size를 newsize로 설정 */
				/* allocated 되었음을 명시 */

				/* 새로운 노드를 설정하며 생긴 빈공간 노드를 설정 
				 * 새로운 노드는 = (struct empty_node *)(( char * )cureent_empty_node + newsize )); */

				/* 새로운 노드의 memorycontrol block의 현재 위치 설정 (-8) (Header크기 유의) */
				/* 새로운 노드의 size는 oldSize - newSize */
				/* 새로운 노드는 free 상태 */
				/* 새로운 노드를 링크드리스트로 설정 */

				/* Prev 와 Next 모두가 NULL 일 경우 root를 새로운 노드로 설정 */
				if( currentNode -> Prev == NULL && currentNode -> Next == NULL )
				{
					root = nextNode;
				}
				
				/* Prev 가 NULL 일 경우 root를 새로운 노드로 설정
                         	   Next의 Prev 를 새로운 노드로 설정 */
				else if( currentNode -> Prev == NULL && currentNode -> Next != NULL )
				{
					root = nextNode;
					currentNode -> Next -> Prev = nextNode;
				}

				/* Next 가 NULL 일 경우 Prev의 Next를 새로운 노드로 설정 */
				else if( currentNode -> Prev != NULL && currentNode -> Next == NULL )
				{
					currentNode -> Prev -> Next = nextNode;
				}
				
				/* 모두 NULL 이 아닐 경우 Prev의 Next를 새로운 노드로 설정
				   Next의 Prev도 새로운 노드로 설정 */
				else if( currentNode -> Prev != NULL && currentNode -> Next != NULL )
				{

					currentNode -> Prev -> Next = nextNode;
					currentNode -> Next -> Prev = nextNode;
				}
				break;
			}

			/* 현재 노드를 현재노드의 Next로 변환 */
			/* 적당한 공간이 결정되지 않앗을 경우는 새로운 노드를 만든다. */
		}
		currentNode = currentNode -> Next;
	}
	
	if( !MemoryLocation )
	{
		MemoryLocation = mem_sbrk( newSize );

		if( ( long ) MemoryLocation <= 0 )
			return NULL;

		MemoryLocation += SIZE_T_SIZE;

		currentHeader = ( Block * )SIZE_PTR( MemoryLocation );

		currentHeader -> allocated = 1;
		currentHeader -> size = ( unsigned )newSize;
	}

	return MemoryLocation;
}

/*
 * free - We don't know how to free a block.  So we ignore this call.
 *      Computers have big memories; surely it won't be a problem.
 */
void free(void *ptr)
{
	Block *Header;
	struct Node *currentNode;
	
	if( ptr == NULL )
		return;
	/* root Node가 NULL 일 경우는 free된 공간을 root로 설정 */

	/* root = ptr
	   현재 비어있는 노드 = ptr 
	   현재의 memory control block 은 현재 노드의 -8 위치로 설정
	   상태는 free상태
	   연결리스트를 해지시킴*/
	if( root == NULL )
	{
		root = ptr;
		currentNode = ptr;

		Header = ( Block * )SIZE_PTR( currentNode );
		Header -> allocated = 0;

		currentNode -> Next = NULL;
		currentNode -> Prev = NULL;
	}

	/* 루트노드가 NULL 이 아닐 경우
	   현재 노드를 ptr로 변환후 현재의 memorycontrolblock 은 현재 노드의 -8 위치로 설정
	   상태는 free상태
	   루트 노드의 Prev를 현재 노드로 설정하고 Next를 root로 설정하고, Prev는 NULL로 해지
	   root 는 현재 노드로 설정 */
	else
	{
		currentNode = ptr;

		Header = ( Block *)SIZE_PTR( currentNode );
		Header -> allocated = 0;

		currentNode -> Next = root;
		currentNode -> Prev = NULL;
		root -> Prev = currentNode;
		root = currentNode;
	}

	return ;
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
	Block *MemoryControlBlock;

	/* If size == 0 then this is just free, and we return NULL. */
	/* size 가 0과 같으면 free */
	if( size == 0 ) 
	{
		free( oldptr );

		return 0;
	}

	/* If oldptr is NULL, then this is just malloc. */
	/* oldptr 이 NULL 일 경우는 그냥 malloc */
	if( oldptr == NULL ) 
	{
		return malloc( size );
	}

	/* 새로운 ptr에 malloc(size) 를 하고 */
	newptr = malloc( size );

	/* If realloc() fails the original block is left untouched  */
	if( !newptr ) 
	{
		return 0;
	}

	/* memorycontrol block 의 위치는 oldptr의 -8 위치 */
	MemoryControlBlock = ( Block * )SIZE_PTR( oldptr );

	/* oldsize 는 Memory control block 의 size로 설정 */
	oldsize = MemoryControlBlock -> size;

	/* size가 oldsize보다 작을 경우에는 oldsize를 다시 size로 변경 */
	if( size < oldsize )
		oldsize = size;

	memcpy( newptr, oldptr, oldsize );

	free( oldptr );

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
