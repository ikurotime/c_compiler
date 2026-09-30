#include <stdint.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "value.h"

void initChunk(Chunk *chunk) {
  chunk->lineCount = 0;
  chunk->lineCapacity = 0;
  chunk->runCount = 0;
  chunk->runCapacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initValueArray(&chunk->constants);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->lineCapacity < chunk->lineCount + 1) {
    int oldCapacity = chunk->lineCapacity;
    chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->lineCapacity));
  }
  if (chunk->lines != NULL && chunk->lines->line == line) {
    chunk->lines->count++;
  } else {
    chunk->lines->line = line;
    chunk->lines->count = 1;
  }
  chunk->code[chunk->lineCount] = byte;
  chunk->lineCount++;
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
  FREE_ARRAY(uint8_t, chunk->code, chunk->lineCapacity);
  FREE_ARRAY(int, chunk->lines, chunk->runCapacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}

int addConstant(Chunk *chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}
