#include <iostream>
#include <string>

#include "leveldb/iterator.h"
#include "leveldb/slice.h"

#include "include/leveldb/db.h"
#include "include/leveldb/options.h"
#include "include/leveldb/status.h"

void test_leveldb_iterator(leveldb::DB* db) {
  std::cout<<"=============test iterator============"<<std::endl;
  leveldb::Iterator* iter = db->NewIterator(leveldb::ReadOptions());
  for (iter->SeekToLast(); iter->Valid(); iter->Prev()) {
    std::cout << "key:" << iter->key().ToString() <<",value:"<< iter->value().ToString()
              << std::endl;
  }
  delete iter;
}

// g++ main.cpp -I ../ -I ../include/  -std=c++11 -L ../build
// https://blog.csdn.net/HaoZiHuang/article/details/126754794?spm=1001.2101.3001.6650.1&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EAD_ESQUERY%7Eyljh-1-126754794-blog-125991669.235%5Ev27%5Epc_relevant_3mothn_strategy_and_data_recovery&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EAD_ESQUERY%7Eyljh-1-126754794-blog-125991669.235%5Ev27%5Epc_relevant_3mothn_strategy_and_data_recovery&utm_relevant_index=2
// https://stackoverflow.com/questions/6141147/how-do-i-include-a-path-to-libraries-in-g
int main() {
  leveldb::DB* db;
  std::string filename = "test";
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, filename, &db);
  if (!status.ok()) {
    std::cout << "there is open leveldb failed" << std::endl;
    return -1;
  }
  std::cout << "there is open leveldb success" << std::endl;
  leveldb::WriteOptions wo;
  leveldb::Slice key("hello");
  leveldb::Slice value("world");
  status = db->Put(wo, key, value);
  if (!status.ok()) {
    std::cout << "leveldb execute put('hello','world') failed" << std::endl;
    return -1;
  }
  std::cout << "leveldb put key:" << key.ToString()
            << ",value:" << value.ToString() << " success" << std::endl;
  leveldb::ReadOptions ro;
  std::string rValue;
  status = db->Get(ro, key, &rValue);
  if (!status.ok()) {
    std::cout << "leveldb execute get('hello') failed" << std::endl;
    return -1;
  }
  std::cout << "leveldb get key:" << key.ToString() << ",value:" << rValue
            << std::endl;
  for (int i = 0; i < 100000; i++) {
    key = leveldb::Slice(std::to_string(i));
    db->Put(wo, key, key);
  }
  test_leveldb_iterator(db);
  delete (db);
  return 0;
}
