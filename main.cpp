#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include "Graph.h"

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
    std::cout << "Usage: .\\pathfinder --test_path <path_to_test_case_directory>" << std::endl;
    std::cout << "Example: .\\pathfinder --test_path Test_Cases/test_cases/shanghai_test_cases/case1_simple" << std::endl;
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

// 处理单个地图文件，查找并打印最短路径
void process_map(const std::string &map_file, const std::string &start_node, const std::string &end_node)
{
    std::cout << "\n========================================================" << std::endl;
    std::cout << "Processing map: " << map_file << std::endl;
    std::cout << "========================================================" << std::endl;

    Graph city_map;
    if (!city_map.from_csv(map_file))
    {
        std::cerr << "Error: Failed to load map file " << map_file << std::endl;
        return;
    }

    std::vector<std::string> path = city_map.find_shortest_path(start_node, end_node);

    print_path(path);

    std::cout << "\n\n";
}

int main(int argc, char *argv[])
{
// 设置控制台编码
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    if (argc != 3 || std::string(argv[1]) != "--test_path")
    {
        print_usage();
        return 1;
    }

    std::filesystem::path case_path(argv[2]);
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

    // 处理每个地图文件
    for (const auto &map_file : map_files)
    {
        process_map(map_file, start_node, end_node);
    }

    return 0;
}
