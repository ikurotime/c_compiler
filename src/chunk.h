#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_RETURN
} OpCode;

typedef struct {
  int line;
  int count;
} LineRun;

typedef struct {
  int lineCount;
  int lineCapacity;
  int runCount;
  int runCapacity;
  uint8_t *code;
  LineRun *lines;
  ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
int getLine(Chunk *chunk, int instruction_index);

#endif
