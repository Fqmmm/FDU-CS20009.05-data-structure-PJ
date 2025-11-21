#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Edge.h"
#include "config.h"

// 权重范围结构体（用于归一化）
struct WeightRange
{
    double time_min;
    double time_max;
    double distance_min;
    double distance_max;
};

class Graph
{
public:
    Graph();

    // 从CSV文件加载地图数据来构建图，返回true表示成功，false表示失败
    bool from_csv(const std::string &filename);

    // 查找最短路径（实现Dijkstra算法），返回一个包含路径上所有地点的vector
    // mode: 权重模式（TIME/DISTANCE/BALANCED）
    std::vector<std::string> find_shortest_path(const std::string &start, const std::string &end, WeightMode mode = WeightMode::TIME);

    // 计算图中所有边的权重范围（用于归一化）
    WeightRange calculate_weight_range() const;

private:
    // 邻接表
    // key是地点的名字(string), value是所有从该地点出发的道路(vector of Edges)
    std::unordered_map<std::string, std::vector<Edge>> adj_list;

    // 根据权重模式计算边的权重（支持归一化）
    double calculate_edge_weight(const Edge &edge, WeightMode mode, const WeightRange &range) const;
};

#endif 
