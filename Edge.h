#ifndef EDGE_H
#define EDGE_H

#include <string>
#include <limits>
#include "config.h"

class Edge
{
public:
    std::string destination;    // 这条边指向的目标顶点
    double length;              // 道路长度（米）
    double speed_limit;         // 道路限速（km/h）
    int lanes;                  // 车道数
    int current_vehicles;       // 当前车辆数

    // 预计算的权重字段
    double time;                // 通行时间（秒）= 自由流时间 × 拥堵系数
    double balanced_score;      // 综合评分 = 归一化时间 × α + 归一化距离 × (1-α)

    // 构造函数
    Edge(const std::string& dest, double len, double spd_limit, int num_lanes, int vehicles);

    // 根据权重模式获取边的权重
    double get_weight(WeightMode mode) const;
};

#endif // EDGE_H
