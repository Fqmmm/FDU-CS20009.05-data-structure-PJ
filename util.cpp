#include "util.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

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
    std::cout << "Usage: .\\pathfinder --test_path <path_to_test_case_directory> [--no_cache]" << std::endl;
    std::cout << "       .\\pathfinder --clear-cache" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  --test_path <dir>  Specify the test case directory (required)" << std::endl;
    std::cout << "  --no_cache         Disable cache and force recalculation (optional)" << std::endl;
    std::cout << "  --clear-cache      Clear all cached results and exit" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  .\\pathfinder --test_path Test_Cases/test_cases/shanghai_test_cases/case1_simple" << std::endl;
    std::cout << "  .\\pathfinder --test_path Test_Cases/test_cases/shanghai_test_cases/case1_simple --no_cache" << std::endl;
    std::cout << "  .\\pathfinder --clear-cache" << std::endl;
}

// 打印单条路径（带装饰边框）
void print_single_path(const std::string &title, const std::vector<std::string> &path)
{
    std::cout << "\n┌─ " << title << " ────────────────────" << std::endl;
    if (!path.empty())
    {
        std::cout << "│ Path: ";
        for (size_t i = 0; i < path.size(); ++i)
        {
            std::cout << path[i] << (i == path.size() - 1 ? "" : " --> ");
        }
        std::cout << std::endl;
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
    // 输出时间最短路径
    std::cout << "\n┌─ Time-Optimized Path (时间最短) ────────────────────" << std::endl;
    if (!paths.time_path.empty())
    {
        std::cout << "│ Path: ";
        for (size_t i = 0; i < paths.time_path.size(); ++i)
        {
            std::cout << paths.time_path[i] << (i == paths.time_path.size() - 1 ? "" : " --> ");
        }
        std::cout << std::endl;
        std::cout << "│ Total Time: " << paths.time_path_time << " seconds" << std::endl;
        std::cout << "│ Total Distance: " << paths.time_path_distance << " meters" << std::endl;
    }
    else
    {
        std::cout << "│ No path found." << std::endl;
    }
    std::cout << "└─────────────────────────────────────────────────────" << std::endl;

    // 输出距离最短路径
    std::cout << "\n┌─ Distance-Optimized Path (距离最短) ────────────────" << std::endl;
    if (!paths.distance_path.empty())
    {
        std::cout << "│ Path: ";
        for (size_t i = 0; i < paths.distance_path.size(); ++i)
        {
            std::cout << paths.distance_path[i] << (i == paths.distance_path.size() - 1 ? "" : " --> ");
        }
        std::cout << std::endl;
        std::cout << "│ Total Time: " << paths.distance_path_time << " seconds" << std::endl;
        std::cout << "│ Total Distance: " << paths.distance_path_distance << " meters" << std::endl;
    }
    else
    {
        std::cout << "│ No path found." << std::endl;
    }
    std::cout << "└─────────────────────────────────────────────────────" << std::endl;

    // 输出综合推荐路径
    std::cout << "\n┌─ Balanced Path (综合推荐) ──────────────────────────" << std::endl;
    if (!paths.balanced_path.empty())
    {
        std::cout << "│ Path: ";
        for (size_t i = 0; i < paths.balanced_path.size(); ++i)
        {
            std::cout << paths.balanced_path[i] << (i == paths.balanced_path.size() - 1 ? "" : " --> ");
        }
        std::cout << std::endl;
        std::cout << "│ Total Time: " << paths.balanced_path_time << " seconds" << std::endl;
        std::cout << "│ Total Distance: " << paths.balanced_path_distance << " meters" << std::endl;
    }
    else
    {
        std::cout << "│ No path found." << std::endl;
    }
    std::cout << "└─────────────────────────────────────────────────────" << std::endl;

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
