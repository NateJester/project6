#include "common.h"
#include <stdlib.h>
#include <stdio.h>

struct entry {
	key_type key;
	value_type value;
};

struct kvstore {
	struct entry* entries;
	int max_size;
	int size;
};

int put(struct kvstore kvstore, key_type k, value_type v) {
	
}

int get(struct kvstore kvstore, key_type k) {
	
}

int main(int argc, char* argv[]) {
	struct kvstore* kvstore = malloc(sizeof(struct kvstore));
	if (kvstore == NULL) {
		printf("failed kvstore malloc");
		return -1;
	}
	kvstore->max_size = (int) argv[4];
	kvstore->size = 0;
	kvstore->entries = calloc(kvstore->max_size, sizeof(struct entry));
	if (kvstore->entries == NULL) {
		free(kvstore);
		printf("failed kvstore entries malloc");
		return -1;
	}
}
