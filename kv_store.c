#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ring_buffer.h"

typedef struct HashNode {
	key_type k;
	value_type v;
	struct HashNode * next;
} HashNode_t;

HashNode_t* hashtable[1000];
pthread_mutex_t mutex[1000];
int hash_size = 10;

struct thread_context {
	int tid;
	// other stuff maybe??
};

pthread_t threads[128];
struct thread_context contexts[128];
int num_threads = 4;

struct ring *ring = NULL;
char * shmem_area = NULL;
char shm_file[] = "shmem_file";

/*
 * Puts key/value pair in the hashtable
 */
int put(key_type k, value_type v) {
	
	int index = hash_function(k, hash_size);
	
	pthread_mutex_lock(&mutex[index]);
	
	HashNode_t * temp = hashtable[index];
	if(hashtable[index] != NULL) {
		while (temp->next != NULL && temp->k != k) {
			temp = temp->next;
		}
		if (temp->k == k) {
			temp->v = v;
		}
	} 
	
	HashNode_t * newNode = (HashNode_t * ) malloc(sizeof(HashNode_t));
	newNode->k = k;
	newNode->v = v;
	newNode->next = NULL;
	
	if (hashtable[index] == NULL) {
		hashtable[index] = newNode;
	} else {
		HashNode_t * temp = hashtable[index];
		while (temp->next != NULL) {
			temp = temp->next;
		}
			temp->next = newNode;
		
	}
	
	pthread_mutex_unlock(&mutex[index]);
	
	return 0;
}

/*
 * Returns value of that key or 0 if not found
*/
int get(key_type k) {
	int index = hash_function(k, hash_size);
	int value = 0;
	
	pthread_mutex_lock(&mutex[index]);
	HashNode_t * temp = hashtable[index];
	while (temp != NULL) {
		if (temp->k == k) {
			value = temp->v;
		}
		temp = temp->next;
	}
	pthread_mutex_unlock(&mutex[index]);
	
	return value;
}

/* Sets the shmem_area global variable to the beginning of the shared region
 * Sets the ring global variable the beginning of the shared region
 * Shared memory area is organized as follows:
 * | RING | TID_0_COMPLETIONS | TID_1_COMPLETIONS | ... | TID_N_COMPLETIONS |
*/
int init_server() {
	   printf("init server start\n");
	   
       int fd = open(shm_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (fd < 0) {
        	perror("open");
		}

		struct stat * buf;
		fstat(fd, buf);
		int shm_size = buf->st_size;
		printf("shm_size %d\n", shm_size);
        char *mem = mmap(NULL, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
        if (mem == (void *)-1) {
         	perror("mmap");
         }
		
        /* mmap dups the fd, no longer needed */
        close(fd);

        memset(mem, 0, shm_size);
        
        ring = (struct ring *)mem;
       
        shmem_area = mem;
        

}

/*
 * Parses arguments to get number of threads and hash size, if provided
 *
*/
static int parse_args(int argc, char **argv) {
	int op;
	while ((op = getopt(argc, argv, "n:s:v")) != -1) {
		switch (op) {
			
	        case 'n':
	        num_threads = atoi(optarg);
	        break;
	        
	        case 's':
	        hash_size = atoi(optarg);
	        break;
	        
	        default:
	        return 1;
	        }
	        
	}
	return 0;
}

void serve_request(struct buffer_descriptor* bd) {
	if (bd->req_type == PUT) {
		put(bd->k, bd->v);
		bd->ready = 1;
		memcpy(shmem_area + bd->res_off, bd, sizeof(*bd));
	} 
	if (bd->req_type == GET) {		
		int value = get(bd->k);
		struct buffer_descriptor *newbd;
		newbd->k = bd->k;
		newbd->v = value;
		newbd->ready = 1;
		newbd->res_off = bd->res_off;
		memcpy(shmem_area + bd->res_off, newbd, sizeof(*newbd));
	}
}

/*
 * Function that's run by each thread
 * @param arg context for this thread
*/
void *thread_function(void *arg) {
	printf("h\n");
    struct thread_context *ctx = arg;
	printf("tid %d\n", ctx->tid);

	while(1) {
		struct buffer_descriptor* bd;
		ring_get(ring, bd);
		serve_request(bd);
	}
    // TODO infinite loop to fetch requests from buffer using ring_get
}

/*
 * Launch num_threads number of threads
 * Prepares the context for each thread
*/
void start_threads() {
	printf("start of start threads\n");
    for (int i = 0; i < num_threads; i++) {
    	printf("thread\n");
	    contexts[i].tid = i;
        if (pthread_create(&threads[i], NULL, &thread_function, &contexts[i])) {
        	perror("pthread_create");
        }
    }
}


void wait_for_threads() {
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL)) {
        	perror("pthread_join");
        }
    }
}

int main(int argc, char *argv[]) {
	printf("server\n");
	if (parse_args(argc, argv) != 0) {
	    exit(EXIT_FAILURE);
	}
	printf("parsed args\n");
	printf("hash_size %d\n", hash_size);
	// initialize locks for each hash index
	for (int i = 0; i < hash_size; i++) {
		pthread_mutex_init(&mutex[i], NULL);
	}
	printf("initialized locks\n");
	// TODO get shared memory... init_server();
	init_server();
	printf("inited server\n");
	start_threads();
	printf("started threads\n");
	
/*	
	put(49, 2701209440);
	put(45, 3602669057);
	get(32);
	put(37, 2495837118);
	get(10);
	get(20);
	put(42, 2811417357);
	put(21, 1727595483);
    get(50);
	get(39);
	get(17);
	put(31, 907538104);
	get(9);
	put(26, 635642472);
	put(50, 1489802152);
	get(27);
	get(10);
	put(9, 1168427025);
	put(19, 2462587621);
	get(15);
	put(16, 1387294160);
	get(41);
	get(39);
	put(33, 2589848910);
	put(29, 2589600969);
	get(17);
	get(26);
	get(8);
	put(2, 2657429931);
	put(48, 1533176625);
	put(1, 1899449759);
	put(46, 1502328340);
	put(32, 1670425044);
	get(21);
	put(17, 1171968046);
	get(44);
	get(33);
	get(28);
	put(43, 2475444797);
	put(4, 1380810609);
	put(39, 1061995247);
	put(20, 3506616455);
	get(9);
	put(7, 1848306733);
	put(30, 2435745368);
	put(10, 3284124238);
	get(27);
	get(49);
	put(38, 840009267);
	get(4);
	get(36);
	get(11);
	get(20);
	get(17);
	get(22);
	get(43);
	put(44, 2442054922);
	get(13);
	get(23);
	get(22);
	get(26);
	put(22, 1241709879);
	get(23);
	put(47, 641650486);
	put(27, 1728112374);
	get(36);
	get(27);
	put(8, 977265603);
	get(18);
	get(4);
	put(24, 3957874317);
	get(32);
	get(36);
	get(43);
	put(36, 2927698958);
	get(20);
	get(17);
	put(41, 1979428607);
	get(9);
	get(27);
	put(6, 1530601361);
	put(35, 2921342100);
	put(23, 774606382);
	get(37);
	put(34, 1673128353);
	put(18, 1643306410);
	put(40, 658446124);
	get(17);
	put(13, 3291334975);
	put(3, 264256110);
	put(12, 1108079365);
	put(14, 623815409);
	get(36);
	get(21);
	put(5, 3186715834);
	put(25, 741019578);
	printf("%d\n", get(3));
	put(15, 3268754311);
	put(28, 2018964216);
	put(11, 1434712982);
	*/
}
