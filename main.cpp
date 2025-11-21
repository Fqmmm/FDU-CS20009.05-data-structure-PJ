#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include "Graph.h"
#include "Cache.h"
#include "config.h"
#include "util.h"

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

    // 输出所有三种路径
    print_multi_paths(paths);
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
        print_cache_statistics(cache);
        delete cache;
    }

    return 0;
}
