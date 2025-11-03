#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Edge.h"

class Graph
{
public:
    Graph();

    // 从CSV文件加载地图数据来构建图，返回true表示成功，false表示失败
    bool from_csv(const std::string &filename);

    // 查找最短路径（实现Dijkstra算法），返回一个包含路径上所有地点的vector
    std::vector<std::string> find_shortest_path(const std::string &start, const std::string &end);

private:
    // 邻接表
    // key是地点的名字(string), value是所有从该地点出发的道路(vector of Edges)
    std::unordered_map<std::string, std::vector<Edge>> adj_list;
};

#endif 
