#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt()

#define BYTES_PER_WORD 4
// #define DEBUG

/*
 * Cache structures
 */
int time = 0;

typedef struct
{
    int age;  // LRU (Least recently used)
    int valid; // 0 : miss | 1 : hit
    int modified;
    uint32_t tag;
} cline;

typedef struct
{
    cline *lines;
} cset;

typedef struct
{
    int s; // index bits
    int E; // way
    int b; // block offset bits
    cset *sets;
} cache;

int index_bit(int n){
    int cnt = 0;

    while(n) {
        cnt++;
        n = n >> 1;
    }

    return cnt-1;
}

/***************************************************************/
/*                                                             */
/* Procedure : build_cache                                     */
/*                                                             */
/* Purpose   : Initialize cache structure                      */
/*                                                             */
/* Parameters:                                                 */
/*     int S: The set of cache                                 */
/*     int E: The associativity way of cache                   */
/*     int b: The blocksize of cache                           */
/*                                                             */
/***************************************************************/
cache build_cache(int S, int E, int b)
{ // S = 16, E = 8, b = 8
	/* Implement this function */
    cache result_cache;

    result_cache.s = index_bit(S);
    result_cache.E = E;
    result_cache.b = index_bit(b);

    result_cache.sets = (cset*)malloc(sizeof(cset)*S);

    for(int i = 0; i < S; i++) {

        cline* cache_line;
        result_cache.sets[i].lines = (cline*)malloc(sizeof(cline)*E);
        cache_line = result_cache.sets[i].lines;

        for(int j = 0; j < E; j++) {

            cache_line[j].age = 0;
            cache_line[j].valid = 0;
            cache_line[j].modified = 0;
            cache_line[j].tag = 0;

        }

    }

	// return result_cache;
     return result_cache; 
}

/***************************************************************/
/*                                                             */
/* Procedure : access_cache                                    */
/*                                                             */
/* Purpose   : Update cache stat and content                   */
/*                                                             */
/* Parameters:                                                 */
/*     cache *L: An accessed cache                             */
/*     int op: Read/Write operation                            */
/*     uint32_t addr: The address of memory access             */
/*     int *hit: The number of cache hit                       */
/*     int *miss: The number of cache miss                     */
/*     int *wb: The number of write-back                       */
/*                                                             */
/***************************************************************/
void access_cache(cache *L, int op, uint32_t addr, int *read, int *write, int *r_hit, int *w_hit, 
                                int *r_miss, int *w_miss, int *wb)
{
	/* Implement this function */
    
    /* Your output must contain the following statistics:
        - the number of total reads
        - the number of total writes
        - the number of write backs
        - the number of read hits
        - the number of write hits
        - the number of read misses
        - the number of write misses
    */

   // index = 4 bits
   // offset = 3 bites
   // tag = 25 bits

    uint32_t access_index = (addr >> 3) & 0xF;
    uint32_t access_tag = (addr >> 7);

    for(int i = 0; i < 16; i++) {
        for(int j = 0; j < 8; j++) {
            if(L->sets[i].lines[j].valid == 1) {
                L->sets[i].lines[j].age++;
            }
        }
    }

    switch (op) {
        
        case 0: { // write

            (*write)++;

            for(int i = 0; i < 8; i++) {

                if(L->sets[access_index].lines[i].valid == 1) {

                    if(L->sets[access_index].lines[i].tag == access_tag) {
                        (*w_hit)++;
                        L->sets[access_index].lines[i].age = time;
                        L->sets[access_index].lines[i].modified = 1;
                        return;
                    }

                } else {

                    (*w_miss)++;
                    L->sets[access_index].lines[i].valid = 1;
                    L->sets[access_index].lines[i].tag = access_tag;
                    L->sets[access_index].lines[i].modified = 1;
                    return;

                }

            }

            int LRU_count = 0;
            int LRU_out = 0;

            for(int i = 0; i < 8; i++) {

                if(L->sets[access_index].lines[i].age > LRU_count) {
                    LRU_count = L->sets[access_index].lines[i].age;
                    LRU_out = i;
                }

            }

            (*w_miss)++;
            
            L->sets[access_index].lines[LRU_out].tag = access_tag;
            L->sets[access_index].lines[LRU_out].age = 0;

            if(L->sets[access_index].lines[LRU_out].modified == 1) {
                (*wb)++;
            } else {
                L->sets[access_index].lines[LRU_out].modified = 1;
            }

            break;

        }

        case 1: { // read 

            (*read)++;

            for(int i = 0; i < 8; i++) {

                if(L->sets[access_index].lines[i].valid == 1) {

                    if(L->sets[access_index].lines[i].tag == access_tag) {
                        (*r_hit)++;
                        L->sets[access_index].lines[i].age = 0;
                        return;
                    }

                } else {

                    (*r_miss)++;
                    L->sets[access_index].lines[i].valid = 1;
                    L->sets[access_index].lines[i].tag = access_tag;
                    return;

                }

            }

            int LRU_count = 0;
            int LRU_out = 0;

            for(int i = 0; i < 8; i++) {

                if(L->sets[access_index].lines[i].age > LRU_count) {
                    LRU_count = L->sets[access_index].lines[i].age;
                    LRU_out = i;
                }

            }

            (*r_miss)++;

            L->sets[access_index].lines[LRU_out].valid = 1;
            L->sets[access_index].lines[LRU_out].tag = access_tag;
            L->sets[access_index].lines[LRU_out].age = 0;

            if(L->sets[access_index].lines[LRU_out].modified == 1) {
                (*wb)++;
                L->sets[access_index].lines[LRU_out].modified = 0;
            }

            break;
        }

        default:
            break;
    }
    
}

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize)
{

    printf("Cache Configuration:\n");
    printf("-------------------------------------\n");
    printf("Capacity: %dB\n", capacity);
    printf("Associativity: %dway\n", assoc);
    printf("Block Size: %dB\n", blocksize);
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat                                 */
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
           int reads_hits, int write_hits, int reads_misses, int write_misses)
{
    printf("Cache Stat:\n");
    printf("-------------------------------------\n");
    printf("Total reads: %d\n", total_reads);
    printf("Total writes: %d\n", total_writes);
    printf("Write-backs: %d\n", write_backs);
    printf("Read hits: %d\n", reads_hits);
    printf("Write hits: %d\n", write_hits);
    printf("Read misses: %d\n", reads_misses);
    printf("Write misses: %d\n", write_misses);
    printf("\n");
}


/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */
/*                                                             */
/* Cache Design                                                */
/*                                                             */
/*      cache[set][assoc][word per block]                      */
/*                                                             */
/*                                                             */
/*       ----------------------------------------              */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*                                                             */
/*                                                             */
/***************************************************************/
void xdump(cache* L)
{
    int i, j, k = 0;
    int b = L->b, s = L->s;
    int way = L->E, set = 1 << s;

    uint32_t line;

    printf("Cache Content:\n");
    printf("-------------------------------------\n");

    for(i = 0; i < way; i++) {
        if(i == 0) {
            printf("    ");
        }
        printf("      WAY[%d]", i);
    }
    printf("\n");

    for(i = 0; i < set; i++) {
        printf("SET[%d]:   ", i);

        for(j = 0; j < way; j++) {
            if(k != 0 && j == 0) {
                printf("          ");
            }
            if(L->sets[i].lines[j].valid) {
                line = L->sets[i].lines[j].tag << (s + b);
                line = line | (i << b);
            }
            else {
                line = 0;
            }
            printf("0x%08x  ", line);
        }
        printf("\n");
    }
    printf("\n");
}


int main(int argc, char *argv[])
{
    int capacity=1024;
    int way=8;
    int blocksize=8;
    int set;

    // Cache
    cache simCache;

    // Counts
    int read=0, write=0, writeback=0;
    int readhit=0, writehit=0;
    int readmiss=0, writemiss = 0;

    // Input option
    int opt = 0;
    char* token;
    int xflag = 0;

    // Parse file
    char *trace_name = (char*)malloc(32);
    FILE *fp;
    char line[16];
    char *op;
    uint32_t addr;

    /* You can define any variables that you want */

    int op_toInt = 0;

    trace_name = argv[argc-1];
    if (argc < 3) {
        printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n", argv[0]);
        exit(1);
    }

    while((opt = getopt(argc, argv, "c:x")) != -1) {
        switch(opt) {
            case 'c':
                token = strtok(optarg, ":");
                capacity = atoi(token);
                token = strtok(NULL, ":");
                way = atoi(token);
                token = strtok(NULL, ":");
                blocksize  = atoi(token);
                break;

            case 'x':
                xflag = 1;
                break;

            default:
                printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n", argv[0]);
                exit(1);

        }
    }

    // Allocate
    set = capacity / way / blocksize;

    /* TODO: Define a cache based on the struct declaration */
    simCache = build_cache(set, way, blocksize);
    // Simulate
    fp = fopen(trace_name, "r"); // read trace file
    if(fp == NULL) {
        printf("\nInvalid trace file: %s\n", trace_name);
        return 1;
    }

    cdump(capacity, way, blocksize);

    /* TODO: Build an access function to load and store data from the file */
    while (fgets(line, sizeof(line), fp) != NULL) {
        op = strtok(line, " ");
        addr = strtoull(strtok(NULL, ","), NULL, 16);

#ifdef DEBUG
        // You can use #define DEBUG above for seeing traces of the file.
        fprintf(stderr, "op: %s\n", op);
        fprintf(stderr, "addr: %x\n", addr);
#endif
        // ...

        if(strcmp(op, "W") == 0) {
            op_toInt = 0;
        } else if(strcmp(op, "R") == 0) {
            op_toInt = 1;
        }

        access_cache(&simCache, op_toInt, addr, &read, &write, &readhit, &writehit, &readmiss, &writemiss, &writeback);
        // ...
    }

    // test example
    sdump(read, write, writeback, readhit, writehit, readmiss, writemiss);
    if (xflag) {
        xdump(&simCache);
    }

    return 0;
}
