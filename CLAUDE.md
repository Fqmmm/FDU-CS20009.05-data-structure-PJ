# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个城市路网最短路径查找应用，是复旦大学数据结构课程的项目。系统使用带有BPR（Bureau of Public Roads）拥堵建模的Dijkstra算法，在时变交通条件下寻找最优路径。

## 编译和运行命令

**编译：**
```bash
g++ -std=c++17 main.cpp Graph.cpp Edge.cpp config.cpp Cache.cpp -o pathfinder.exe
```

**基本运行：**
```bash
.\pathfinder.exe --test_path <测试用例目录路径>
```

**运行（禁用缓存）：**
```bash
.\pathfinder.exe --test_path <测试用例目录路径> --no_cache
```

**清空缓存：**
```bash
.\pathfinder.exe --clear-cache
```

**示例：**
```bash
# 正常运行（启用缓存）
.\pathfinder.exe --test_path Test_Cases\test_cases\shanghai_test_cases\case1_simple

# 强制重新计算（禁用缓存）
.\pathfinder.exe --test_path Test_Cases\test_cases\shanghai_test_cases\case1_simple --no_cache

# 清空所有缓存
.\pathfinder.exe --clear-cache
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

**3. Config（config.h/cpp）**
- **BPRConfig**：拥堵建模的全局参数
  - alpha = 0.15（拥堵敏感系数）
  - beta = 4.0（拥堵指数）
  - lane_capacity = 1800.0（每车道每小时通行能力，单位：辆）
- **CacheConfig**：缓存系统配置参数
  - max_size = 50（LRU缓存最大条目数）
  - cache_dir = ".cache"（缓存目录路径）

**4. PathCache类（Cache.h/cpp）**
- 实现持久化LRU（Least Recently Used）缓存机制
- **核心功能**：
  - `get()`：查询缓存，返回已保存的路径结果
  - `put()`：保存新计算的路径到缓存
  - `clear()`：清空所有缓存
- **文件签名机制**：
  - 使用文件修改时间 + 文件大小检测文件变化
  - 轻量级、几乎零性能开销
  - 文件修改后自动失效对应缓存
- **缓存键设计**：组合起点、终点、CSV文件签名生成唯一哈希键
- **LRU淘汰策略**：超过max_size时自动删除最久未使用的条目
- **持久化存储**：
  - `.cache/cache_index.txt`：缓存索引文件（LRU顺序和元数据）
  - `.cache/paths/*.cache`：具体的路径缓存文件

**5. main.cpp**
- 命令行参数解析（--test_path, --no_cache, --clear-cache）
- 测试用例文件发现（查找demand.txt和map_*.csv文件）
- Windows控制台UTF-8编码设置（`chcp 65001`）
- 缓存集成和统计信息输出
- 按顺序处理多个时变地图

### 数据流

1. 用户提供包含以下文件的测试用例目录：
   - `demand*.txt`：包含"起点："和"终点："
   - `map_*.csv`：不同时间点的路网数据

2. 对每个地图文件：
   - **缓存查询**（如果启用）：
     - 生成缓存键：hash(起点 + 终点 + CSV文件签名)
     - 检查缓存中是否存在该查询结果
     - 如果命中且文件未修改，直接返回缓存的路径
   - **路径计算**（缓存未命中或禁用缓存）：
     - Graph加载CSV，动态解析列
     - Dijkstra使用BPR权重边计算最短路径
     - 将结果保存到缓存（如果启用）
   - 输出路径格式："A --> B --> C"
   - 更新LRU顺序（如果使用缓存）

3. 输出缓存统计信息（Hits/Misses/Entries）

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

**缓存机制：**
- **LRU算法**：使用`std::list`维护访问顺序，`std::unordered_map`快速查找
- **持久化**：程序退出后缓存保留，下次运行继续使用
- **文件签名**：
  - 结构：`{path, modification_time, file_size}`
  - 检测开销：仅查询文件系统元数据，不读取文件内容
  - 自动失效：文件修改后签名不匹配，缓存自动失效
- **缓存键生成**：`std::hash<string>(start + "|" + end + "|" + csv_signature)`
- **索引文件格式**（文本格式，易于调试）：
  ```
  max_size: 50
  entry_count: N
  lru_order: key1,key2,key3,...
  entry: key|start|end|csv_path|mtime|size|cache_file|created_time
  ```
- **缓存文件格式**（纯文本，每行一个节点名）：
  ```
  节点1
  节点2
  节点3
  ```

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

## 缓存功能详解

### 功能概述

项目实现了完整的**持久化LRU缓存系统**，用于存储最近计算的路径查询结果，避免重复计算，提高性能。

### 核心特性

1. **持久化存储**
   - 缓存保存在`.cache/`目录，程序退出后不会丢失
   - 适用于反复测试、批量运行等场景
   - 跨运行保持缓存状态

2. **LRU淘汰策略**
   - 最多保存N条缓存（默认50，可在config.cpp中修改）
   - 超过上限时自动删除最久未使用的条目
   - 每次访问缓存会更新其在LRU列表中的位置

3. **智能文件变化检测**
   - 使用文件修改时间 + 文件大小作为"轻量级签名"
   - 几乎零性能开销（不读取文件内容计算哈希）
   - CSV文件修改后自动失效对应缓存

4. **灵活的命令行控制**
   - 默认启用缓存
   - `--no_cache`：强制重新计算，不查询和保存缓存
   - `--clear-cache`：手动清空所有缓存

### 缓存目录结构

```
.cache/
├── cache_index.txt           # 索引文件：LRU顺序和元数据
└── paths/
    ├── {hash1}.cache         # 路径缓存文件1
    ├── {hash2}.cache         # 路径缓存文件2
    └── ...
```

### 缓存命中判断

查询结果会被缓存，当且仅当以下三个条件**完全相同**时缓存命中：
1. **起点**相同
2. **终点**相同
3. **CSV文件签名**相同（路径、修改时间、文件大小）

### 性能提升

- **缓存命中时**：< 1ms（仅读取小文本文件）
- **缓存未命中时**：正常Dijkstra计算时间
- **适用场景**：
  - 开发调试阶段反复运行相同测试用例
  - 批量测试中有重复的(起点,终点,地图)组合
  - CI/CD流程中的回归测试

### 配置参数

在`config.cpp`中修改：

```cpp
// 修改缓存最大条目数
size_t CacheConfig::max_size = 100;  // 默认50

// 修改缓存目录
std::string CacheConfig::cache_dir = ".mycache";  // 默认".cache"
```

修改后需要重新编译。

### 缓存统计

程序运行结束时会输出缓存统计信息：

```
========================================================
Cache Statistics:
  Hits: 3        # 缓存命中次数
  Misses: 2      # 缓存未命中次数
  Entries: 5     # 当前缓存中的条目总数
========================================================
```

### 注意事项

- 缓存文件使用UTF-8编码，与项目整体编码一致
- 清空缓存不会删除`.cache/`目录本身，只删除其中的文件
- 如果测试用例的CSV文件被修改，对应缓存会自动失效，无需手动清空
- 缓存键使用哈希值，不同查询产生冲突的概率极低（< 2^-64）
