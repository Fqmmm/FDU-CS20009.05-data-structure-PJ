#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include "Graph.h"
#include "Cache.h"
#include "config.h"

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

// 打印路径结果
void print_path(const std::vector<std::string> &path)
{
    if (path.empty())
    {
        std::cout << "No path found." << std::endl;
    }
    else
    {
        std::cout << "The shortest path is: " << std::endl;
        for (size_t i = 0; i < path.size(); ++i)
        {
            std::cout << path[i] << (i == path.size() - 1 ? "" : " --> ");
        }
        std::cout << std::endl;
    }
}

// 辅助函数：判断两个路径是否相同
bool paths_equal(const std::vector<std::string> &path1, const std::vector<std::string> &path2)
{
    if (path1.size() != path2.size())
        return false;

    for (size_t i = 0; i < path1.size(); ++i)
    {
        if (path1[i] != path2[i])
            return false;
    }

    return true;
}

// 处理单个地图文件，查找并打印最短路径
void process_map(const std::string &map_file, const std::string &start_node, const std::string &end_node,
                 PathCache *cache, bool use_cache)
{
    std::cout << "\n========================================================" << std::endl;
    std::cout << "Processing map: " << map_file << std::endl;
    std::cout << "========================================================" << std::endl;

    MultiPath cached_paths;
    bool cache_hit = false;

    // 尝试从缓存获取（如果启用缓存）
    if (use_cache && cache != nullptr)
    {
        // 记录查询前的hit_count，通过hit_count变化判断是否命中
        size_t old_hit_count = cache->get_hit_count();
        cached_paths = cache->get(start_node, end_node, map_file);
        cache_hit = (cache->get_hit_count() > old_hit_count);

        if (cache_hit)
        {
            std::cout << "\n[Cache Hit] Using cached results.\n" << std::endl;
        }
        else
        {
            std::cout << "\n[Cache Miss] Computing paths using three different strategies...\n" << std::endl;
        }
    }
    else if (!use_cache)
    {
        std::cout << "\n[Cache Disabled] Computing paths using three different strategies...\n" << std::endl;
    }
    else
    {
        std::cout << "\nComputing paths using three different strategies...\n" << std::endl;
    }

    MultiPath paths;

    if (cache_hit)
    {
        // 使用缓存的路径
        paths = cached_paths;
    }
    else
    {
        // 缓存未命中或禁用缓存，执行Dijkstra算法
        Graph city_map;
        if (!city_map.from_csv(map_file))
        {
            std::cerr << "Error: Failed to load map file " << map_file << std::endl;
            return;
        }

        // 计算三种路径
        paths.time_path = city_map.find_shortest_path(start_node, end_node, WeightMode::TIME);
        paths.distance_path = city_map.find_shortest_path(start_node, end_node, WeightMode::DISTANCE);
        paths.balanced_path = city_map.find_shortest_path(start_node, end_node, WeightMode::BALANCED);

        // 保存到缓存（如果启用缓存）
        // 注意：即使路径为空（无路径），也应该缓存，避免重复计算
        if (use_cache && cache != nullptr)
        {
            cache->put(start_node, end_node, map_file, paths);
        }
    }

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
    }
    else
    {
        std::cout << "│ No path found." << std::endl;
    }
    std::cout << "└─────────────────────────────────────────────────────" << std::endl;

    std::cout << "\n";
}

int main(int argc, char *argv[])
{
// 设置控制台编码
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    // 特殊处理：清空缓存命令
    if (argc == 2 && std::string(argv[1]) == "--clear-cache")
    {
        std::cout << "Clearing all cached results..." << std::endl;

        try
        {
            PathCache cache(CacheConfig::cache_dir, CacheConfig::max_size);
            size_t entry_count = cache.get_entry_count();
            cache.clear();

            std::cout << "Cache cleared successfully!" << std::endl;
            std::cout << "  Removed " << entry_count << " cache entries." << std::endl;
            std::cout << "  Cache directory: " << CacheConfig::cache_dir << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error while clearing cache: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    // 解析命令行参数
    if (argc < 3)
    {
        print_usage();
        return 1;
    }

    std::string test_path;
    bool use_cache = true; // 默认启用缓存

    // 解析所有参数
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--test_path")
        {
            if (i + 1 < argc)
            {
                test_path = argv[i + 1];
                i++; // 跳过下一个参数（路径值）
            }
            else
            {
                std::cerr << "Error: --test_path requires a path argument" << std::endl;
                print_usage();
                return 1;
            }
        }
        else if (arg == "--no_cache")
        {
            use_cache = false;
        }
        else if (arg == "--clear-cache")
        {
            std::cerr << "Error: --clear-cache cannot be used with other arguments" << std::endl;
            print_usage();
            return 1;
        }
        else if (arg != test_path)
        {
            std::cerr << "Error: Unknown argument: " << arg << std::endl;
            print_usage();
            return 1;
        }
    }

    if (test_path.empty())
    {
        std::cerr << "Error: --test_path is required" << std::endl;
        print_usage();
        return 1;
    }

    std::filesystem::path case_path(test_path);
    if (!std::filesystem::is_directory(case_path))
    {
        std::cerr << "Error: Provided path is not a valid directory: " << case_path.string() << std::endl;
        return 1;
    }

    // 查找测试文件
    std::string demand_file;
    std::vector<std::string> map_files;
    if (!find_test_files(case_path, demand_file, map_files))
    {
        return 1;
    }

    // 读取起点和终点
    std::string start_node, end_node;
    if (!read_demand(demand_file, start_node, end_node))
    {
        return 1;
    }
    std::cout << "Request: Find path from \"" << start_node << "\" to \"" << end_node << "\"." << std::endl;

    // 创建缓存对象（如果启用缓存）
    PathCache *cache = nullptr;
    if (use_cache)
    {
        cache = new PathCache(CacheConfig::cache_dir, CacheConfig::max_size);
        std::cout << "\n[Cache] Cache enabled. Max entries: " << CacheConfig::max_size << std::endl;
    }
    else
    {
        std::cout << "\n[Cache] Cache disabled (--no_cache flag set)" << std::endl;
    }

    // 处理每个地图文件
    for (const auto &map_file : map_files)
    {
        process_map(map_file, start_node, end_node, cache, use_cache);
    }

    // 输出缓存统计信息
    if (use_cache && cache != nullptr)
    {
        std::cout << "========================================================" << std::endl;
        std::cout << "Cache Statistics:" << std::endl;
        std::cout << "  Hits: " << cache->get_hit_count() << std::endl;
        std::cout << "  Misses: " << cache->get_miss_count() << std::endl;
        std::cout << "  Entries: " << cache->get_entry_count() << std::endl;
        std::cout << "========================================================" << std::endl;

        delete cache;
    }

    return 0;
}
