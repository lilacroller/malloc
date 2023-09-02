#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include "memory.h"

#define PAGE_SIZE 4096

typedef unsigned long long ull; 

ull freecounter= 0;

void *alloc_from_ram(size_t size)
{
	assert((size % PAGE_SIZE) == 0 && "size must be multiples of 4096");
	void* base = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
	if (base == MAP_FAILED)
	{
		printf("Unable to allocate RAM space\n");
		exit(0);
	}
	return base;
}

void free_ram(void *addr, size_t size)
{
	munmap(addr, size);
}

struct metadata {
    unsigned long long bucketsize;
    unsigned long long freebytes;

};
struct slotLinks {
    void *left, *right;
};
int bucketSizeList[]= {16, 32, 64, 128, 256, 512, 1024, 2048, 4080};
void *headerList[]= {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

//getslotSize:size_t->int
int getbucketSize(size_t size) {
    int i=0;
    while(bucketSizeList[i]<size) i++;
    return i;
}

struct metadata* getMD(void* ptr) {
    return (struct metadata*)(((ull)ptr/4096)*4096);
}
void freethepage(void *ptr){

    struct metadata* md= getMD(ptr);
    ull bucketsize= md->bucketsize;
    int sectorCount= (md->freebytes)/bucketsize;
    void *itr= (void*)md+16;
    int i= 0;
    while(i<sectorCount){

        void *Left= ((struct slotLinks*)itr)->left;
        void *Right= ((struct slotLinks*)itr)->right;
		

        if(Left!=NULL&&Right!=NULL) {
            ((struct slotLinks*)Left)->right= Right;
            ((struct slotLinks*)Right)->left= Left;
            itr+= bucketsize;
        }
        else if(Left!=NULL&&Right==NULL) {
            ((struct slotLinks*)Left)->right= NULL;
            itr+= bucketsize;
        }
        else if(Right!=NULL&&Left==NULL) {
            ((struct slotLinks*)Right)->left= NULL;
            itr+= bucketsize;
        }


/*        printf("((struct slotLinks*)Left)->right: %p", ((struct slotLinks*)Left)->right);
        printf("((struct slotLinks*)Right)->left: %p", ((struct slotLinks*)Right)->left);
        printf("program was here\n");
        ((struct slotLinks*)Left)->right= Right;
        ((struct slotLinks*)Right)->left= Left;*/
        i++;
    }

    free_ram(md, md->freebytes);
}

/*freefrombucket:void*->void
it will add the node back to the list, and update the metadata,
next and prev pointers.
if adding the node makes the whole page free, then each
node's links are adjusted and the page is freed.
*/
void freefrombucket(void *ptr) {
    struct metadata* md= getMD(ptr);



    int bucketsize= md->bucketsize;
    md->freebytes+= md->bucketsize;
    int i= getbucketSize(bucketsize);
    if(headerList[i]==NULL){
        struct slotLinks *sl= (struct slotLinks*) ptr;
        sl->left= NULL;
        sl->right= NULL;
        headerList[i]= ptr;
        return;
    }
    void *head= headerList[i];
    struct slotLinks *sl= (struct slotLinks*) ptr;
    sl->left= NULL;
    sl->right= head;
    headerList[i]= ptr;
    ((struct slotLinks*)(sl->right))->left= ptr;

    if(md->bucketsize==0){
        printf("bucketsize: %d\n", md->bucketsize);
        printf("index: %d\n", i);
    }
    assert(md->bucketsize!=0 && "bucketsize cant be 0");
    if(md->freebytes==(4080/(md->bucketsize))*(md->bucketsize));
        freethepage((void *)md+16);
         

}

void freelargeobject(void *ptr) {
//    printf("ptr: %p\n", ptr);
   struct metadata *md= getMD(ptr);
//    printf("metadata: %p\n", md);
//    printf("step1\n");
//    printf("md->bucketsize: %p\n", md->bucketsize);
    ull size= md->bucketsize;
    free_ram((void*)md, size); 
}
/* myfree: void*->void
takes pionter to the segment that needs to be freed.
Finds its metadata, increase by one the freebytes md,
and adds the slot back to the linked list.In case when
all the list are free, then release the complete page.

*/
void myfree(void *ptr)
{

    struct metadata *md= getMD(ptr);
    

    if(md->bucketsize<=4080) freefrombucket(ptr);
    else if(md->bucketsize>4080){
        freelargeobject(ptr);
    }
    //printf("step2\n");
//	printf("myfree is not implemented\n");
//	abort(); 
}


//B
//function that take size and directly allocates 
//from the ram
void *largesizeRequest (size_t size) {
    int packagesize= ceil((size+16)/4096)*4096;
    void *spaceblock= alloc_from_ram(packagesize); 
    struct metadata *md= (struct metadata *)spaceblock;
    md->bucketsize= packagesize-16;
    md->freebytes= packagesize-16;
    return (spaceblock + 16);
}




/* slotify: void* * size_t->void*
takes a pointer pointing to first byte of a fresh page.
modifies the page to store list of free slots which are
linked together to form a linked list. Also updates
the metadata of the page and returns first byte of the
updated page.
*/
void slotify(void *pageblock, size_t bucketsize){

	if(pageblock==NULL) return;

    struct metadata* md= (struct metadata*)pageblock;
    md->bucketsize= bucketsize;
    md->freebytes= (4080/bucketsize)*bucketsize;
    void *itr= pageblock+16;
    int size= md->freebytes;
   
    if(md->bucketsize==0){
        printf("md->bucketsize: %llu\n", md->bucketsize);
//        printf("md->freebytes: %llu\n", md->freebytes);
    }
    struct slotLinks* slot;

    while(size>=bucketsize){ 
        itr= itr+bucketsize;
        slot= (struct slotLinks*) itr;
        slot->left= itr-bucketsize;
        slot->right= itr+bucketsize;
        size-= bucketsize;
    }
    ((struct slotLinks*)(pageblock+16))->left= NULL;
    slot->right= NULL;
   	 
}


//A
//function take size<=4080,
//finds the slot size, extracts slot from the 
//figured free slot list updates the metadata of
//the page and returns the chosen slot.
//three cases, 1.there is no lilst defined yet
//2. There is already a list
//3. last node present in the list.
// doA:size_t->void*
void *bucketsizeRequest(size_t size) {
    int i= getbucketSize(size);
    printf("mymalloc size: %zd %d\n", size, bucketSizeList[i]);
    printf("header: %p\n", headerList[i]);
    if(headerList[i]==NULL) {
        void *pageblock= alloc_from_ram(4096);
        printf("mymalloc pageblock %p\n", pageblock);
		slotify(pageblock, bucketSizeList[i]);
        headerList[i]= pageblock+16;
        struct metadata *md= (struct metadata*)pageblock;
        md->bucketsize= bucketSizeList[i];
        if(md->bucketsize==0)   
            printf("***metadata->bucketsize: %d\n", md->bucketsize);
    }

     struct metadata *md= (struct metadata*)((((ull)headerList[i])>>12)<<12);
    md->freebytes-= md->bucketsize;
    
    printf("headerList[i]: %p\n", headerList[i]);
    struct slotLinks *node= (struct slotLinks*)headerList[i];
//    printf("node: %p\n", node);
 //   printf("i: %d", i);
    printf("node->right: %p\n", node->right);
    headerList[i]= node->right;
//    ((struct slotLinks*)headerList[i])->left= NULL;
    printf("mymalloc node: %p\n\n",node);
    return node;
}


void *mymalloc(size_t size)
{
     if(size<=4080) return bucketsizeRequest(size);
     if(size>4080) return largesizeRequest(size);

    return NULL;
/*	printf("mymalloc is not implemented\n");
	abort();
	return NULL;
    */
}
