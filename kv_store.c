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

HashNode_t* hashtable[128];
pthread_mutex_t mutex[128];
int hash_size = 4;

struct thread_context {
	int tid;
	// other stuff maybe??
};

pthread_t threads[128];
struct thread_context contexts[128];
int num_threads = 4;

struct ring *ring = NULL;
char * shmem_area = NULL;

/*
 * Puts key/value pair in the hashtable
 */
int put(key_type k, value_type v) {
	
	int index = hash_function(k, hash_size);
	
	HashNode_t * newNode = (HashNode_t *) malloc(sizeof(HashNode_t));
	newNode->k = k;
	newNode->v = v;
	newNode->next = NULL;
	
	pthread_mutex_lock(&mutex[index]);
	
	if (hashtable[index] == NULL) {
		hashtable[index] = newNode;
	} else {
		HashNode_t * temp = hashtable[index];
		while (temp->next != NULL && temp->k != k) {
			temp = temp->next;
		}
		if (temp->k == k) {
			temp->v = v;
		} else {
			temp->next = newNode;
		}
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

       int fd = shm_open("shmem_file", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        
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
	while ((op = getopt(argc, argv, "n:s:")) != -1) {
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

/*
 * Function that's run by each thread
 * @param arg context for this thread
*/
void *thread_function(void *arg) {
    struct thread_context *ctx = arg;
	printf("tid %d\n", ctx->tid);
    // TODO infinite loop to fetch requests from buffer using ring_get
}

/*
 * Launch num_threads number of threads
 * Prepares the context for each thread
*/
void start_threads() {
    for (int i = 0; i < num_threads; i++) {
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

	if (parse_args(argc, argv) != 0) {
	    exit(EXIT_FAILURE);
	}
	// initialize locks for each hash index
	for (int i = 0; i < hash_size; i++) {
		pthread_mutex_init(&mutex[i], NULL);
	}
	
	// TODO get shared memory... init_server();
	init_server();

	start_threads();

//	wait_for_threads();
}
