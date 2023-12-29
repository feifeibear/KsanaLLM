/* Copyright 2023 Tencent Inc.  All rights reserved.

==============================================================================*/
#pragma once

#include <vector>
#include "numerous_llm/runtime/context.h"
#include "numerous_llm/utils/environment.h"
#include "numerous_llm/utils/id_generator.h"
#include "numerous_llm/utils/status.h"

namespace numerous_llm {

// The base class of all allocators.
// All the method must be thread-safe.
class BaseAllocator {
 public:
  BaseAllocator(const AllocatorConfig& allocator_config, std::shared_ptr<Context> context);
  virtual ~BaseAllocator() {}

  // Allocate blocked memory.
  virtual Status AllocateBlocks(size_t block_num, std::vector<int>& blocks);

  // Free blocked memory.
  virtual Status FreeBlocks(const std::vector<int>& blocks);

  // Allocate contiguous memory.
  virtual Status AllocateContiguous(size_t size, int& block_id);

  // Free contiguous memory.
  virtual Status FreeContiguous(int block_id);

  // Get memory address of blocked memory.
  virtual Status GetBlockPtrs(const std::vector<int>& blocks, std::vector<void*>& addrs);

  // Get memory address of contiguous memory.
  virtual Status GetContiguousPtr(int block_id, void*& addr);

  // Get number of free blocked memory.
  virtual int GetFreeBlockNumber();

  // Get number of used blocked memory.
  virtual int GetUsedBlockNumber();

 protected:
  // pre-allocate all blocks.
  void PreAllocateBlocks();

  // Allocate memory.
  virtual void AllocateMemory(void** memory_ptr, size_t bytes) = 0;

  // Free memory.
  virtual void FreeMemory(void* memory_ptr) = 0;

 protected:
  // The global id generator, used for all allocators.
  static IdGenerator id_generator_;

  // The current allocator config.
  AllocatorConfig allocator_config_;

  // The global context.
  std::shared_ptr<Context> context_ = nullptr;

  // The free and used blocks.
  std::unordered_map<int, MemoryBlock> free_blocks_;
  std::unordered_map<int, MemoryBlock> used_blocks_;

  // The used contiguous memory.
  std::unordered_map<int, MemoryBlock> used_contiguous_;

  // Make thread-safe.
  std::mutex block_mutex_;
  std::mutex contiguous_mutex_;
};

}  // namespace numerous_llm