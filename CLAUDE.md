# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个城市路网最短路径查找应用，是复旦大学数据结构课程的项目。系统使用带有BPR（Bureau of Public Roads）拥堵建模的Dijkstra算法，在时变交通条件下寻找最优路径。

**核心特性：**
- **多路径规划**：同时计算并输出三种优化策略的路径
  - 时间最短路径（Time-Optimized）：基于BPR拥堵模型
  - 距离最短路径（Distance-Optimized）：基于道路长度
  - 综合推荐路径（Balanced）：时间与距离的归一化加权平均
- **智能缓存系统**：持久化LRU缓存，避免重复计算
- **时变路网支持**：处理不同时间点的交通状况

## 编译和运行命令

**编译：**
```bash
g++ -std=c++17 main.cpp Graph.cpp Edge.cpp config.cpp Cache.cpp util.cpp -o pathfinder.exe
```

**基本运行：**
```bash
.\pathfinder.exe --test-path <测试用例目录路径>
```

**运行（禁用缓存）：**
```bash
.\pathfinder.exe --test-path <测试用例目录路径> --no-cache
```

**清空缓存：**
```bash
.\pathfinder.exe --clear-cache
```

**示例：**
```bash
# 正常运行（启用缓存）
.\pathfinder.exe --test-path Test_Cases\test_cases\shanghai_test_cases\case1_simple

# 强制重新计算（禁用缓存）
.\pathfinder.exe --test-path Test_Cases\test_cases\shanghai_test_cases\case1_simple --no-cache

# 清空所有缓存
.\pathfinder.exe --clear-cache
```

## 架构概览

### 核心组件

**1. PathResult结构体（Graph.h）**
- 路径查询结果的统一表示
- **成员变量**：
  - `path`（vector<string>）：路径节点列表
  - `time`（double）：总时间（秒）
  - `distance`（double）：总距离（米）
- **设计理念**：无论使用哪种权重模式（TIME/DISTANCE/BALANCED）进行路径查找，PathResult总是包含完整的时间和距离两个指标，方便用户全面对比不同路径的优劣

**2. Graph类（Graph.h/cpp）**
- 使用邻接表表示：`unordered_map<string, vector<Edge>>`
- 节点名称为中文地名（UTF-8编码）
- `from_csv()`：从CSV文件加载路网，支持动态表头解析
  - **三遍处理**：第一遍计算所有边的time字段，第二遍计算归一化范围，第三遍计算balanced_score
  - 预计算Edge的time和balanced_score字段，避免运行时重复计算
- `find_shortest_path(WeightMode)`：实现优先队列优化的Dijkstra算法，返回PathResult
  - `WeightMode::TIME`：时间最短（使用预计算的通行时间）
  - `WeightMode::DISTANCE`：距离最短（道路长度）
  - `WeightMode::BALANCED`：综合推荐（使用预计算的归一化加权平均）
  - 返回`PathResult`结构体，**自动计算并填充time和distance两个字段**
  - 优化：利用calculate_path_cost()直接计算两个指标，无需调用方额外计算
- `calculate_path_cost()`：计算给定路径在指定权重模式下的总代价
- **私有成员**：
  - `WeightRange`（嵌套结构体）：存储时间和距离的min/max范围，用于归一化
  - `calculate_weight_range()`：计算所有边的时间和距离范围

**3. Edge类（Edge.h/cpp）**
- **设计理念**：纯数据容器，不包含计算逻辑（计算逻辑分离到util模块）
- **成员变量**：
  - 基本属性：destination（目标节点）、length（长度）、speed_limit（限速）、lanes（车道数）、current_vehicles（当前车辆数）
  - 预计算字段：
    - `time`：通行时间（秒）= 自由流时间 × 拥堵系数，在Graph::from_csv()中计算
    - `balanced_score`：综合评分 = α × normalized_time + (1-α) × normalized_distance，在Graph::from_csv()中计算
- `get_weight(WeightMode)`：根据权重模式返回对应的预计算权重值
  - TIME模式返回time
  - DISTANCE模式返回length
  - BALANCED模式返回balanced_score

**4. Config（config.h/cpp）**
- **BPRConfig**：拥堵建模的全局参数
  - alpha = 0.15（拥堵敏感系数）
  - beta = 4.0（拥堵指数）
  - lane_capacity = 1800.0（每车道每小时通行能力，单位：辆）
- **PathWeightConfig**：综合路径权重配置
  - time_factor = 0.6（时间的权重因子α）
  - distance_factor = 0.4（距离的权重因子1-α）
  - 综合权重 = α × normalized_time + (1-α) × normalized_distance
- **CacheConfig**：缓存系统配置参数
  - max_size = 50（LRU缓存最大条目数）
  - cache_dir = ".cache"（缓存目录路径）

**5. PathCache类（Cache.h/cpp）**
- 实现持久化LRU（Least Recently Used）缓存机制
- **核心功能**：
  - `get()`：查询缓存，返回已保存的MultiPath结果（包含三种路径及其指标）
  - `put()`：保存新计算的MultiPath到缓存
  - `clear()`：清空所有缓存
- **MultiPath结构**：
  - `time_path`（PathResult）：时间最短路径，包含path、time、distance
  - `distance_path`（PathResult）：距离最短路径，包含path、time、distance
  - `balanced_path`（PathResult）：综合推荐路径，包含path、time、distance
  - **设计理念**：使用三个PathResult组合，消除数据重复，结构更清晰
  - 每种路径都存储完整的时间和距离指标，方便用户全面对比
  - 三种路径及其指标一次查询全部计算，一起缓存
  - **空路径缓存**：即使路径不存在（三个路径都为空），也会被缓存，避免重复计算
- **文件签名机制（FileSignature）**：
  - 使用文件修改时间 + 文件大小检测文件变化
  - **路径规范化**：使用`std::filesystem::canonical()`将相对路径和绝对路径统一为规范形式
  - 轻量级、几乎零性能开销
  - 文件修改后自动失效对应缓存
- **缓存键设计**：组合起点、终点、CSV文件签名生成唯一哈希键
- **缓存命中判断**：通过`hit_count`变化判断（而不是检查路径是否为空），确保空路径结果也能正确识别为缓存命中
- **LRU淘汰策略**：超过max_size时自动删除最久未使用的条目
- **持久化存储**：
  - `.cache/cache_index.txt`：缓存索引文件（LRU顺序和元数据）
  - `.cache/paths/*.cache`：具体的路径缓存文件

**6. util.h/cpp（工具函数模块）**
- **BPR拥堵函数**：
  - `calculate_bpr_congestion_factor()`：计算拥堵系数 = 1 + α × (V/C)^β
  - `calculate_free_flow_time()`：计算自由流通行时间 = 距离 / 速度
  - `calculate_travel_time()`：计算实际通行时间 = 自由流时间 × 拥堵系数
  - BPR公式：T = T₀ × [1 + α × (V/C)^β]，其中T₀为自由流时间，V/C为流量容量比
- **字符串工具**：
  - `trim()`：移除字符串首尾空白字符
- **文件操作工具**：
  - `find_test_files()`：在测试用例目录中查找demand.txt和map_*.csv文件
  - `read_demand()`：从demand文件读取起点和终点
- **输出工具**：
  - `print_usage()`：打印命令行使用说明
  - `print_single_path()`：打印单条路径（带装饰边框）
  - `print_multi_paths()`：打印所有三种路径及其时间和距离指标
  - `print_cache_statistics()`：打印缓存统计信息

**7. main.cpp**
- 程序入口和主流程控制
- 命令行参数解析（--test-path, --no-cache, --clear-cache）
- Windows控制台UTF-8编码设置（`chcp 65001`）
- 缓存对象创建和生命周期管理
- 调用`process_map()`处理每个地图文件
- **核心函数**：
  - `process_map()`：处理单个地图文件，执行缓存查询/路径计算/结果输出
  - **路径计算简化**：直接调用`find_shortest_path()`三次，返回的PathResult已包含完整的时间和距离信息
    - 旧逻辑：需要6次`calculate_path_cost()`调用来计算指标
    - 新逻辑：`find_shortest_path()`自动填充time和distance，无需额外计算
    - 代码更简洁，性能相同（因为find_shortest_path内部调用了calculate_path_cost）

### 数据流

1. 用户提供包含以下文件的测试用例目录：
   - `demand*.txt`：包含"起点："和"终点："
   - `map_*.csv`：不同时间点的路网数据

2. 对每个地图文件：
   - **缓存查询**（如果启用）：
     - 生成缓存键：hash(起点 + 终点 + CSV文件签名)
     - 检查缓存中是否存在该查询结果
     - 如果命中且文件未修改，直接返回缓存的MultiPath（包含三种路径）
   - **路径计算**（缓存未命中或禁用缓存）：
     - Graph加载CSV，动态解析列
     - 分别运行三次Dijkstra算法，使用不同的WeightMode：
       - `WeightMode::TIME`：计算时间最短路径，返回PathResult（自动填充time和distance）
       - `WeightMode::DISTANCE`：计算距离最短路径，返回PathResult（自动填充time和distance）
       - `WeightMode::BALANCED`：计算综合推荐路径，返回PathResult（自动填充time和distance）
     - 三个PathResult组合成MultiPath
     - 将MultiPath结果保存到缓存（如果启用）
   - **输出三种路径**：
     - 时间最短路径（Time-Optimized Path）
     - 距离最短路径（Distance-Optimized Path）
     - 综合推荐路径（Balanced Path）
     - 格式："A --> B --> C"
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
- **权重获取**：调用`edge.get_weight(mode)`获取预计算的权重值
- **多种权重模式**：
  - TIME模式：使用edge.time（预计算的BPR通行时间）
  - DISTANCE模式：使用edge.length（道路长度）
  - BALANCED模式：使用edge.balanced_score（预计算的归一化加权平均）
- **三遍预计算**（在Graph::from_csv()中）：
  - 第一遍：计算所有edge.time = 自由流时间 × 拥堵系数
  - 第二遍：计算所有边的时间和距离范围（min/max），用于归一化
  - 第三遍：计算所有edge.balanced_score = α × normalized_time + (1-α) × normalized_distance
  - 默认权重：α=0.6（时间），1-α=0.4（距离）

**编码处理：**
- 所有中文文本使用UTF-8编码
- Windows控制台需要`chcp 65001`（在main()中设置）
- 文件路径和节点名称全程保持UTF-8

**缓存机制：**
- **LRU算法**：使用`std::list`维护访问顺序，`std::unordered_map`快速查找
- **持久化**：程序退出后缓存保留，下次运行继续使用
- **文件签名**：
  - 结构：`{path, modification_time, file_size}`
  - **路径规范化**：使用`std::filesystem::canonical()`将所有路径（绝对/相对）转换为规范的绝对路径，确保指向同一文件的不同路径表示生成相同的缓存键
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
- **缓存文件格式**（纯文本，存储三种路径及指标）：
  ```
  # TIME
  time: 1048.95
  distance: 19710
  节点1
  节点2
  节点3
  # DISTANCE
  time: 1050.20
  distance: 19500
  节点1
  节点2
  节点3
  # BALANCED
  time: 1049.50
  distance: 19600
  节点1
  节点2
  节点3
  ```
  - 使用section markers（# TIME, # DISTANCE, # BALANCED）分隔三种路径
  - 每个section后紧跟两个指标值：time和distance
  - 然后是节点名称，每行一个节点
  - **空路径**：如果某种路径不存在，对应section下的time和distance值为0，没有节点行

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

4. **空路径结果缓存**
   - **重要特性**：即使查询结果为"无路径"（起点和终点不连通），也会被缓存
   - **原因**：计算"无路径"仍需运行完整的Dijkstra算法，性能开销相同
   - **效果**：避免对不连通节点对重复计算
   - **实现**：通过`hit_count`变化判断缓存命中，而不是检查路径是否为空

5. **灵活的命令行控制**
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
3. **CSV文件签名**相同（规范化路径、修改时间、文件大小）

**重要**：文件路径会自动规范化，因此以下情况都会正确命中缓存：
- 第一次使用绝对路径：`D:/Jimmy/_data_structure/PJ/Test_Cases/...`
- 第二次使用相对路径：`Test_Cases/...`
- 只要指向同一文件，路径会被规范化为相同的绝对路径

### 性能提升

- **缓存命中时**：< 1ms（仅读取小文本文件，一次获取三种路径）
- **缓存未命中时**：正常Dijkstra计算时间 × 3（需要计算三种路径）
- **适用场景**：
  - 开发调试阶段反复运行相同测试用例
  - 批量测试中有重复的(起点,终点,地图)组合
  - CI/CD流程中的回归测试
  - 时变路网分析（同一查询在不同时间点的地图上）

### 配置参数

在`config.cpp`中修改：

```cpp
// 修改缓存最大条目数
size_t CacheConfig::max_size = 100;  // 默认50

// 修改缓存目录
std::string CacheConfig::cache_dir = ".mycache";  // 默认".cache"

// 修改综合路径的权重因子
double PathWeightConfig::time_factor = 0.7;      // 默认0.6（时间权重）
double PathWeightConfig::distance_factor = 0.3;  // 默认0.4（距离权重）
// 注意：time_factor + distance_factor 应该等于 1.0
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
- **路径规范化**：系统自动将绝对路径和相对路径规范化，确保指向同一文件的不同路径表示可以正确命中缓存
- **多路径缓存**：每次缓存存储三种路径（时间最短、距离最短、综合推荐），一次查询命中可获取全部三种路径结果
- **空路径缓存**：即使查询结果为"无路径"（起点终点不连通），也会被缓存，第二次查询相同的无路径情况会直接命中缓存，避免重复运行Dijkstra算法

## 多路径规划详解

### 功能概述

系统为每个查询同时计算三种优化策略的路径，满足不同的导航需求：

1. **时间最短路径（Time-Optimized）**
   - 优化目标：最小化总通行时间
   - 权重：使用BPR拥堵模型计算的实际通行时间
   - 适用场景：用户希望尽快到达目的地，可以接受较长的距离

2. **距离最短路径（Distance-Optimized）**
   - 优化目标：最小化总行驶距离
   - 权重：直接使用道路长度（米）
   - 适用场景：用户希望节省燃油、减少里程，可以接受较长的时间

3. **综合推荐路径（Balanced）**
   - 优化目标：平衡时间和距离
   - 权重：归一化后的加权平均
   - 适用场景：用户希望在时间和距离之间取得平衡

### 综合路径算法

综合推荐路径使用**归一化 + 加权平均**的方法：

**步骤1：计算权重范围**
- 扫描图中所有边
- 记录时间的最小值time_min和最大值time_max
- 记录距离的最小值distance_min和最大值distance_max

**步骤2：归一化到[0, 1]**
```
normalized_time = (time - time_min) / (time_max - time_min)
normalized_distance = (distance - distance_min) / (distance_max - distance_min)
```

**步骤3：加权平均**
```
balanced_weight = α × normalized_time + (1-α) × normalized_distance
```
其中α = 0.6（时间权重），1-α = 0.4（距离权重）

### 为什么使用归一化

时间（秒）和距离（米）是**不同量纲**的物理量，不能直接相加或比较。归一化的作用：
1. **消除量纲影响**：将两者映射到相同的[0, 1]无量纲区间
2. **数值稳定性**：避免某一指标因数值过大而主导结果
3. **权重可控性**：权重因子α的意义明确（时间占比）

### 输出格式

程序为每个地图查询输出三个路径及其指标，格式如下：

```
┌─ Time-Optimized Path (时间最短) ────────────────────
│ Path: 起点 --> 中转1 --> 中转2 --> 终点
│ Total Time: 1048.95 seconds
│ Total Distance: 19710 meters
└─────────────────────────────────────────────────────

┌─ Distance-Optimized Path (距离最短) ────────────────
│ Path: 起点 --> 中转A --> 中转B --> 终点
│ Total Time: 1050.20 seconds
│ Total Distance: 19500 meters
└─────────────────────────────────────────────────────

┌─ Balanced Path (综合推荐) ──────────────────────────
│ Path: 起点 --> 中转X --> 中转Y --> 终点
│ Total Time: 1049.50 seconds
│ Total Distance: 19600 meters
└─────────────────────────────────────────────────────
```

**设计原则**：
- **所有路径都显示时间和距离**：方便用户全面对比三种路径的优劣
- 时间最短路径：通常时间最少，但距离可能较长
- 距离最短路径：通常距离最短，但时间可能较长
- 综合推荐路径：在时间和距离之间取得平衡
- 即使三条路径完全相同，也会分别显示完整路径和指标
- 指标值紧随路径显示，而不是在计算时输出
