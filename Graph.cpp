#include "Graph.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <queue>
#include <vector>
#include <algorithm>

Graph::Graph()
{
}

// 从CSV文件加载地图数据来构建图
bool Graph::from_csv(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }

    // 清空旧的邻接表，以便加载新地图
    adj_list.clear();

    std::string line;

    // 读取并解析表头
    if (!std::getline(file, line))
    {
        std::cerr << "Error: Could not read header line from " << filename << std::endl;
        return false;
    }

    std::stringstream header_ss(line);
    std::string field;
    std::vector<std::string> headers;
    while (std::getline(header_ss, field, ','))
    {
        // 移除可能的回车符
        if (!field.empty() && field.back() == '\r')
        {
            field.pop_back();
        }
        headers.push_back(field);
    }

    // 动态确定列索引
    int start_node_idx = -1, end_node_idx = -1, direction_idx = -1,
        length_idx = -1, speed_limit_idx = -1, lanes_idx = -1, vehicles_idx = -1;

    for (int i = 0; i < headers.size(); ++i)
    {
        if (headers[i] == "起始地点") start_node_idx = i;
        else if (headers[i] == "目标地点") end_node_idx = i;
        else if (headers[i] == "道路方向") direction_idx = i;
        else if (headers[i] == "道路长度(米)") length_idx = i;
        else if (headers[i] == "道路限速(km/h)") speed_limit_idx = i;
        else if (headers[i] == "车道数") lanes_idx = i;
        else if (headers[i] == "现有车辆数") vehicles_idx = i;
    }

    // 检查是否所有必需的列都已找到
    if (start_node_idx == -1 || end_node_idx == -1 || direction_idx == -1 ||
        length_idx == -1 || speed_limit_idx == -1 || lanes_idx == -1 || vehicles_idx == -1)
    {
        std::cerr << "Error: CSV file " << filename << " is missing one or more required columns." << std::endl;
        return false;
    }

    // 逐行读取文件内容
    int line_number = 1; // 表头已读取
    while (std::getline(file, line))
    {
        line_number++;
        std::stringstream ss(line);
        std::vector<std::string> fields;

        while (std::getline(ss, field, ','))
        {
            // 移除可能的回车符
            if (!field.empty() && field.back() == '\r')
            {
                field.pop_back();
            }
            fields.push_back(field);
        }

        if (fields.size() < headers.size())
        {
            // 跳过格式不正确的行
            continue;
        }

        std::string start_node = fields[start_node_idx];
        std::string end_node = fields[end_node_idx];
        std::string direction = fields[direction_idx];

        try
        {
            double length = std::stod(fields[length_idx]);
            double speed_limit = std::stod(fields[speed_limit_idx]);
            int lanes = std::stoi(fields[lanes_idx]);
            int current_vehicles = std::stoi(fields[vehicles_idx]);

            // 使用Edge的构造函数创建边
            adj_list[start_node].emplace_back(end_node, length, speed_limit, lanes, current_vehicles);

            // 如果是双向路，则添加反向的边
            if (direction == "双向")
            {
                adj_list[end_node].emplace_back(start_node, length, speed_limit, lanes, current_vehicles);
            }
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Warning: Invalid data format at line " << line_number << " in " << filename << ", skipping this line." << std::endl;
            continue;
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "Warning: Data out of range at line " << line_number << " in " << filename << ", skipping this line." << std::endl;
            continue;
        }
    }

    file.close();

    // 检查是否成功加载了边
    if (adj_list.empty())
    {
        std::cerr << "Warning: No valid edges loaded from " << filename << ". The graph is empty." << std::endl;
    }

    return true;
}

// 计算图中所有边的权重范围（用于归一化）
WeightRange Graph::calculate_weight_range() const
{
    WeightRange range;
    range.time_min = std::numeric_limits<double>::infinity();
    range.time_max = 0.0;
    range.distance_min = std::numeric_limits<double>::infinity();
    range.distance_max = 0.0;

    // 扫描所有边
    for (const auto &pair : adj_list)
    {
        for (const Edge &edge : pair.second)
        {
            // 更新时间范围
            double time = edge.weight;  // BPR计算的时间
            if (time < range.time_min)
                range.time_min = time;
            if (time > range.time_max)
                range.time_max = time;

            // 更新距离范围
            double distance = edge.length;
            if (distance < range.distance_min)
                range.distance_min = distance;
            if (distance > range.distance_max)
                range.distance_max = distance;
        }
    }

    return range;
}

// 根据权重模式计算边的权重（支持归一化）
double Graph::calculate_edge_weight(const Edge &edge, WeightMode mode, const WeightRange &range) const
{
    switch (mode)
    {
    case WeightMode::TIME:
        // 时间最短：使用BPR计算的时间
        return edge.weight;

    case WeightMode::DISTANCE:
        // 距离最短：使用道路长度
        return edge.length;

    case WeightMode::BALANCED:
    {
        // 综合推荐：归一化时间和距离的加权平均
        double time = edge.weight;
        double distance = edge.length;

        // 归一化到[0, 1]区间
        double normalized_time = 0.0;
        double normalized_distance = 0.0;

        // 避免除零错误
        if (range.time_max > range.time_min)
        {
            normalized_time = (time - range.time_min) / (range.time_max - range.time_min);
        }

        if (range.distance_max > range.distance_min)
        {
            normalized_distance = (distance - range.distance_min) / (range.distance_max - range.distance_min);
        }

        // 加权平均
        return PathWeightConfig::time_factor * normalized_time +
               PathWeightConfig::distance_factor * normalized_distance;
    }

    default:
        return edge.weight;
    }
}

// 查找最短路径
std::vector<std::string> Graph::find_shortest_path(const std::string &start, const std::string &end, WeightMode mode)
{
    // 检查起点和终点是否存在于图中
    if (adj_list.find(start) == adj_list.end())
    {
        std::cerr << "Error: Start node '" << start << "' not found in graph." << std::endl;
        return {};
    }

    if (adj_list.find(end) == adj_list.end())
    {
        std::cerr << "Error: End node '" << end << "' not found in graph." << std::endl;
        return {};
    }

    // 计算权重范围（用于归一化）
    WeightRange range;
    if (mode == WeightMode::BALANCED)
    {
        range = calculate_weight_range();
    }

    // 定义优先队列的元素类型: <距离, 节点名>
    using QElement = std::pair<double, std::string>;
    std::priority_queue<QElement, std::vector<QElement>, std::greater<QElement>> pq;

    // 从起点到图中每个节点的最短距离
    std::unordered_map<std::string, double> distances;

    // 最短路径树中每个节点的前驱节点，用于最后回溯路径
    std::unordered_map<std::string, std::string> predecessors;

    // 初始化
    for (const auto &pair : adj_list)
    {
        distances[pair.first] = std::numeric_limits<double>::infinity();
    }

    // 起点到自身的距离为0
    distances[start] = 0;
    pq.push({0.0, start});

    // Dijkstra
    while (!pq.empty())
    {
        double current_dist = pq.top().first;
        std::string current_node = pq.top().second;
        pq.pop();

        // 如果当前节点就是终点，则路径已找到，可以提前退出循环
        if (current_node == end)
        {
            break;
        }

        // 如果队列中取出的距离比已知的最短距离要长，说明是旧的、已作废的记录，跳过
        if (current_dist > distances[current_node])
        {
            continue;
        }

        // 遍历当前节点的所有邻居（"松弛"操作）
        if (adj_list.count(current_node))
        {
            for (const Edge &edge : adj_list.at(current_node))
            {
                std::string neighbor = edge.destination;
                // 根据模式计算边的权重
                double edge_weight = calculate_edge_weight(edge, mode, range);
                double new_dist = current_dist + edge_weight;

                if (new_dist < distances[neighbor])
                {
                    // 更新最短距离和前驱节点
                    distances[neighbor] = new_dist;
                    predecessors[neighbor] = current_node;

                    // 将更新后的邻居放入优先队列
                    pq.push({new_dist, neighbor});
                }
            }
        }
    }

    // 路径回溯
    std::vector<std::string> path;
    std::string current = end;

    // 如果终点的距离仍然是无穷大，或者终点没有前驱（且非起点），说明不可达
    if (distances[end] == std::numeric_limits<double>::infinity())
    {
        return {}; // 返回空路径
    }

    while (predecessors.count(current))
    {
        path.push_back(current);
        current = predecessors[current];
    }

    // 最后加入起点
    path.push_back(start);

    // 翻转路径
    std::reverse(path.begin(), path.end());

    // 根据模式输出不同的信息
    switch (mode)
    {
    case WeightMode::TIME:
        std::cout << "Path found! Total estimated time: " << distances[end] << " seconds." << std::endl;
        break;
    case WeightMode::DISTANCE:
        std::cout << "Path found! Total distance: " << distances[end] << " meters." << std::endl;
        break;
    case WeightMode::BALANCED:
        std::cout << "Path found! Balanced score: " << distances[end] << std::endl;
        break;
    }

    return path;
}
