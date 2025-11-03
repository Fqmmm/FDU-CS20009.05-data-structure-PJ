#ifndef EDGE_H
#define EDGE_H

#include <string>
#include <limits>

class Edge
{
public:
    std::string destination;    // 这条边指向的目标顶点
    double length;              // 道路长度（米）
    double speed_limit;         // 道路限速（km/h）
    int lanes;                  // 车道数
    int current_vehicles;       // 当前车辆数
    double weight;              // 边的权重（通行时间）

    // 构造函数
    Edge(const std::string& dest, double len, double spd_limit, int num_lanes, int vehicles);

    // 计算并返回边的权重（使用 BPR 函数）
    double calculate_weight() const;
};

#endif // EDGE_H
