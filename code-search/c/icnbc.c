#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define ALPHABET_LEN 7

void usage(char *argv0) {
	fprintf(stderr, "%s: <n> <min_ld> <min_iso>\n", argv0);
}

typedef struct codeword_list_t {
	uint16_t index;
	uint8_t *codewords;
} codeword_list;

codeword_list *new_codeword_list(uint8_t n, int size) {
	codeword_list *cw_list;
	cw_list = malloc(sizeof(cw_list));
	if (cw_list==NULL)
		exit(1);
	cw_list->index = 0;
	cw_list->codewords = malloc(size * n);
	if (cw_list->codewords==NULL)
		exit(1);
	int i;
	for(i=0; i<size*n; i++)
		cw_list->codewords[i] = 0x0f;
	return cw_list;
}

void delete_codeword_list(codeword_list *list) {
	free(list->codewords);
	free(list);
}

static inline void copy_codeword(uint8_t *src, int src_offset,
								 uint8_t *dst, int dst_offset,
								 int n) {
	int i;
	uint8_t *from, *to;
	from = src + src_offset * n;
	to = dst + dst_offset * n;
	//printf("Copying: ");
	for(i=0; i<n; i++) {
		//printf("%d ", from[i]);
		to[i] = from[i];
	}
	//printf("\n");
}

void print_code(codeword_list *a_code, codeword_list *b_code, uint8_t n) {
	int i,j,k;
	if (b_code)
		printf("%d %d %d [", a_code->index + b_code->index, a_code->index, b_code->index);
	else
		printf("%d [", a_code->index);
	
	for(i=0; i<a_code->index; i++) {
		j = i*n;
		printf("(");
		for(k=j; k<j+n-1; k++)
			printf("%d, ", a_code->codewords[k]);
		printf("%d", a_code->codewords[k]);
		printf("), ");
	}
	if (b_code) {
		printf("] [");
		for(i=0; i<b_code->index; i++) {
			j = i*n;
			printf("(");
			for(k=j; k<j+n-1; k++)
				printf("%d, ", b_code->codewords[k]);
			printf("%d", b_code->codewords[k]);
			printf("), ");
		}
	}
	printf("]\n");
}

int lee_distance(uint8_t* x, uint8_t *y, uint8_t n) {
	int i, ld = 0;
	//printf("Lee distance\n");
	for(i=0; i<n; i++) {
		//printf("%d %d\n", x[i], y[i]);
		ld += MIN((x[i]-y[i]), ALPHABET_LEN-(x[i]-y[i]));
	}
	ld %= ALPHABET_LEN;
	ld = abs(ld);
	//printf("distance=%d\n", ld);
	return ld;
}

codeword_list* populate_candidates(codeword_list* code,
								   codeword_list *candidates,
								   uint8_t n, uint8_t min_dist)
{
	codeword_list* next_candidates;
	int i;
	next_candidates = new_codeword_list(n, candidates->index);
	next_candidates->index = 0;
	uint8_t *codeword = code->codewords + (code->index-1)*n;
	for (i=0; i<candidates->index; i++) {
		if (lee_distance(candidates->codewords + n*i, codeword, n) >= min_dist) {
			copy_codeword(candidates->codewords, i, next_candidates->codewords,
						  next_candidates->index++, n);
		}
	}
	return next_candidates;
}

codeword_list *create_search_space(uint8_t n) {
	int size, i, j, k;
	codeword_list *space;
	
	size = pow(ALPHABET_LEN, n);
	space = new_codeword_list(n, size);
	space->index = size;
	
	j = (size-1)*n;
	for(i=0; i<n; i++)
		space->codewords[i+j] = 0;
	
	for(i=size-2; i>=0; i--) {
		j = i*n;
		copy_codeword(space->codewords, i+1, space->codewords, i, n);
		
		for(k=n-1; k>=0; k--) {
			space->codewords[j+k] = (space->codewords[j+k] + 1) % ALPHABET_LEN;
			if(space->codewords[j+k])
				break;
		}
	}
	//print_code(space, NULL, n);
	//exit(0);
	return space;
}

uint16_t find_comp(codeword_list *a_code, codeword_list *b_code,
				   codeword_list* candidates, uint8_t n, uint8_t min_ld,
				   uint16_t min_b_len, uint16_t min_len) {
	codeword_list *next_candidates;
	uint16_t best_len, longest = 0;
	
	while (candidates->index--) {
		copy_codeword(candidates->codewords, candidates->index,
					  b_code->codewords, b_code->index++, n);
		
		next_candidates = populate_candidates(b_code, candidates, n, min_ld);
		
		/* only look for b_code at least as long as the longest we have found */
		if ((candidates->index + b_code->index) >= min_b_len) {
			if ((candidates->index + b_code->index + a_code->index) >= min_len) {
				if (candidates->index) {
					best_len = find_comp(a_code, b_code, next_candidates, n,
										 min_ld, min_b_len, min_len);
					if (best_len >= min_len) {
						longest = best_len;
						min_len = best_len;
					}
				} else {
					print_code(a_code, b_code, n);
					if ((a_code->index + b_code->index) >= min_len)
						longest = a_code->index + b_code->index;
				}
			}
		}
		b_code->index--;
	}
	return longest;
}

int find_iso(codeword_list *code, codeword_list *candidates,
			 codeword_list *b_candidates, uint8_t n, uint8_t min_ld,
			 uint8_t min_iso, uint16_t a_len, uint16_t min_b_len,
			 uint16_t min_len) {
	codeword_list *next_candidates, *next_b_candidates;
	uint16_t max_b_len, total_len, best_len, longest = 0;
	int i, j;
	
	while (candidates->index--) {
		copy_codeword(candidates->codewords, candidates->index,
					  code->codewords, code->index++, n);
		
		next_candidates = populate_candidates(code, candidates, n, min_ld);
		next_b_candidates = populate_candidates(code, b_candidates, n, min_iso);
		
		if (((next_candidates->index + code->index) >= a_len) &&
			(next_b_candidates->index >= min_b_len)) {
			if (code->index == a_len) {
				codeword_list *b_code = new_codeword_list(n, next_b_candidates->index);
				best_len = find_comp(code, b_code, next_b_candidates, n, min_ld,
									 min_b_len, min_len);
				if (best_len >= min_len) {
					longest = best_len;
					min_len = best_len;
				}
				delete_codeword_list(b_code);
			}
			if ((next_candidates->index) && (code->index < a_len)) {
				best_len = find_iso(code, next_candidates, next_b_candidates, n,
									min_ld, min_iso, a_len, min_b_len, min_len);
				if (best_len >= min_len) {
					longest = best_len;
					min_len = best_len;
				}
			}
		}
		code->index--;
	}
	delete_codeword_list(next_candidates);
	delete_codeword_list(next_b_candidates);
	return longest;
}

int find_iso_from_start(uint8_t n, uint8_t min_ld, uint8_t min_iso,
						uint16_t a_len, uint16_t min_b_len,
						codeword_list *candidates) {
	codeword_list *code, *next_candidates, *next_b_candidates;
	int i, longest;
	
	code = new_codeword_list(n, candidates->index);
	for(i=0; i<n; i++)
		code->codewords[i] = 0;
	code->index = 1;
	
	next_candidates = populate_candidates(code, candidates, n, min_ld);
	next_b_candidates = populate_candidates(code, candidates, n, min_iso);
	
	longest = find_iso(code, next_candidates, next_b_candidates, n, min_ld,
					min_iso, a_len, min_b_len, a_len + min_b_len);
	
	delete_codeword_list(next_candidates); 
	delete_codeword_list(next_b_candidates);
	delete_codeword_list(code);
	return longest;
}

void find_best_iso(uint8_t n, uint8_t min_ld, uint8_t min_iso)
{
	uint16_t a_len, min_b_len, longest, longest_b;
	codeword_list *candidates;
	min_b_len = 2;
	candidates = create_search_space(n);
	
	for (a_len = pow(ALPHABET_LEN, n-1); a_len >= min_b_len; a_len--) {
		printf("trying a: %d, min b: %d, total: %d\n", a_len,
				min_b_len, a_len + min_b_len);
		longest = find_iso_from_start(n, min_ld, min_iso, a_len, min_b_len,
									  candidates);
		if (longest >= (a_len + min_b_len)) {
			min_b_len = longest - a_len + 1;
		}
	}
	delete_codeword_list(candidates);

}

int main(int argc, char** argv)
{
	if (argc < 4) {
		usage(argv[0]);
		exit(1);
	}

	uint8_t n, min_ld, min_iso;
	if ((n 		 = atoi(argv[1])) == 0 ||
		(min_ld  = atoi(argv[2])) == 0 ||
		(min_iso = atoi(argv[3])) == 0) {
		usage(argv[0]);
		exit(1);
	}
	
	find_best_iso(n, min_ld, min_iso);

	return 0;
}
