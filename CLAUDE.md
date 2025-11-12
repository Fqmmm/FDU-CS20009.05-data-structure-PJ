# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个城市路网最短路径查找应用，是复旦大学数据结构课程的项目。系统使用带有BPR（Bureau of Public Roads）拥堵建模的Dijkstra算法，在时变交通条件下寻找最优路径。

## 编译和运行命令

**编译：**
```bash
g++ -std=c++17 main.cpp Graph.cpp Edge.cpp config.cpp -o pathfinder.exe
```

**运行：**
```bash
.\pathfinder.exe --test_path <测试用例目录路径>
```

**示例：**
```bash
.\pathfinder.exe --test_path Test_Cases\test_cases\shanghai_test_cases\case1_simple
```

## 架构概览

### 核心组件

**1. Graph类（Graph.h/cpp）**
- 使用邻接表表示：`unordered_map<string, vector<Edge>>`
- 节点名称为中文地名（UTF-8编码）
- `from_csv()`：从CSV文件加载路网，支持动态表头解析
- `find_shortest_path()`：实现优先队列优化的Dijkstra算法

**2. Edge类（Edge.h/cpp）**
- 表示道路，包含属性：destination（目标节点）、length（长度）、speed_limit（限速）、lanes（车道数）、current_vehicles（当前车辆数）
- `calculate_weight()`：使用BPR拥堵函数计算通行时间
- BPR公式：T = T₀ × [1 + α × (V/C)^β]，其中T₀为自由流时间，V/C为流量容量比

**3. BPRConfig（config.h/cpp）**
- 拥堵建模的全局参数：
  - alpha = 0.15（拥堵敏感系数）
  - beta = 4.0（拥堵指数）
  - lane_capacity = 1800.0（每车道每小时通行能力，单位：辆）

**4. main.cpp**
- 命令行参数解析
- 测试用例文件发现（查找demand.txt和map_*.csv文件）
- Windows控制台UTF-8编码设置（`chcp 65001`）
- 按顺序处理多个时变地图

### 数据流

1. 用户提供包含以下文件的测试用例目录：
   - `demand*.txt`：包含"起点："和"终点："
   - `map_*.csv`：不同时间点的路网数据

2. 对每个地图文件：
   - Graph加载CSV，动态解析列
   - Dijkstra使用BPR权重边计算最短路径
   - 输出路径格式："A --> B --> C"

### 关键实现细节

**CSV解析：**
- 动态表头检测，支持不同的列顺序
- 处理"双向"道路，自动添加反向边
- 鲁棒的错误处理（try-catch捕获异常并警告）
- 移除字段末尾的'\r'字符

**路径查找：**
- 优先队列：`priority_queue<pair<double, string>, vector<...>, greater<...>>`
- 到达目标节点时提前终止
- 通过predecessors映射重建路径
- 无路径时返回空vector

**编码处理：**
- 所有中文文本使用UTF-8编码
- Windows控制台需要`chcp 65001`（在main()中设置）
- 文件路径和节点名称全程保持UTF-8

## 重要约束

- **非负权重**：BPR函数始终产生非负通行时间，确保Dijkstra算法的最优性
- **稀疏图**：道路网络是稀疏图，邻接表效率高
- **字符串节点**：节点名称是字符串（中文地名），不是整数ID
- **时变特性**：多个CSV文件代表不同时间点的网络状态，按顺序处理

## 测试用例位置

测试用例位于`Test_Cases/`，结构如下：
```
Test_Cases/
├── test_cases/shanghai_test_cases/
│   ├── case1_simple/
│   ├── case2_medium/
│   └── case3_complex/
└── large_scale_cases/
```

每个用例目录必须包含：
- 一个`.txt`文件，包含需求（起点/终点）
- 一个或多个`map_*.csv`文件（按文件名排序）

## 开发说明

- 使用C++17标准（`<filesystem>`需要）
- 在Windows 11上使用MinGW-w64 GCC 14.2.0编译
- 编辑器：Visual Studio Code
- 除C++标准库外无外部依赖
