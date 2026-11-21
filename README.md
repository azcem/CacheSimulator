# Cache Simulator

A cache simulator implemented in **C**, based on the Cache Lab from *Computer Systems: A Programmer's Perspective (CS:APP)*.

The simulator models a set-associative cache and processes memory access traces to track cache hits, misses, and evictions. It provides a practical exploration of cache architecture, memory addressing, and cache replacement policies.

## Features

- Configurable cache architecture:
  - Number of set index bits (`s`)
  - Set associativity (`E`)
  - Number of block offset bits (`b`)
- Memory address decomposition into:
  - Tag
  - Set index
  - Block offset
- Cache hit and miss detection.
- **Least Recently Used (LRU)** cache replacement policy.
- Memory trace processing.
- Hit, miss, and eviction statistics.
- Optional verbose output.

## Cache Architecture

The simulator models a cache with the following parameters:

| Parameter | Description |
|---|---|
| `S = 2^s` | Number of cache sets |
| `E` | Number of cache lines per set |
| `B = 2^b` | Block size in bytes |
| `C = S × E × B` | Total cache capacity |

Each cache line maintains the information required for cache simulation:

- Valid bit
- Tag
- LRU metadata

## How It Works

For each memory access in the input trace:

1. Extract the tag and set index from the memory address using bitwise operations.
2. Identify the corresponding cache set.
3. Search the set for a valid cache line with a matching tag.
4. If a matching line exists, record a cache hit and update its LRU metadata.
5. Otherwise, record a cache miss.
6. If an invalid cache line is available, allocate it.
7. If the set is full, evict the least recently used cache line and record an eviction.

## Building

Compile the simulator using GCC:

```bash
gcc -Wall -Wextra -std=c11 -o csim csim.c
```

Adjust the source filename if necessary.

## Usage

```bash
./csim -s <s> -E <E> -b <b> -t <tracefile>
```

Optional verbose mode:

```bash
./csim -v -s <s> -E <E> -b <b> -t <tracefile>
```

### Example

```bash
./csim -s 4 -E 2 -b 4 -t traces/yi.trace
```

This configures a cache with:

- 16 sets
- 2 cache lines per set
- 16 bytes per cache block
- 512 bytes of total cache capacity

## Example Output

```text
hits:10 misses:5 evictions:2
```

*Illustrative output; actual statistics depend on the input trace and cache configuration.*

## Concepts Explored

This project provided hands-on experience with several fundamental computer systems concepts:

- **Cache organization:** Direct-mapped and set-associative cache structures.
- **Memory hierarchy:** Understanding how caches interact with main memory.
- **Bit manipulation:** Extracting address fields using shifts and masks.
- **Cache replacement policies:** Implementing LRU to manage cache capacity.
- **Dynamic memory management:** Allocating and managing cache structures in C.
- **File processing:** Reading and interpreting memory access traces.
- **Performance analysis:** Understanding how memory access patterns influence cache behavior.

## Technologies

- C
- GCC
- Bitwise operations
- Dynamic memory allocation
- Linux / Unix command-line tools

## Reference

*Computer Systems: A Programmer's Perspective (CS:APP)*, Bryant and O'Hallaron.

Based on the Cache Lab programming assignment.
