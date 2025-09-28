import csv
import os
import re
from random import randint, choice
from typing import List, Dict

# 输出根文件夹（包含3个子Case文件夹）
ROOT_OUTPUT_DIR = "shanghai_test_cases/"
# 3组Case的配置
CASE_CONFIGS = {
    # Case1：简单（1个时间点）
    "case1_simple": {
        "demand_file": "demand_case1.txt",
        "timestamps": ["1200"],  # 仅午间平峰1个时间点
        "road_count_range": (12, 15),  # 道路数量：12-15条
        "has_traffic_light": False,
        "desc": "简单Case：1个时间点，无红绿灯，少道路，以短路径为主"
    },
    # Case2：中等（2个时间点）
    "case2_medium": {
        "demand_file": "demand_case2.txt",
        "timestamps": ["0800", "1400"],  # 早高峰+平峰2个时间点
        "road_count_range": (16, 18),  # 道路数量：16-18条
        "has_traffic_light": False,
        "desc": "中等Case：2个时间点，无红绿灯，中等道路数量"
    },
    # Case3：复杂（3个时间点）
    "case3_complex": {
        "demand_file": "demand_case3.txt",
        "timestamps": ["0800", "1400", "1830"],  # 早+平+晚高峰3个时间点
        "road_count_range": (20, 25),  # 道路数量：20-25条
        "has_traffic_light": False,
        "desc": "复杂Case：3个时间点，无红绿灯，多道路，含远郊长路径"
    }
}
# 上海17个指定景点（按地理区域分组）
SHANGHAI_SPOTS = {
    "Yangpu_Edu": ["复旦大学（江湾校区）", "复旦大学（邯郸校区）", "五角场"],
    "Puxi_Core": ["人民广场", "外滩", "豫园·城隍庙", "静安寺", "上海博物馆", 
                 "田子坊", "上海南京路步行街", "中山公园"],
    "Pudong_Core": ["东方明珠", "上海科技馆", "上海世纪公园", "上海迪士尼乐园"],
    "Pudong_Suburb": ["上海野生动物园"],
    "Other_Suburb": ["上海东林寺"]
}
# 景点间现实距离参考（单位：km）
SPOT_DISTANCE_REF = {
    # 杨浦高校区内部及周边
    ("复旦大学（江湾校区）", "复旦大学（邯郸校区）"): 5,
    ("复旦大学（邯郸校区）", "五角场"): 2,
    ("五角场", "中山公园"): 8,
    ("复旦大学（邯郸校区）", "人民广场"): 10,
    
    # 浦西核心区内部
    ("人民广场", "外滩"): 2,
    ("人民广场", "静安寺"): 3,
    ("人民广场", "上海博物馆"): 1,
    ("人民广场", "上海南京路步行街"): 1,
    ("外滩", "豫园·城隍庙"): 3,
    ("静安寺", "中山公园"): 4,
    ("田子坊", "豫园·城隍庙"): 2,
    ("田子坊", "外滩"): 3,
    ("上海南京路步行街", "外滩"): 1,
    
    # 浦西→浦东跨江
    ("外滩", "东方明珠"): 2,
    ("人民广场", "上海科技馆"): 6,
    ("人民广场", "上海世纪公园"): 7,
    ("五角场", "上海科技馆"): 12,
    
    # 浦东内部
    ("东方明珠", "上海科技馆"): 3,
    ("上海科技馆", "上海世纪公园"): 2,
    ("上海世纪公园", "上海迪士尼乐园"): 15,
    ("上海迪士尼乐园", "上海野生动物园"): 10,
    
    # 远郊连接
    ("人民广场", "上海东林寺"): 60,
    ("上海野生动物园", "上海东林寺"): 55,
    ("上海迪士尼乐园", "上海东林寺"): 50
}
# 不同时段路况配置
TRAFFIC_CONFIG = {
    "0800": {"base_car": 18, "light_delay": 1.6},  # 早高峰
    "1400": {"base_car": 6, "light_delay": 1.0},   # 平峰
    "1830": {"base_car": 20, "light_delay": 1.8},  # 晚高峰
    "1200": {"base_car": 8, "light_delay": 1.0}    # 午间平峰（Case1专用）
}
# 车道数与限速
LANE_OPTIONS = {
    "Core": [3, 4],
    "Suburb_Main": [2, 3],
    "Suburb_Minor": [2]
}
SPEED_LIMIT = {
    "Core": 60,
    "Suburb_Main": 80,
    "Suburb_Minor": 70
}


class RoadBaseData:
    """道路基础数据类（无红绿灯属性）"""
    def __init__(self, road_id: str, start: str, end: str, length: int, lanes: int, speed: int):
        self.road_id = road_id
        self.start = start
        self.end = end
        self.length = length  # 单位：米
        self.lanes = lanes
        self.speed = speed    # 单位：km/h


def init_root_dir() -> None:
    """初始化根输出文件夹及3个Case子文件夹"""
    if not os.path.exists(ROOT_OUTPUT_DIR):
        os.makedirs(ROOT_OUTPUT_DIR)
        print(f"已创建根输出文件夹：{ROOT_OUTPUT_DIR}")
    # 为每个Case创建子文件夹
    for case_name in CASE_CONFIGS.keys():
        case_dir = os.path.join(ROOT_OUTPUT_DIR, case_name)
        if not os.path.exists(case_dir):
            os.makedirs(case_dir)
            print(f"  - 已创建{case_name}子文件夹")


def get_spot_area(spot: str) -> str:
    """获取景点所属地理区域"""
    for area, spots in SHANGHAI_SPOTS.items():
        if spot in spots:
            return area
    return "Suburb_Minor"


def get_realistic_distance(start: str, end: str, case_name: str) -> int:
    """基于Case复杂度生成符合现实比例的道路长度"""
    # 优先使用预设真实距离
    if (start, end) in SPOT_DISTANCE_REF:
        km = SPOT_DISTANCE_REF[(start, end)]
    elif (end, start) in SPOT_DISTANCE_REF:
        km = SPOT_DISTANCE_REF[(end, start)]
    else:
        start_area = get_spot_area(start)
        end_area = get_spot_area(end)
        
        # 根据Case复杂度调整距离范围
        if case_name == "case1_simple":
            # 简单Case：仅短路径（3-10km），避免远郊
            km = randint(3, 10)
        elif case_name == "case3_complex":
            # 复杂Case：允许远郊长路径（45-60km）
            if (start_area == "Other_Suburb" or end_area == "Other_Suburb"):
                km = randint(45, 60)
            else:
                km = randint(3, 20)
        else:  # case2_medium
            # 中等Case：普通路径（3-15km）
            km = randint(3, 15)
    
    # 转换为米，添加±10%波动
    meter = km * 1000
    return int(meter * (0.9 + randint(0, 20) / 100))


def get_road_config(start_area: str, end_area: str, case_name: str) -> tuple:
    """基于Case复杂度和区域获取道路配置（车道数、限速）"""
    if start_area in ["Puxi_Core", "Yangpu_Edu", "Pudong_Core"] and end_area in ["Puxi_Core", "Yangpu_Edu", "Pudong_Core"]:
        # 核心区道路
        if case_name == "case1_simple":
            # 简单Case：3车道为主（80%）
            lanes = 3 if randint(0, 10) > 2 else 4
        elif case_name == "case3_complex":
            # 复杂Case：4车道比例更高（60%）
            lanes = 4 if randint(0, 10) > 4 else 3
        else:
            lanes = choice(LANE_OPTIONS["Core"])
        speed = SPEED_LIMIT["Core"]
    elif (start_area in ["Puxi_Core", "Yangpu_Edu", "Pudong_Core"]) and (end_area in ["Pudong_Suburb"]):
        # 郊区主干道
        lanes = choice(LANE_OPTIONS["Suburb_Main"])
        speed = SPEED_LIMIT["Suburb_Main"]
    else:
        # 远郊道路
        lanes = choice(LANE_OPTIONS["Suburb_Minor"])
        speed = SPEED_LIMIT["Suburb_Minor"]
    return lanes, speed


def generate_all_spots(case_name: str) -> List[str]:
    """基于Case复杂度筛选景点"""
    all_spots = []
    for area, spots in SHANGHAI_SPOTS.items():
        # 简单Case：排除远郊景点（上海东林寺）
        if case_name == "case1_simple" and area == "Other_Suburb":
            continue
        all_spots.extend(spots)
    return all_spots


def generate_demand_file(case_name: str, start: str, end: str) -> None:
    """生成对应Case的客户需求TXT"""
    case_config = CASE_CONFIGS[case_name]
    case_dir = os.path.join(ROOT_OUTPUT_DIR, case_name)
    demand_path = os.path.join(case_dir, case_config["demand_file"])
    with open(demand_path, "w", encoding="utf-8") as f:
        f.write(f"起点：{start}\n")
        f.write(f"终点：{end}\n")
        f.write(f"说明：{case_config['desc']}\n")
    print(f"✅ {case_name} - 生成需求文件：{os.path.basename(demand_path)}")


def generate_base_roads(case_name: str, spots: List[str]) -> List[RoadBaseData]:
    """基于Case复杂度生成道路基础数据"""
    case_config = CASE_CONFIGS[case_name]
    base_roads = []
    road_set = set()
    road_id = 1
    road_count = randint(*case_config["road_count_range"])
    has_long_road = False  # 标记复杂Case是否已包含远郊长路径

    # 生成目标数量的不重复道路
    while len(base_roads) < road_count:
        start = choice(spots)
        end = choice(spots)
        # 避免起点=终点和重复道路
        if start == end or f"{start}-{end}" in road_set:
            continue
        
        # 复杂Case：确保包含至少1条远郊长路径
        if case_name == "case3_complex" and not has_long_road:
            long_spot = "上海东林寺"
            if (start == long_spot or end == long_spot):
                has_long_road = True
        
        # 生成道路属性
        start_area = get_spot_area(start)
        end_area = get_spot_area(end)
        length = get_realistic_distance(start, end, case_name)
        lanes, speed = get_road_config(start_area, end_area, case_name)
        
        # 添加道路数据
        base_roads.append(RoadBaseData(
            road_id=f"SH{road_id:02d}",
            start=start,
            end=end,
            length=length,
            lanes=lanes,
            speed=speed
        ))
        road_set.add(f"{start}-{end}")
        road_id += 1

    # 确保所有景点都有连接（防止孤立）
    connected_spots = set()
    for road in base_roads:
        connected_spots.add(road.start)
        connected_spots.add(road.end)
    missing_spots = [spot for spot in spots if spot not in connected_spots]
    for spot in missing_spots:
        core_spot = choice(["人民广场", "外滩", "东方明珠", "五角场"])
        road_key = f"{spot}-{core_spot}"
        if road_key not in road_set:
            length = get_realistic_distance(spot, core_spot, case_name)
            start_area = get_spot_area(spot)
            end_area = get_spot_area(core_spot)
            lanes, speed = get_road_config(start_area, end_area, case_name)
            base_roads.append(RoadBaseData(
                road_id=f"SH{road_id:02d}",
                start=spot,
                end=core_spot,
                length=length,
                lanes=lanes,
                speed=speed
            ))
            road_set.add(road_key)
            road_id += 1

    print(f"✅ {case_name} - 生成{len(base_roads)}条道路")
    return base_roads


def generate_map_csv(case_name: str, timestamp: str, base_roads: List[RoadBaseData]) -> None:
    """生成对应Case、对应时间戳的地图CSV（无红绿灯）"""
    case_dir = os.path.join(ROOT_OUTPUT_DIR, case_name)
    csv_path = os.path.join(case_dir, f"map_{timestamp}.csv")
    traffic = TRAFFIC_CONFIG[timestamp]
    base_car = traffic["base_car"]

    # 写入CSV（移除红绿灯字段）
    with open(csv_path, "w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "道路ID", "起始地点", "目标地点", "道路方向", "道路长度(米)",
            "道路限速(km/h)", "车道数", "现有车辆数"
        ])
        writer.writeheader()

        for road in base_roads:
            # 计算现有车辆数
            lane_compensation = {2: 0, 3: 2, 4: 4}[road.lanes]
            car_count = base_car + randint(-4, 4) + lane_compensation
            car_count = max(1, min(car_count, road.lanes * 8))  # 边界控制

            # 确定道路方向（根据Case复杂度调整单向/双向比例）
            if case_name == "case1_simple":
                # 简单Case：双向为主（80%）
                road_direction = "双向" if randint(0, 10) > 2 else "单向"
            elif case_name == "case3_complex":
                # 复杂Case：单向为主（70%）
                road_direction = "单向" if randint(0, 10) > 3 else "双向"
            else:
                # 中等Case：均衡比例
                road_direction = "单向" if randint(0, 10) > 5 else "双向"

            # 写入道路数据
            writer.writerow({
                "道路ID": road.road_id,
                "起始地点": road.start,
                "目标地点": road.end,
                "道路方向": road_direction,
                "道路长度(米)": road.length,
                "道路限速(km/h)": road.speed,
                "车道数": road.lanes,
                "现有车辆数": car_count
            })

    print(f"✅ {case_name} - 生成时间戳[{timestamp}]的地图CSV")


def generate_case(case_name: str) -> None:
    """生成单个Case的完整测试样例"""
    print(f"\n===== 开始生成 {case_name} =====")
    
    # 获取当前Case的景点列表
    spots = generate_all_spots(case_name)
    print(f"📌 {case_name} - 包含景点数：{len(spots)}")
    
    # 随机选择起点和终点（确保不同且连通）
    start_spot = choice(spots)
    end_spot = choice(spots)
    while end_spot == start_spot:
        end_spot = choice(spots)
    print(f"📌 {case_name} - 需求：{start_spot} → {end_spot}")
    
    # 生成需求文件
    generate_demand_file(case_name, start_spot, end_spot)
    
    # 生成道路基础数据
    base_roads = generate_base_roads(case_name, spots)
    
    # 生成所有时间戳的地图CSV
    for timestamp in CASE_CONFIGS[case_name]["timestamps"]:
        generate_map_csv(case_name, timestamp, base_roads)
    
    print(f"===== {case_name} 生成完成 =====")


def main():
    """主函数：生成所有3组Case"""
    # 初始化文件夹
    init_root_dir()
    
    # 依次生成3组Case（从简单到复杂）
    for case_name in CASE_CONFIGS.keys():
        generate_case(case_name)
    
    # 生成完成提示
    print(f"\n🎉 所有测试样例生成完成！")
    print(f"📂 样例根目录：{os.path.abspath(ROOT_OUTPUT_DIR)}")
    print(f"📝 包含 {len(CASE_CONFIGS)} 组Case，每组均确保起点到终点连通")


if __name__ == "__main__":
    main()