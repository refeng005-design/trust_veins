# Veins - 车联网仿真框架

## 项目声明

- **项目名称**：Veins车联网仿真框架
- **项目作者**：徐彦丰
- **作者单位**：暨南大学网络空间安全学院

## 项目简介

Veins (Vehicles in Network Simulation) 是一个开源的车联网仿真框架，用于模拟车辆间通信(V2V)和车辆与基础设施间通信(V2I)。

本项目基于 [Veins官方项目](https://github.com/sommer/veins) 进行学习和研究，结合OMNeT++网络仿真器和SUMO交通仿真器，实现车联网场景的仿真建模。

## 核心目录结构

```
veins/
├── examples/veins/              # 仿真示例场景
│   ├── omnetpp.ini              # OMNeT++仿真配置文件
│   ├── run                      # 仿真启动脚本
│   ├── erlangen.*               # 德国Erlangen城市地图场景
│   ├── downtown.*               # 市中心地图场景
│   ├── JNU.*                    # 暨南大学(JNU)地图场景
│   └── results/                 # 仿真结果输出目录
│
└── src/veins/modules/application/traci/   # TraCI应用核心模块
    ├── MyVeinsApp.*             # 基础车辆应用类（.cc/.h/.ned）
    ├── TraCIDemo11p.*           # TraCI演示程序（车辆与SUMO交互）
    ├── TraCIDemoRSU11p.*        # 路侧单元(RSU)应用示例
    ├── TraCIDemoTrafficLightApp.*  # 交通灯控制应用
    └── TraCIDemo11pMessage.*    # 消息定义文件（.msg/.cc/.h）
```

## 核心模块说明

### 1. 示例程序 (`examples/veins/`)
| 文件 | 说明 |
|------|------|
| `omnetpp.ini` | OMNeT++仿真主配置文件，定义网络参数 |
| `run` | 仿真启动脚本 |
| `erlangen.*` | Erlangen城市场景地图配置 |
| `JNU.*` | 暨南大学自定义地图场景 |
| `downtown.*` | 市中心场景配置 |

### 2. TraCI应用模块 (`src/veins/modules/application/traci/`)
| 文件 | 说明 |
|------|------|
| `MyVeinsApp.cc/.h/.ned` | 基础车辆应用，实现车辆基本通信功能 |
| `TraCIDemo11p.cc/.h/.ned` | IEEE 802.11p标准演示，车辆自组网通信 |
| `TraCIDemoRSU11p.cc/.h/.ned` | 路侧单元(RSU)实现 |
| `TraCIDemoTrafficLightApp.cc/.h/.ned` | 交通灯控制应用 |
| `TraCIDemo11pMessage.msg/.cc/.h` | 消息格式定义 |

## 依赖环境

- **OMNeT++** 5.x 或更高版本
- **SUMO** 1.x 或更高版本
- **C++11** 编译器

## 快速开始

```bash
# 1. 配置环境
./configure

# 2. 编译项目
make

# 3. 运行仿真示例
cd examples/veins
./run
```

## 技术特点

- **V2V通信**：车辆间直接通信，支持IEEE 802.11p标准
- **V2I通信**：车辆与路侧单元(RSU)通信
- **真实交通流**：通过SUMO获取真实的车辆移动轨迹
- **可扩展架构**：模块化设计，便于添加新应用

## 许可证

本项目遵循 GPL-2.0-or-later 许可证。详见 [COPYING](COPYING) 文件。

## 参考资源

- Veins官方网站: http://veins.car2x.org/
- OMNeT++: https://omnetpp.org/
- SUMO: https://www.eclipse.org/sumo/

## 作者信息

徐彦丰
暨南大学网络空间安全学院
