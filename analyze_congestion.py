#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
拥堵系数分析工具
分析路网CSV文件中每条道路的拥堵情况

使用方法：
    python analyze_congestion.py <csv_file_path>

示例：
    python analyze_congestion.py Test_Cases/test_cases/shanghai_test_cases/case1_simple/map_1200.csv
"""

import csv
import sys
import os
from typing import List, Dict


# BPR函数参数（与C++代码保持一致）
ALPHA = 0.15
BETA = 4.0
LANE_CAPACITY = 1800.0  # 每车道每小时通行能力（辆）


def calculate_bpr_congestion_factor(current_vehicles: int, lanes: int,
                                    length_meters: float, speed_limit_kmh: float) -> float:
    """
    计算BPR拥堵系数：1 + α × (V/C)^β
    注意：current_vehicles是道路上的车辆数（occupancy），需要转换为流量（flow）

    参数：
        current_vehicles: 当前车辆数（occupancy）
        lanes: 车道数
        length_meters: 道路长度（米）
        speed_limit_kmh: 道路限速（km/h）

    返回：
        拥堵系数（>=1.0）
    """
    if lanes <= 0 or speed_limit_kmh <= 0:
        return float('inf')

    # 计算自由流通行时间（小时）
    speed_mps = speed_limit_kmh * 1000.0 / 3600.0
    travel_time_sec = length_meters / speed_mps
    travel_time_hour = travel_time_sec / 3600.0

    # 将occupancy转换为flow（veh/h）
    # flow = occupancy / travel_time
    volume = current_vehicles / travel_time_hour

    # 计算道路容量
    capacity = lanes * LANE_CAPACITY

    # 计算流量/容量比
    volume_capacity_ratio = volume / capacity

    # BPR公式
    congestion_factor = 1.0 + ALPHA * (volume_capacity_ratio ** BETA)

    return congestion_factor


def calculate_free_flow_time(length_meters: float, speed_limit_kmh: float) -> float:
    """
    计算自由流通行时间（秒）

    参数：
        length_meters: 道路长度（米）
        speed_limit_kmh: 道路限速（km/h）

    返回：
        自由流通行时间（秒）
    """
    if speed_limit_kmh <= 0:
        return float('inf')

    # 转换为 m/s
    speed_mps = speed_limit_kmh * 1000.0 / 3600.0

    return length_meters / speed_mps


def analyze_csv(csv_file: str) -> None:
    """
    分析CSV文件中的拥堵情况

    参数：
        csv_file: CSV文件路径
    """
    # 检查文件是否存在
    if not os.path.exists(csv_file):
        print(f"错误：文件不存在 - {csv_file}")
        sys.exit(1)

    # 读取CSV文件
    try:
        with open(csv_file, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)

            # 存储统计数据
            congestion_factors = []
            vc_ratios = []
            road_data = []

            # 逐行处理
            for row in reader:
                try:
                    # 提取数据
                    road_id = row.get('道路ID', 'N/A')
                    start = row.get('起始地点', 'N/A')
                    end = row.get('目标地点', 'N/A')
                    length = float(row['道路长度(米)'])
                    speed = float(row['道路限速(km/h)'])
                    lanes = int(row['车道数'])
                    vehicles = int(row['现有车辆数'])

                    # 计算拥堵系数
                    congestion = calculate_bpr_congestion_factor(vehicles, lanes, length, speed)

                    # 计算V/C比（使用正确的flow计算）
                    # 先计算自由流通行时间
                    speed_mps = speed * 1000.0 / 3600.0
                    travel_time_sec = length / speed_mps
                    travel_time_hour = travel_time_sec / 3600.0
                    # 将occupancy转换为flow
                    volume = vehicles / travel_time_hour
                    capacity = lanes * LANE_CAPACITY
                    vc_ratio = volume / capacity

                    # 计算自由流时间和实际时间
                    free_time = calculate_free_flow_time(length, speed)
                    actual_time = free_time * congestion

                    # 计算时间增加百分比
                    time_increase_pct = (congestion - 1.0) * 100

                    # 保存数据
                    congestion_factors.append(congestion)
                    vc_ratios.append(vc_ratio)
                    road_data.append({
                        'id': road_id,
                        'start': start,
                        'end': end,
                        'length': length,
                        'speed': speed,
                        'lanes': lanes,
                        'vehicles': vehicles,
                        'capacity': capacity,
                        'vc_ratio': vc_ratio,
                        'congestion': congestion,
                        'free_time': free_time,
                        'actual_time': actual_time,
                        'time_increase_pct': time_increase_pct
                    })

                except (KeyError, ValueError) as e:
                    print(f"警告：跳过无效行 - {e}")
                    continue

            # 检查是否有有效数据
            if not congestion_factors:
                print("错误：CSV文件中没有有效数据")
                sys.exit(1)

            # 计算统计量
            min_congestion = min(congestion_factors)
            max_congestion = max(congestion_factors)
            avg_congestion = sum(congestion_factors) / len(congestion_factors)

            min_vc = min(vc_ratios)
            max_vc = max(vc_ratios)
            avg_vc = sum(vc_ratios) / len(vc_ratios)

            # 打印统计信息
            print("=" * 80)
            print(f"文件：{csv_file}")
            print(f"道路总数：{len(road_data)}")
            print("=" * 80)

            print("\n【拥堵系数统计】")
            print(f"  最小值：{min_congestion:.6f}")
            print(f"  最大值：{max_congestion:.6f}")
            print(f"  平均值：{avg_congestion:.6f}")
            print(f"  变化范围：{(max_congestion - min_congestion) / min_congestion * 100:.2f}%")

            print("\n【流量/容量比 (V/C) 统计】")
            print(f"  最小值：{min_vc:.6f}")
            print(f"  最大值：{max_vc:.6f}")
            print(f"  平均值：{avg_vc:.6f}")

            print("\n【时间延迟统计】")
            print(f"  最小延迟：{(min_congestion - 1.0) * 100:.2f}%")
            print(f"  最大延迟：{(max_congestion - 1.0) * 100:.2f}%")
            print(f"  平均延迟：{(avg_congestion - 1.0) * 100:.2f}%")

            # 找出最拥堵和最畅通的道路
            most_congested = max(road_data, key=lambda x: x['congestion'])
            least_congested = min(road_data, key=lambda x: x['congestion'])

            print("\n【最拥堵道路】")
            print(f"  道路ID：{most_congested['id']}")
            print(f"  路段：{most_congested['start']} → {most_congested['end']}")
            print(f"  车道数：{most_congested['lanes']} 车道")
            print(f"  车辆数：{most_congested['vehicles']} 辆")
            print(f"  容量：{most_congested['capacity']:.0f} 辆/小时")
            print(f"  V/C比：{most_congested['vc_ratio']:.4f}")
            print(f"  拥堵系数：{most_congested['congestion']:.6f}")
            print(f"  时间延迟：+{most_congested['time_increase_pct']:.2f}%")
            print(f"  自由流时间：{most_congested['free_time']:.2f} 秒")
            print(f"  实际通行时间：{most_congested['actual_time']:.2f} 秒")

            print("\n【最畅通道路】")
            print(f"  道路ID：{least_congested['id']}")
            print(f"  路段：{least_congested['start']} → {least_congested['end']}")
            print(f"  车道数：{least_congested['lanes']} 车道")
            print(f"  车辆数：{least_congested['vehicles']} 辆")
            print(f"  容量：{least_congested['capacity']:.0f} 辆/小时")
            print(f"  V/C比：{least_congested['vc_ratio']:.4f}")
            print(f"  拥堵系数：{least_congested['congestion']:.6f}")
            print(f"  时间延迟：+{least_congested['time_increase_pct']:.2f}%")
            print(f"  自由流时间：{least_congested['free_time']:.2f} 秒")
            print(f"  实际通行时间：{least_congested['actual_time']:.2f} 秒")

            # 拥堵等级分布
            print("\n【拥堵等级分布】")
            level_1 = sum(1 for c in congestion_factors if c < 1.05)  # 畅通
            level_2 = sum(1 for c in congestion_factors if 1.05 <= c < 1.15)  # 缓慢
            level_3 = sum(1 for c in congestion_factors if 1.15 <= c < 1.30)  # 拥堵
            level_4 = sum(1 for c in congestion_factors if c >= 1.30)  # 严重拥堵

            total = len(congestion_factors)
            print(f"  畅通 (系数<1.05)：{level_1} 条 ({level_1/total*100:.1f}%)")
            print(f"  缓慢 (1.05-1.15)：{level_2} 条 ({level_2/total*100:.1f}%)")
            print(f"  拥堵 (1.15-1.30)：{level_3} 条 ({level_3/total*100:.1f}%)")
            print(f"  严重拥堵 (≥1.30)：{level_4} 条 ({level_4/total*100:.1f}%)")

            print("\n" + "=" * 80)

    except Exception as e:
        print(f"错误：无法读取文件 - {e}")
        sys.exit(1)


def main():
    """主函数"""
    # 检查命令行参数
    if len(sys.argv) != 2:
        print("使用方法：python analyze_congestion.py <csv_file_path>")
        print("\n示例：")
        print("  python analyze_congestion.py Test_Cases/test_cases/shanghai_test_cases/case1_simple/map_1200.csv")
        sys.exit(1)

    csv_file = sys.argv[1]

    # 分析CSV文件
    analyze_csv(csv_file)


if __name__ == "__main__":
    main()
