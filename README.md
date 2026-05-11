# Veins - 车联网仿真框架

## 项目简介

Veins (Vehicles in Network Simulation) 是一个开源的车联网仿真框架，用于模拟车辆间通信(V2V)和车辆与基础设施间通信(V2I)。

本项目基于 [Veins官方项目](https://github.com/sommer/veins) 进行学习和研究。

## 核心模块

### 1. 示例程序 (`examples/veins/`)
- **MyVeinsApp**: 基础车辆应用示例
- **TraCIDemo11p**: TraCI演示程序，展示车辆与SUMO交通仿真器的交互
- **TraCIDemoRSU11p**: 路侧单元(RSU)示例
- **TraCIDemoTrafficLightApp**: 交通灯控制应用示例
- **仿真场景**: 包含downtown、erlangen和JNU三种地图场景

### 2. TraCI应用模块 (`src/veins/modules/application/traci/`)
- TraCI (Traffic Control Interface) 是与SUMO交通仿真器交互的核心接口
- 提供车辆移动性管理、交通控制等功能

## 依赖环境

- OMNeT++ 5.x 或更高版本
- SUMO 1.x 或更高版本
- C++11 编译器

## 编译运行

```bash
# 配置环境
./configure

# 编译
make

# 运行示例
cd examples/veins
./run
```

## 许可证

本项目遵循 GPL-2.0-or-later 许可证。详见 [COPYING](COPYING) 文件。

## 参考资源

- Veins官方网站: http://veins.car2x.org/
- OMNeT++: https://omnetpp.org/
- SUMO: https://www.eclipse.org/sumo/

## 作者说明

本项目用于学术研究和学习目的。
