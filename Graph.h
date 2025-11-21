#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Edge.h"
#include "config.h"

// 路径结果结构体（包含路径和指标）
struct PathResult
{
    std::vector<std::string> path;  // 路径节点列表
    double time;                     // 总时间（秒）
    double distance;                 // 总距离（米）

    PathResult() : time(0), distance(0) {}
};

class Graph
{
public:
    Graph();

    // 从CSV文件加载地图数据来构建图，返回true表示成功，false表示失败
    bool from_csv(const std::string &filename);

    // 查找最短路径（实现Dijkstra算法），返回PathResult包含路径和代价
    // mode: 权重模式（TIME/DISTANCE/BALANCED）
    PathResult find_shortest_path(const std::string &start, const std::string &end, WeightMode mode = WeightMode::TIME);

    // 计算给定路径的总代价
    // path: 节点序列
    // mode: 权重模式（TIME/DISTANCE/BALANCED）
    // 返回: 路径的总代价，如果路径无效返回0
    double calculate_path_cost(const std::vector<std::string> &path, WeightMode mode);

private:
    // 权重范围结构体（用于归一化）
    struct WeightRange
    {
        double time_min;
        double time_max;
        double distance_min;
        double distance_max;
    };

    // 邻接表
    // key是地点的名字(string), value是所有从该地点出发的道路(vector of Edges)
    std::unordered_map<std::string, std::vector<Edge>> adj_list;

    // 计算图中所有边的权重范围（用于归一化）
    WeightRange calculate_weight_range() const;
};

#endif 
