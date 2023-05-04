// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/log_writer.h"

#include <cstdint>

#include "leveldb/env.h"

#include "util/coding.h"
#include "util/crc32c.h"

namespace leveldb {
namespace log {

static void InitTypeCrc(uint32_t* type_crc) {
  for (int i = 0; i <= kMaxRecordType; i++) {
    char t = static_cast<char>(i);
    type_crc[i] = crc32c::Value(&t, 1);
  }
}

Writer::Writer(WritableFile* dest) : dest_(dest), block_offset_(0) {
  InitTypeCrc(type_crc_);
}

Writer::Writer(WritableFile* dest, uint64_t dest_length)
    : dest_(dest), block_offset_(dest_length % kBlockSize) {
  InitTypeCrc(type_crc_);
}

Writer::~Writer() = default;

// 将slice记录添加到WAL log中
Status Writer::AddRecord(const Slice& slice) {
  const char* ptr = slice.data();
  size_t left = slice.size();

  // Fragment the record if necessary and emit it.  Note that if slice
  // is empty, we still want to iterate once to emit a single
  // zero-length record
  // 如果slice的过大，会进行分段。
  Status s;
  bool begin = true;
  do {
    const int leftover = kBlockSize - block_offset_;
    // 如果有剩余空间
    assert(leftover >= 0);
    // 剩余空间小于kHeaderSize
    if (leftover < kHeaderSize) {
      // Switch to a new block
      if (leftover > 0) {
        // Fill the trailer (literal below relies on kHeaderSize being 7)
        static_assert(kHeaderSize == 7, "");
        // 用\x00填充
        dest_->Append(Slice("\x00\x00\x00\x00\x00\x00", leftover));
      }
      block_offset_ = 0;
    }

    // Invariant: we never leave < kHeaderSize bytes in a block.
    assert(kBlockSize - block_offset_ - kHeaderSize >= 0);

    const size_t avail = kBlockSize - block_offset_ - kHeaderSize;
    const size_t fragment_length = (left < avail) ? left : avail;

    RecordType type;
    const bool end = (left == fragment_length);
    // 如果begin和end都为true，则说明该次写入的数据可以存储在一个block内
    if (begin && end) {
      type = kFullType;
    } else if (begin) {
      // 如果只有begin为true，则说明该次写入的数据需要分段，且当前是第一段
      type = kFirstType;
    } else if (end) {
      // 如果只有end为true，则说明该次写入的数据需要分段，且当前是最后段
      type = kLastType;
    } else {
      // 排除上述几种情况外，本次写入属于分段后的中间的一段
      type = kMiddleType;
    }
    // 将ptr~ptr+fragment_length的数据写入log中
    s = EmitPhysicalRecord(type, ptr, fragment_length);
    //写成功后更新ptr指针和剩余待写入的数据长度
    ptr += fragment_length;
    left -= fragment_length;
    begin = false;
  } while (s.ok() && left > 0);
  return s;
}

Status Writer::EmitPhysicalRecord(RecordType t, const char* ptr,
                                  size_t length) {
  assert(length <= 0xffff);  // Must fit in two bytes
  assert(block_offset_ + kHeaderSize + length <= kBlockSize);

  // Format the header
  char buf[kHeaderSize];
  // 4~5记录长度
  buf[4] = static_cast<char>(length & 0xff);
  buf[5] = static_cast<char>(length >> 8);
  // 记录record的类型
  buf[6] = static_cast<char>(t);

  // Compute the crc of the record type and the payload.
  uint32_t crc = crc32c::Extend(type_crc_[t], ptr, length);
  crc = crc32c::Mask(crc);  // Adjust for storage
  // 0~3记录crc校验码
  EncodeFixed32(buf, crc);

  // Write the header and the payload
  // 追加写入头信息
  Status s = dest_->Append(Slice(buf, kHeaderSize));
  if (s.ok()) {
    // 追加写入record数据
    s = dest_->Append(Slice(ptr, length));
    if (s.ok()) {
      s = dest_->Flush();
    }
  }
  block_offset_ += kHeaderSize + length;
  return s;
}

}  // namespace log
}  // namespace leveldb
