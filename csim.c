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

/* cache structs */

typedef struct {
  unsigned long long last_used;
  unsigned long long tag;
  char valid;
} line;

typedef struct {
  line *lines;
} set;

typedef struct {
  set *sets;
  unsigned long long clock;
  int S;
  int E;
  int b;
  int s;
} cache;

/* init funcs */

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
}

// init cache and set its parameters
void init_cache(cache **cp, int s, int E, int b) {
  *cp = calloc(1, sizeof(cache));
  cache *c = *cp;
  if (!c) {
    exit(1);
  }
  int S = 1 << s;
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

void free_sets(set *s, int S) {
  for (int i = 0; i < S; i++) {
    free(s[i].lines);
  }
}

void free_cache(cache *c) {
  free_sets(c->sets, c->S);
  free(c->sets);
  free(c);
  c = NULL;
}

/* cache funcs */

void print_cache(cache *c) {
  for (int i = 0; i < c->S; i++) {
    printf("Set %d addr= %p\n", i, (c->sets + i));
    for (int j = 0; j < c->E; j++) {
      printf("  line %d addr= %p\n", j, &(c->sets[i].lines[j]));
      printf("  valid= %d tag= %llx\n", c->sets[i].lines[j].valid,
             c->sets[i].lines[j].tag);
    }
  }
}

enum Status query_cache(cache *c, unsigned long long address) {
  unsigned long long s = (address >> c->b) & ((1ULL << c->s) - 1);
  unsigned long long t = address >> (c->s + c->b);
  set *current_set = c->sets + s;
  c->clock++;

  int empty = -1;
  int lru = 0;
  for (int i = 0; i < c->E; i++) {
    line *current_line = current_set->lines + i;
    if (current_line->valid) {
      if (current_line->tag == t) {
        current_line->last_used = c->clock;
        return HIT;
      }
    }
    if (!current_line->valid && empty == -1) {
      empty = i;
    } else if (current_line->last_used < current_set->lines[lru].last_used) {
      lru = i;
    }
  }
  int victim = (empty >= 0) ? empty : lru;

  line *v = current_set->lines + victim;
  v->valid = 1;
  v->tag = t;
  v->last_used = c->clock;
  return (empty >= 0) ? MISS : EVICT;
}

int main(int argc, char **argv) {
  // input handling
  int s, E, b;
  FILE *fptr = NULL;
  bool verbose = false;
  for (int i = 1; i < argc; i++) {
    if (argv[i][1] == 's')
      s = atoi(argv[i + 1]);
    else if (argv[i][1] == 'E')
      E = atoi(argv[i + 1]);
    else if (argv[i][1] == 'b')
      b = atoi(argv[i + 1]);
    else if (argv[i][1] == 't')
      fptr = fopen(argv[i + 1], "r");
    else if (argv[i][1] == 'v')
      verbose = true;
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
    if (verbose) {
      // remove \n
      int i = 0;
      while (buffer[i] != '\n') {
        i++;
      }
      buffer[i] = '\0';
      printf("%s ", buffer + 1);
      if (status == HIT)
        printf("hit");
      if (status == EVICT)
        printf("miss eviction");
      if (status == MISS)
        printf("miss");
      if (mem_access.operation == DATA_MODIFY)
        printf(" hit");
      printf("\n");
    }
  }

  // printSummary
  printSummary(hits, misses, evictions);
  free_cache(c);
  fclose(fptr);
  return 0;
}
