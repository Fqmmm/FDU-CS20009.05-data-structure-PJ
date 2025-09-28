import csv
import os
import random
from typing import List, Dict, Set

# -------------------------- 大规模场景核心配置 --------------------------
# 输出目录
OUTPUT_DIR = "large_scale_case_example/"
# 需求文件名
DEMAND_FILE = "demand.txt"
# 时间戳（细分时段）
TIMESTAMPS = ["0700", "0900", "1200", "1500", "1800", "2000"]
# 生成的地点总数（可调整）
TOTAL_LOCATIONS = 150 
# 地点类型配置（使用A-Z作为后缀）
LOCATION_TYPES = {
    "公园": [chr(ord('A') + i) for i in range(26)], 
    "酒店": [chr(ord('A') + i) for i in range(26)],
    "商场": [chr(ord('A') + i) for i in range(26)],
    "学校": [chr(ord('A') + i) for i in range(26)],
    "医院": [chr(ord('A') + i) for i in range(26)],
    "车站": [chr(ord('A') + i) for i in range(26)],
    "景点": [chr(ord('A') + i) for i in range(26)],
    "办公楼": [chr(ord('A') + i) for i in range(26)],
    "居民区": [chr(ord('A') + i) for i in range(26)],
    "工厂": [chr(ord('A') + i) for i in range(26)]
}
# 区域划分（模拟城市不同区域）
REGIONS = ["中心区", "东区", "西区", "南区", "北区", "高新区", "经开区", "旅游区", "郊区", "远郊区"]
# 道路配置
ROAD_CONFIG = {
    # 不同区域道路密度（中心区最高）
    "region_density": {
        "中心区": 0.8,    # 高
        "东区": 0.6,      # 中高
        "西区": 0.6,      # 中高
        "南区": 0.5,      # 中
        "北区": 0.5,      # 中
        "高新区": 0.7,     # 中高
        "经开区": 0.6,     # 中高
        "旅游区": 0.5,     # 中
        "郊区": 0.3,       # 低
        "远郊区": 0.2      # 最低
    },
    # 道路类型比例
    "road_types": {
        "高速": 0.05,      # 长距离，高限速
        "主干道": 0.2,     # 中长距离，中高限速
        "次干道": 0.35,    # 中等距离，中限速
        "支路": 0.4        # 短距离，低限速
    },
    # 道路类型属性
    "road_attributes": {
        "高速": {"speed": 100, "lanes": [4, 6], "length_range": (5000, 20000)},
        "主干道": {"speed": 60, "lanes": [3, 4], "length_range": (1000, 5000)},
        "次干道": {"speed": 40, "lanes": [2, 3], "length_range": (500, 2000)},
        "支路": {"speed": 30, "lanes": [1, 2], "length_range": (100, 1000)}
    }
}
# 交通流量配置（不同时段）
TRAFFIC_CONFIG = {
    "0700": {"base_flow": 25, "波动范围": 8},  # 早高峰
    "0900": {"base_flow": 15, "波动范围": 6},  # 早高峰后
    "1200": {"base_flow": 20, "波动范围": 7},  # 午间高峰
    "1500": {"base_flow": 12, "波动范围": 5},  # 午后平峰
    "1800": {"base_flow": 28, "波动范围": 9},  # 晚高峰
    "2000": {"base_flow": 18, "波动范围": 6}   # 晚间平峰
}
# 确保连通性的核心枢纽数量
CORE_HUBS_COUNT = 10


class Location:
    """地点类，包含名称、类型和所属区域"""
    def __init__(self, id: int, name: str, type: str, region: str):
        self.id = id
        self.name = name
        self.type = type
        self.region = region

    def __str__(self) -> str:
        return self.name


class Road:
    """道路类，包含连接的地点和道路属性"""
    def __init__(self, road_id: str, start: Location, end: Location, road_type: str, 
                 length: int, lanes: int, speed: int):
        self.road_id = road_id
        self.start = start
        self.end = end
        self.road_type = road_type
        self.length = length  # 米
        self.lanes = lanes
        self.speed = speed    # km/h


def generate_locations() -> List[Location]:
    """生成上百个多样化的自定义地点（使用A-Z后缀）"""
    locations = []
    location_id = 1
    type_list = list(LOCATION_TYPES.keys())
    region_list = REGIONS.copy()
    
    # 记录每种类型已使用的字母索引，避免短期内重复
    type_indexes = {loc_type: 0 for loc_type in type_list}
    
    while len(locations) < TOTAL_LOCATIONS:
        # 随机选择地点类型和区域
        loc_type = random.choice(type_list)
        region = random.choice(region_list)
        
        # 使用A-Z作为后缀，循环使用
        suffix = LOCATION_TYPES[loc_type][type_indexes[loc_type] % 26]
        type_indexes[loc_type] += 1
        
        # 生成具体名称（类型+字母后缀）
        name = f"{loc_type}{suffix}"
        
        # 创建地点对象
        locations.append(Location(location_id, name, loc_type, region))
        location_id += 1
    
    return locations


def select_core_hubs(locations: List[Location]) -> List[Location]:
    """选择核心枢纽地点，确保整个地图的连通性"""
    # 确保每个区域至少有一个核心枢纽
    region_hubs = {}
    for region in REGIONS:
        # 从该区域选择一个地点作为枢纽
        region_locations = [loc for loc in locations if loc.region == region]
        if region_locations:
            region_hubs[region] = random.choice(region_locations)
    
    # 如果核心枢纽数量不足，从中心区补充
    hubs = list(region_hubs.values())
    if len(hubs) < CORE_HUBS_COUNT:
        center_locations = [loc for loc in locations if loc.region == "中心区"]
        while len(hubs) < CORE_HUBS_COUNT and center_locations:
            hub_candidate = random.choice(center_locations)
            if hub_candidate not in hubs:
                hubs.append(hub_candidate)
                center_locations.remove(hub_candidate)
    
    return hubs


def add_road_between(start: Location, end: Location, road_type: str, 
                    roads: List[Road], road_set: Set[str], road_id: int) -> bool:
    """在两个地点之间添加一条道路（避免重复）"""
    # 确保道路唯一（无论方向）
    road_key1 = f"{start.id}-{end.id}"
    road_key2 = f"{end.id}-{start.id}"
    if road_key1 in road_set or road_key2 in road_set:
        return False
    
    # 获取道路属性
    attrs = ROAD_CONFIG["road_attributes"][road_type]
    length = random.randint(*attrs["length_range"])
    lanes = random.choice(attrs["lanes"])
    speed = attrs["speed"]
    
    # 创建道路对象
    roads.append(Road(
        road_id=f"R{road_id:04d}",
        start=start,
        end=end,
        road_type=road_type,
        length=length,
        lanes=lanes,
        speed=speed
    ))
    
    # 记录已添加的道路
    road_set.add(road_key1)
    return True


def generate_roads(locations: List[Location], core_hubs: List[Location]) -> List[Road]:
    """生成复杂的道路网络，确保所有地点连通"""
    roads = []
    road_id = 1
    road_set = set()  # 用于避免重复道路
    location_count = len(locations)
    
    # 连接核心枢纽，形成骨干网络
    print("生成核心枢纽连接...")
    for i in range(len(core_hubs)):
        for j in range(i + 1, len(core_hubs)):
            hub1 = core_hubs[i]
            hub2 = core_hubs[j]
            # 核心枢纽之间使用高速或主干道连接
            road_type = random.choices(["高速", "主干道"], weights=[0.3, 0.7])[0]
            add_road_between(hub1, hub2, road_type, roads, road_set, road_id)
            road_id += 1
    
    # 为每个区域生成内部道路
    print("生成区域内部道路...")
    for region in REGIONS:
        region_locations = [loc for loc in locations if loc.region == region]
        if not region_locations:
            continue
            
        # 区域内道路密度由配置决定
        density = ROAD_CONFIG["region_density"][region]
        max_roads = int(len(region_locations) * (len(region_locations) - 1) * density / 2)
        roads_added = 0
        
        # 生成区域内道路
        while roads_added < max_roads and roads_added < 100:  # 限制每个区域最多100条路
            loc1 = random.choice(region_locations)
            loc2 = random.choice(region_locations)
            if loc1 == loc2:
                continue
                
            # 区域内主要使用次干道和支路
            road_type = random.choices(["主干道", "次干道", "支路"], weights=[0.1, 0.4, 0.5])[0]
            if add_road_between(loc1, loc2, road_type, roads, road_set, road_id):
                road_id += 1
                roads_added += 1
    
    # 生成跨区域道路
    print("生成跨区域连接道路...")
    cross_region_roads = int(location_count * 0.8)  # 跨区域道路数量
    roads_added = 0
    
    while roads_added < cross_region_roads:
        loc1 = random.choice(locations)
        loc2 = random.choice(locations)
        if loc1 == loc2 or loc1.region == loc2.region:
            continue
            
        # 跨区域主要使用高速和主干道
        road_type = random.choices(["高速", "主干道", "次干道"], weights=[0.2, 0.5, 0.3])[0]
        if add_road_between(loc1, loc2, road_type, roads, road_set, road_id):
            road_id += 1
            roads_added += 1
    
    # 确保所有地点都有连接（无孤立节点）
    print("确保所有地点连通...")
    connected_locations = set()
    for road in roads:
        connected_locations.add(road.start.id)
        connected_locations.add(road.end.id)
    
    # 找出孤立节点并连接到核心枢纽
    isolated_locations = [loc for loc in locations if loc.id not in connected_locations]
    for loc in isolated_locations:
        # 随机选择一个核心枢纽进行连接
        hub = random.choice(core_hubs)
        # 使用次干道连接孤立节点和枢纽
        road_type = "次干道" if loc.region in ["中心区", "东区", "西区", "南区", "北区"] else "主干道"
        add_road_between(loc, hub, road_type, roads, road_set, road_id)
        road_id += 1
        print(f"连接孤立节点: {loc.name} -> {hub.name}")
    
    print(f"共生成 {len(roads)} 条道路")
    return roads


def generate_demand_file(start: Location, end: Location) -> None:
    """生成需求文件"""
    demand_path = os.path.join(OUTPUT_DIR, DEMAND_FILE)
    with open(demand_path, "w", encoding="utf-8") as f:
        f.write(f"起点：{start.name}\n")
        f.write(f"终点：{end.name}\n")
        f.write(f"地点总数：{TOTAL_LOCATIONS}\n")
        f.write(f"道路总数：根据生成结果动态计算\n")
    
    print(f"生成需求文件: {demand_path}")


def generate_map_csv(timestamp: str, roads: List[Road]) -> None:
    """生成指定时间戳的地图CSV文件"""
    csv_path = os.path.join(OUTPUT_DIR, f"map_{timestamp}.csv")
    traffic = TRAFFIC_CONFIG[timestamp]
    
    with open(csv_path, "w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "道路ID", "起始地点", "目标地点", "道路类型", "道路方向",
            "道路长度(米)", "道路限速(km/h)", "车道数", "现有车辆数"
        ])
        writer.writeheader()
        
        for road in roads:
            # 计算车辆数（根据道路类型和时段动态调整）
            lane_factor = road.lanes * 2  # 车道数越多，基础车辆数越多
            base_flow = traffic["base_flow"] + lane_factor
            vehicle_count = base_flow + random.randint(-traffic["波动范围"], traffic["波动范围"])
            vehicle_count = max(1, vehicle_count)  # 确保至少有1辆车
            
            # 道路方向（主干道和高速多为单向）
            if road.road_type in ["高速", "主干道"]:
                direction = "单向" if random.random() > 0.3 else "双向"  # 70%单向
            else:
                direction = "双向" if random.random() > 0.5 else "单向"  # 50%单向
            
            writer.writerow({
                "道路ID": road.road_id,
                "起始地点": road.start.name,
                "目标地点": road.end.name,
                "道路类型": road.road_type,
                "道路方向": direction,
                "道路长度(米)": road.length,
                "道路限速(km/h)": road.speed,
                "车道数": road.lanes,
                "现有车辆数": vehicle_count
            })
    
    print(f"生成地图文件: {csv_path}")


def main():
    """主函数：生成大规模复杂测试场景"""
    # 初始化输出目录
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)
        print(f"创建输出目录: {OUTPUT_DIR}")
    
    # 生成上百个地点（使用A-Z后缀）
    print(f"生成 {TOTAL_LOCATIONS} 个地点...")
    locations = generate_locations()
    print(f"生成的地点示例: {[str(loc) for loc in locations[:5]]}...")
    
    # 选择核心枢纽
    core_hubs = select_core_hubs(locations)
    print(f"选择了 {len(core_hubs)} 个核心枢纽")
    
    # 生成道路网络
    roads = generate_roads(locations, core_hubs)
    
    # 随机选择起点和终点（确保不同）
    start = random.choice(locations)
    end = random.choice(locations)
    while end == start:
        end = random.choice(locations)
    print(f"选择路径需求: {start.name} -> {end.name}")
    
    # 生成需求文件
    generate_demand_file(start, end)
    
    # 生成所有时间戳的地图文件
    for timestamp in TIMESTAMPS:
        generate_map_csv(timestamp, roads)
    
    print("\n所有测试文件生成完成!")
    print(f"输出目录: {os.path.abspath(OUTPUT_DIR)}")
    print(f"包含: 1个需求文件 + {len(TIMESTAMPS)}个时段地图文件")


if __name__ == "__main__":
    main()