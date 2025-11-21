#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <filesystem>
#include "Cache.h"

// 起点和终点的前缀常量
extern const std::string start_prefix;
extern const std::string end_prefix;

// 字符串工具函数
std::string trim(const std::string &str);

// 文件操作工具函数
bool find_test_files(const std::filesystem::path &case_path,
                     std::string &demand_file,
                     std::vector<std::string> &map_files);

bool read_demand(const std::string &filename, std::string &start, std::string &end);

// 输出工具函数
void print_usage();
void print_single_path(const std::string &title, const std::vector<std::string> &path);
void print_multi_paths(const MultiPath &paths);
void print_cache_statistics(PathCache *cache);

#endif
