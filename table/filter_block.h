// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A filter block is stored near the end of a Table file.  It contains
// filters (e.g., bloom filters) for all data blocks in the table combined
// into a single filter block.

#ifndef STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
#define STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "leveldb/slice.h"

#include "util/hash.h"

namespace leveldb {

class FilterPolicy;

// A FilterBlockBuilder is used to construct all of the filters for a
// particular Table.  It generates a single string which is stored as
// a special block in the Table.
//
// The sequence of calls to FilterBlockBuilder must match the regexp:
//      (StartBlock AddKey*)* Finish
// FilterBlockBuilder 用来构建SSTable中的Filter Block
class FilterBlockBuilder {
 public:
  explicit FilterBlockBuilder(const FilterPolicy*);

  FilterBlockBuilder(const FilterBlockBuilder&) = delete;
  FilterBlockBuilder& operator=(const FilterBlockBuilder&) = delete;

  void StartBlock(uint64_t block_offset);
  // 往Filter Block中添加key
  void AddKey(const Slice& key);
  // 生成Filter Block的数据
  Slice Finish();

 private:
  // 生成过滤器
  void GenerateFilter();

  // 过滤器的实现接口
  const FilterPolicy* policy_;
  // 扁平化存储的key的列表
  std::string keys_;  // Flattened key contents
  // 每个key在keys_中的开始的位置
  std::vector<size_t> start_;  // Starting index in keys_ of each key
  // 过滤器的数据
  std::string result_;           // Filter data computed so far
  std::vector<Slice> tmp_keys_;  // policy_->CreateFilter() argument
  // 存储过滤器的每段数据的偏移量
  std::vector<uint32_t> filter_offsets_;
};

// FilterBlockReader 用来实现布隆过滤器的读取
class FilterBlockReader {
 public:
  // REQUIRES: "contents" and *policy must stay live while *this is live.
  FilterBlockReader(const FilterPolicy* policy, const Slice& contents);
  bool KeyMayMatch(uint64_t block_offset, const Slice& key);

 private:
  // 过滤器的实现接口
  const FilterPolicy* policy_;
  const char* data_;    // Pointer to filter data (at block-start)
  // 
  const char* offset_;  // Pointer to beginning of offset array (at block-end)
  // 过滤器的个数
  size_t num_;          // Number of entries in offset array
  size_t base_lg_;      // Encoding parameter (see kFilterBaseLg in .cc file)
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
