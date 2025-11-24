#include "util.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <limits>

const std::string start_prefix = "起点：";
const std::string end_prefix = "终点：";

// 辅助函数：移除字符串首尾的空白字符
std::string trim(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// 查找测试用例目录中的 demand 文件和 map 文件
bool find_test_files(const std::filesystem::path &case_path,
                     std::string &demand_file,
                     std::vector<std::string> &map_files)
{
    for (const auto &entry : std::filesystem::directory_iterator(case_path))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            if (filename.find(".txt") != std::string::npos)
            {
                demand_file = entry.path().string();
            }
            else if (filename.rfind("map_", 0) == 0 && filename.find(".csv") != std::string::npos)
            {
                map_files.push_back(entry.path().string());
            }
        }
    }

    // 检查是否找到了需要的文件
    if (demand_file.empty())
    {
        std::cerr << "Error: No .txt demand file found in " << case_path.string() << std::endl;
        return false;
    }
    if (map_files.empty())
    {
        std::cerr << "Error: No map_*.csv files found in " << case_path.string() << std::endl;
        return false;
    }

    // 按文件名排序，以确保按时间顺序处理
    std::sort(map_files.begin(), map_files.end());
    return true;
}

// 辅助函数，从demand.txt文件中读取起点和终点
bool read_demand(const std::string &filename, std::string &start, std::string &end)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open demand file " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // 移除字符串末尾可能存在的回车符
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.rfind(start_prefix, 0) == 0)
        {
            start = trim(line.substr(start_prefix.length()));
        }

        if (line.rfind(end_prefix, 0) == 0)
        {
            end = trim(line.substr(end_prefix.length()));
        }
    }

    if (start.empty() || end.empty())
    {
        std::cerr << "Error: Could not find start or end node in " << filename << std::endl;
        return false;
    }

    return true;
}

// 打印使用说明
void print_usage()
{
    std::cout << "Usage: .\\pathfinder --test-path <path_to_test_case_directory> [--no-cache]" << std::endl;
    std::cout << "       .\\pathfinder --clear-cache" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --test-path <dir>  Specify the test case directory (required)" << std::endl;
    std::cout << "  --no-cache         Disable cache and force recalculation (optional)" << std::endl;
    std::cout << "  --clear-cache      Clear all cached results and exit" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  .\\pathfinder --test-path Test_Cases/test_cases/shanghai_test_cases/case1_simple" << std::endl;
    std::cout << "  .\\pathfinder --test-path Test_Cases/test_cases/shanghai_test_cases/case1_simple --no-cache" << std::endl;
    std::cout << "  .\\pathfinder --clear-cache" << std::endl;
}

// 打印单条路径（带装饰边框）
void print_single_path(const std::string &title, const PathResult &result)
{
    std::cout << "\n┌─ " << title << " ──────────────────────────────────────────" << std::endl;
    if (!result.path.empty())
    {
        std::cout << "│ Path: ";
        for (size_t i = 0; i < result.path.size(); ++i)
        {
            std::cout << result.path[i] << (i == result.path.size() - 1 ? "" : " --> ");
        }
        std::cout << std::endl;
        std::cout << "│ Total Time: " << result.time << " seconds" << std::endl;
        std::cout << "│ Total Distance: " << result.distance << " meters" << std::endl;
    }
    else
    {
        std::cout << "│ No path found." << std::endl;
    }
    std::cout << "└─────────────────────────────────────────────────────" << std::endl;
}

// 打印所有三种路径
void print_multi_paths(const MultiPath &paths)
{
    print_single_path("时间最短", paths.time_path);
    print_single_path("距离最短", paths.distance_path);
    print_single_path("综合推荐", paths.balanced_path);
    std::cout << "\n";
}

// 打印缓存统计信息
void print_cache_statistics(PathCache *cache)
{
    if (cache == nullptr)
    {
        return;
    }

    std::cout << "========================================================" << std::endl;
    std::cout << "Cache Statistics:" << std::endl;
    std::cout << "  Hits: " << cache->get_hit_count() << std::endl;
    std::cout << "  Misses: " << cache->get_miss_count() << std::endl;
    std::cout << "  Entries: " << cache->get_entry_count() << std::endl;
    std::cout << "========================================================" << std::endl;
}

// BPR拥堵函数实现
// 拥堵系数：1 + α × (V/C)^β
double calculate_bpr_congestion_factor(int current_vehicles, int lanes,
                                       double length_meters, double speed_limit_kmh)
{
    if (lanes <= 0 || speed_limit_kmh <= 0)
    {
        return std::numeric_limits<double>::infinity();
    }

    // 计算自由流通行时间（小时）
    double speed_mps = speed_limit_kmh * 1000.0 / 3600.0;
    double travel_time_sec = length_meters / speed_mps;
    double travel_time_hour = travel_time_sec / 3600.0;

    // 将occupancy转换为flow（veh/h）
    // flow = occupancy / travel_time
    double volume = static_cast<double>(current_vehicles) / travel_time_hour;

    // 计算道路容量（每小时）
    double capacity = lanes * BPRConfig::lane_capacity;

    // 计算流量/容量比
    double volume_capacity_ratio = volume / capacity;

    // BPR 公式：1 + α × (V/C)^β
    return 1.0 + BPRConfig::alpha * std::pow(volume_capacity_ratio, BPRConfig::beta);
}

// 计算自由流通行时间（秒）
double calculate_free_flow_time(double length_meters, double speed_limit_kmh)
{
    if (speed_limit_kmh <= 0)
    {
        return std::numeric_limits<double>::infinity();
    }

    // 转换为 m/s
    double speed_mps = speed_limit_kmh * 1000.0 / 3600.0;
    return length_meters / speed_mps;
}

// 计算通行时间（秒）= 自由流时间 × 拥堵系数
double calculate_travel_time(double length_meters, double speed_limit_kmh, int lanes, int current_vehicles)
{
    double free_flow_time = calculate_free_flow_time(length_meters, speed_limit_kmh);
    double congestion_factor = calculate_bpr_congestion_factor(current_vehicles, lanes, length_meters, speed_limit_kmh);
    return free_flow_time * congestion_factor;
}
