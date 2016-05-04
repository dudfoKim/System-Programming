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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)	// size������ 8��Ʈ ����(double word)�� ����
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))	
#define SIZE_PTR(p)  ((size_t*)(((char*)(p)) - SIZE_T_SIZE))

/* bitfields */
/* unsigned ������ : ��Ʈ��; */
typedef struct controlBlock{
	unsigned allocated:1;
	unsigned size:31;
	unsigned Empty:32;
}Block;

/* �迭���ٴ� ��ũ�� ����Ʈ ��� */
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

	/* root ���� ������ Node ���� �˻� */
	/* ���� memory control block�� ������ġ�� -8 �� ���� */
	/* memory control block = Header�� ��� */
	/* ���ο� ũ�⺸�� ũ�ų� ���� ��� */
	/* bestfit�� firstfit�� ���� */
	currentNode = root;

	while( currentNode != NULL )
	{
		currentHeader = ( Block * )SIZE_PTR( currentNode ); // ���� ��� �ּ�

		if( ( currentHeader -> size ) >= newSize )
		{
			/* bestfit */
			/* ���� memory control block �� size �� newsize �� ���̰�
				8(Header�� ũ��)���� �۰ų� ���� ��� */
			if( ( (currentHeader -> size ) - newSize ) <= 8 )
			{
				MemoryLocation = currentNode;

				currentHeader -> allocated = 1;
				
				/* Prev�� Next �� ��� NULL �� ��� root�� NULL �� ���� */
				if( currentNode -> Prev == NULL && currentNode -> Next == NULL )
				{
					root = NULL;
				}

				/* Prev�� NULL �� ��� root�� �������� Next�� ���� Prev�� NULL �� ���� */
				else if( currentNode -> Prev == NULL && currentNode -> Next != NULL )
				{
					root = currentNode -> Next;
					root -> Prev = NULL;
				}

				/* Next�� NULL �� ��� Prev�� Next�� NULL�� ���� */
				else if( currentNode -> Prev != NULL && currentNode -> Next == NULL )
				{
					currentNode -> Prev -> Next = NULL;
				}

				/* Prev�� Next ��� NULL �� �ƴ� ��� Prev�� Next�� Next�� Next�� Prev�� Prev�� ���� */
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

				/* ���ο� ���� ���� */
				/* ������ size�� ���� memory control block�� size�� ���� */
				/* ������ memory control block �� size�� newsize�� ���� */
				/* allocated �Ǿ����� ��� */

				/* ���ο� ��带 �����ϸ� ���� ����� ��带 ���� 
				 * ���ο� ���� = (struct empty_node *)(( char * )cureent_empty_node + newsize )); */

				/* ���ο� ����� memorycontrol block�� ���� ��ġ ���� (-8) (Headerũ�� ����) */
				/* ���ο� ����� size�� oldSize - newSize */
				/* ���ο� ���� free ���� */
				/* ���ο� ��带 ��ũ�帮��Ʈ�� ���� */

				/* Prev �� Next ��ΰ� NULL �� ��� root�� ���ο� ���� ���� */
				if( currentNode -> Prev == NULL && currentNode -> Next == NULL )
				{
					root = nextNode;
				}
				
				/* Prev �� NULL �� ��� root�� ���ο� ���� ����
                         	   Next�� Prev �� ���ο� ���� ���� */
				else if( currentNode -> Prev == NULL && currentNode -> Next != NULL )
				{
					root = nextNode;
					currentNode -> Next -> Prev = nextNode;
				}

				/* Next �� NULL �� ��� Prev�� Next�� ���ο� ���� ���� */
				else if( currentNode -> Prev != NULL && currentNode -> Next == NULL )
				{
					currentNode -> Prev -> Next = nextNode;
				}
				
				/* ��� NULL �� �ƴ� ��� Prev�� Next�� ���ο� ���� ����
				   Next�� Prev�� ���ο� ���� ���� */
				else if( currentNode -> Prev != NULL && currentNode -> Next != NULL )
				{

					currentNode -> Prev -> Next = nextNode;
					currentNode -> Next -> Prev = nextNode;
				}
				break;
			}

			/* ���� ��带 �������� Next�� ��ȯ */
			/* ������ ������ �������� �ʾ��� ���� ���ο� ��带 �����. */
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
	/* root Node�� NULL �� ���� free�� ������ root�� ���� */

	/* root = ptr
	   ���� ����ִ� ��� = ptr 
	   ������ memory control block �� ���� ����� -8 ��ġ�� ����
	   ���´� free����
	   ���Ḯ��Ʈ�� ������Ŵ*/
	if( root == NULL )
	{
		root = ptr;
		currentNode = ptr;

		Header = ( Block * )SIZE_PTR( currentNode );
		Header -> allocated = 0;

		currentNode -> Next = NULL;
		currentNode -> Prev = NULL;
	}

	/* ��Ʈ��尡 NULL �� �ƴ� ���
	   ���� ��带 ptr�� ��ȯ�� ������ memorycontrolblock �� ���� ����� -8 ��ġ�� ����
	   ���´� free����
	   ��Ʈ ����� Prev�� ���� ���� �����ϰ� Next�� root�� �����ϰ�, Prev�� NULL�� ����
	   root �� ���� ���� ���� */
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
	/* size �� 0�� ������ free */
	if( size == 0 ) 
	{
		free( oldptr );

		return 0;
	}

	/* If oldptr is NULL, then this is just malloc. */
	/* oldptr �� NULL �� ���� �׳� malloc */
	if( oldptr == NULL ) 
	{
		return malloc( size );
	}

	/* ���ο� ptr�� malloc(size) �� �ϰ� */
	newptr = malloc( size );

	/* If realloc() fails the original block is left untouched  */
	if( !newptr ) 
	{
		return 0;
	}

	/* memorycontrol block �� ��ġ�� oldptr�� -8 ��ġ */
	MemoryControlBlock = ( Block * )SIZE_PTR( oldptr );

	/* oldsize �� Memory control block �� size�� ���� */
	oldsize = MemoryControlBlock -> size;

	/* size�� oldsize���� ���� ��쿡�� oldsize�� �ٽ� size�� ���� */
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
