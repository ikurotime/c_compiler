#include <stdint.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "value.h"

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lineCount = 0;
  chunk->lineCapacity = 0;
  chunk->lines = NULL;
  initValueArray(&chunk->constants);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
  /* The code array grows one byte at a time, as before. */
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity));
  }
  chunk->code[chunk->count] = byte;
  chunk->count++;

  /* The runs array grows only when a new run is started. */
  if (chunk->lineCount > 0 &&
      chunk->lines[chunk->lineCount - 1].line == line) {
    chunk->lines[chunk->lineCount - 1].count++;
  } else {
    if (chunk->lineCapacity < chunk->lineCount + 1) {
      int oldCapacity = chunk->lineCapacity;
      chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
      chunk->lines = GROW_ARRAY(LineRun, chunk->lines, oldCapacity,
                                chunk->lineCapacity));
    }
    chunk->lines[chunk->lineCount].line = line;
    chunk->lines[chunk->lineCount].count = 1;
    chunk->lineCount++;
  }
}

int getLine(Chunk *chunk, int instruction_index) {
  for (int i = 0; i < chunk->lineCount; ++i) {
    if (instruction_index < chunk->lines[i].count) {
      return chunk->lines[i].line;
    } else {
      instruction_index -= chunk->lines[i].count;
    }
  }
  return -1;
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(LineRun, chunk->lines, chunk->lineCapacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}

int addConstant(Chunk *chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}
