#include "cachelab.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define QUEUE_MAX_SIZE 32

/* structs */

enum Operation { INSTRUCTION_ACCESS, DATA_LOAD, DATA_STORE, DATA_MODIFY };

enum Status { HIT, MISS, EVICT };

typedef struct {
  enum Operation operation;
  unsigned long long address;
} memory_access;

memory_access parse_text_line(char *text_line) {
  memory_access mem_access;
  mem_access.operation = INSTRUCTION_ACCESS;
  if (text_line[1] == 'M')
    mem_access.operation = DATA_MODIFY;
  else if (text_line[1] == 'L')
    mem_access.operation = DATA_LOAD;
  else if (text_line[1] == 'S')
    mem_access.operation = DATA_STORE;

  if (mem_access.operation != INSTRUCTION_ACCESS) {
    char **endptr = NULL;
    mem_access.address = strtoull(&text_line[3], endptr, 16);
  }
  return mem_access;
}

/* queue struct and funcs */
typedef struct {
  int items[QUEUE_MAX_SIZE];
  int front;
  int rear;
} queue;

void init_queue(queue *q) {
  q->front = -1;
  q->rear = -1;
}

bool is_empty_queue(queue *q) { return q->front == -1; }

bool is_full_queue(queue *q) {
  return (q->rear + 1) % QUEUE_MAX_SIZE == q->front;
}

void push_queue(queue *q, int data) {
  if (q->front == -1) {
    q->front = 0;
  }
  q->rear = (q->rear + 1) % QUEUE_MAX_SIZE;
  q->items[q->rear] = data;
}

int pop_queue(queue *q) {
  if (is_empty_queue(q))
    return -1;
  int data = q->items[q->front];
  if (q->front == q->rear) {
    q->front = q->rear = -1;
  } else {
    q->front = (q->front + 1) % QUEUE_MAX_SIZE;
  }
  return data;
}

/* cache structs */

typedef struct {
  int tag;
  char valid;
} line;

typedef struct {
  line *lines;
  queue lru_queue;
} set;

typedef struct {
  set *sets;
  int S;
  int E;
  int b;
  int s;
} cache;

/* init funcs */

// init line to have zero bytes in it, zero in valid bit
void init_line(line **lp, int E, int b) {
  *lp = calloc(1, sizeof(line));
  line *l = *lp;
  if (!l) {
    exit(1);
  }
}

// init set and initialize all its lines
void init_set(set **sp, int s, int E, int b) {
  set *sset = *sp;
  if (!sset) {
    exit(1);
  }
  sset->lines = calloc(E, sizeof(line));
  if (!sset->lines) {
    exit(1);
  }
  for (int i = 0; i < E; i++) {
    line *current_line = &(sset->lines[i]);
    init_line(&current_line, E, b);
  }
}

// init cache and set its parameters
void init_cache(cache **cp, int s, int E, int b) {
  *cp = calloc(1, sizeof(cache));
  cache *c = *cp;
  if (!c) {
    exit(1);
  }
  int S = (int)pow(2, s);
  c->sets = calloc(1, S * sizeof(set));
  if (!c->sets) {
    exit(1);
  }
  for (int i = 0; i < S; i++) {
    set *current_set = c->sets + i;
    init_set(&current_set, s, E, b);
  }

  c->S = S;
  c->E = E;
  c->b = b;
  c->s = s;
}

/* free funcs */

void free_line(line *l) {
  free(l);
  l = NULL;
}

void free_set(set *s, int E) {
  for (int i = 0; i < E; i++) {
    free_line(&s->lines[i]);
  }
  free(s);
  s = NULL;
}

void free_cache(cache *c) {
  for (int i = 0; i < c->S; i++) {
    free_set(&c->sets[i], c->E);
  }
  free(c);
  c = NULL;
}

/* cache funcs */

void print_cache(cache *c) {
  for (int i = 0; i < c->S; i++) {
    printf("Set %d addr= %p\n", i, (c->sets + i));
    for (int j = 0; j < c->E; j++) {
      printf("  line %d addr= %p\n", j, &(c->sets[i].lines[j]));
      printf("  valid= %d tag= %x\n", c->sets[i].lines[j].valid,
             c->sets[i].lines[j].tag);
    }
  }
}

enum Status query_cache(cache *c, unsigned long long address) {
  unsigned long long ones_mask = -1;

  unsigned long long t_s = address & (ones_mask << c->b);
  t_s = t_s >> c->b;

  unsigned long long s_mask = (1 << (c->s)) - 1;
  int s = t_s & s_mask;

  int t = t_s >> c->s;

  // printf("address= %llb\n", address);
  // printf("s_mask = %llb\n", s_mask);
  // printf("s = %b t= %b\n", s, t);
  // printf("t_s = %llb\n", t_s);
  set *current_set = &c->sets[s];
  int valid_count = 0;
  for (int i = 0; i < c->E; i++) {
    line *current_line = current_set->lines + i;
    if (current_line->valid)
      valid_count++;
    if (current_line->tag == t) {
      if (current_line->valid) {
        return HIT;
      }
    }
  }
  // miss, check if need to evict
  if (valid_count == c->E) {
    // get lru line
    int lru_line_index = pop_queue(&(current_set->lru_queue));
    line *lru_line = &current_set->lines[lru_line_index];
    lru_line->tag = t;
    lru_line->valid = 1;
    return EVICT;
  }

  // if valid_count less than E then there's an empty line and no need to evict
  for (int i = 0; i < c->E; i++) {
    line *current_line = &current_set->lines[i];
    if (!current_line->valid) {
      current_line->valid = true;
      current_line->tag = t;
      push_queue(&(current_set->lru_queue), i);
      break;
    }
  }
  return MISS;
}

int main(int argc, char **argv) {
  // input handling
  int s, E, b;
  FILE *fptr = NULL;
  for (int i = 1; i < argc; i++) {
    if (argv[i][1] == 's')
      s = atoi(argv[i + 1]);
    else if (argv[i][1] == 'E')
      E = atoi(argv[i + 1]);
    else if (argv[i][1] == 'b')
      b = atoi(argv[i + 1]);
    else if (argv[i][1] == 't')
      fptr = fopen(argv[i + 1], "r");
  }
  if (fptr == NULL) {
    printf("error loading file.\n");
    return 1;
  }

  // init cache with its parameters
  cache *c;
  init_cache(&c, s, E, b);

  int hits = 0;
  int misses = 0;
  int evictions = 0;
  // read file line by line
  char buffer[1024];
  while (fgets(buffer, 1024, fptr)) {
    // parse text_line -> get memory_access struct
    memory_access mem_access = parse_text_line(buffer);
    if (mem_access.operation == INSTRUCTION_ACCESS)
      continue;
    enum Status status = query_cache(c, mem_access.address);
    if (status == HIT)
      hits++;
    else if (status == MISS)
      misses++;
    else if (status == EVICT) {
      misses++;
      evictions++;
    }
    // if operation data modify check read hit and eviction
    // then add hit for store
    if (mem_access.operation == DATA_MODIFY)
      hits++;
  }

  // printSummary
  printSummary(hits, misses, evictions);
  return 0;
}
