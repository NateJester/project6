#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <stdio.h>

int init_ring(struct ring *r) {
	if (r == NULL) {
		return -1;
	}
	r->p_head = 0;
	r->c_head = 0;
	r->p_tail = 0;
	r->c_tail = 0;
	return 0;
}

void ring_submit(struct ring *r, struct buffer_descriptor *bd) {
	if (r == NULL || bd == NULL) {
		return;
	}
	uint32_t c_tail;
	uint32_t p_head;
	while (1) {
		c_tail = r->c_tail;
		p_head = r->p_head;
		while (p_head >= c_tail + RING_SIZE) {
		}
		if (atomic_compare_exchange_strong(&r->p_head, &p_head, p_head + 1)) {
			break;
		}
	}
	memcpy(&r->buffer[p_head % RING_SIZE], bd, sizeof(*bd));
	while (1) {
		if (r->p_tail == p_head) {
			r->p_tail = r->p_tail + 1;
			break;
		}
	}	
}

void ring_get(struct ring *r, struct buffer_descriptor *bd) {
	if (r == NULL || bd == NULL) {
		return;
	}
	uint32_t p_tail;
	uint32_t c_head;
	while (1) {
		c_head = r->c_head;
		p_tail = r->p_tail;
		while (c_head == p_tail) {
		}
		if (atomic_compare_exchange_strong(&r->c_head, &c_head, c_head + 1)) {
			break;
		}
	}
	memcpy(bd, &r->buffer[c_head % RING_SIZE], sizeof(*bd));
	while (1) {
		if(r->c_tail == c_head) {
			r->c_tail = r->c_tail + 1;
			break;
		}
	}
}
