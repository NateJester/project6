#include "ring_buffer.h"

int init_ring(struct ring *r) {
	r->p_head = 0;
	r->c_head = 0;
	r->p_tail = 0;
	r->c_tail = 0;
}

void ring_submit(struct ring *r, struct buffer_descriptor *bd) {
	uint32_t p_head = r->p_head;
	uint32_t c_tail = r->c_tail;
	if (c_tail > RING_SIZE) {
		//block (don't remember how to do yet)
	}
	uint32_t p_next = p_head + 1;
	r->p_head = p_next;
	r->buffer[p_head] = *bd;
	r->p_tail = r->p_head;
	
	
}

void ring_get(struct ring *r, struct buffer_descriptor *bd) {
	
}
