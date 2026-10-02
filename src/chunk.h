#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_RETURN
} OpCode;

typedef struct {
  int line;
  int count;
} LineRun;

typedef struct {
  int count;    // number of bytes in code[]
  int capacity; // capacity of code[]
  uint8_t *code;
  LineRun *lines;   // run-length encoded line info
  int lineCount;    // number of runs in lines[]
  int lineCapacity; // capacity of lines[]
  ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
int getLine(Chunk *chunk, int instruction_index);
void writeConstant(Chunk *chunk, Value value, int line);

#endif
