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
void print_single_path(const std::string &title, const PathResult &result);
void print_multi_paths(const MultiPath &paths);
void print_cache_statistics(PathCache *cache);

// BPR拥堵函数
// 计算拥堵系数：1 + α × (V/C)^β
// current_vehicles: 道路上的车辆数（occupancy）
// lanes: 车道数
// length_meters: 道路长度（米）
// speed_limit_kmh: 道路限速（km/h）
double calculate_bpr_congestion_factor(int current_vehicles, int lanes,
                                       double length_meters, double speed_limit_kmh);

// 计算自由流通行时间（秒）
double calculate_free_flow_time(double length_meters, double speed_limit_kmh);

// 计算通行时间（秒）= 自由流时间 × 拥堵系数
double calculate_travel_time(double length_meters, double speed_limit_kmh, int lanes, int current_vehicles);

#endif
