# 校园车联网仿真研究

## 目录 Table of Contents

- [项目简介 Project Introduction](#项目简介-project-introduction)
- [核心目录结构 Project Structure](#核心目录结构-project-structure)
  - [示例程序 Examples](#示例程序-examples)
  - [信任模型实现 Trust Model](#信任模型实现-trust-model)
- [核心模块说明 Core Modules](#核心模块说明-core-modules)
  - [信任模型算法 Trust Model Algorithm](#信任模型算法-trust-model-algorithm)
  - [恶意节点模拟 Malicious Node Simulation](#恶意节点模拟-malicious-node-simulation)
  - [示例场景配置 Scenario Configuration](#示例场景配置-scenario-configuration)
- [信任模型算法流程 Trust Algorithm Flow](#信任模型算法流程-trust-algorithm-flow)
- [依赖环境 Dependencies](#依赖环境-dependencies)
- [快速开始 Getting Started](#快速开始-getting-started)
- [技术特点 Features](#技术特点-features)
- [输出示例 Output Example](#输出示例-output-example)
- [项目声明 Project Statement](#项目声明-project-statement)
- [参考资源 References](#参考资源-references)
- [许可证 License](#许可证-license)

---

## 项目简介 Project Introduction

本项目是基于Veins框架的车联网**信任模型仿真研究**，专注于车联网环境下的信任管理机制研究。结合OMNeT++网络仿真器和SUMO交通仿真器，实现车辆间信任评估的建模与分析。

研究内容包括：
- **直接信任计算**：车辆基于直接交互反馈计算信任值
- **间接信任计算**：通过邻居车辆推荐计算信任值
- **综合信任评估**：结合直接信任和间接信任的最终信任值
- **恶意节点检测**：模拟恶意车辆发送虚假消息，通过信任模型进行识别
- **反馈评价机制**：实现正常车辆与恶意车辆的不同评价策略

---

## 核心目录结构 Project Structure

### 示例程序 Examples

```
examples/veins/
├── omnetpp.ini              # OMNeT++仿真配置文件
├── run                      # 仿真启动脚本
├── erlangen.*               # 德国Erlangen城市地图场景
├── downtown.*               # 市中心地图场景
├── JNU.*                    # 暨南大学(JNU)校园地图场景
└── results/                 # 仿真结果输出目录
```

### 信任模型实现 Trust Model

```
src/veins/modules/application/traci/
├── MyVeinsApp.cc            # 信任模型主实现
├── MyVeinsApp.h             # 信任模型数据结构定义
├── MyVeinsApp.ned           # 信任模型应用模块定义
├── TraCIDemo11p.*           # TraCI演示程序
├── TraCIDemoRSU11p.*        # 路侧单元(RSU)应用
└── TraCIDemo11pMessage.*    # 消息定义文件
```

---

## 核心模块说明 Core Modules

### 信任模型算法 Trust Model Algorithm

| 函数/模块 | 功能说明 |
|---------|---------|
| `initializeMaliciousList()` | 初始化恶意车辆列表 |
| `evaluateMessage()` | 消息评价逻辑（区分正常/恶意车辆） |
| `dtCalc.compute_round_dt()` | 直接信任(DT)计算 |
| `itCalc.compute_it()` | 间接信任(IT)计算 |
| `CT = f(DT, IT)` | 综合信任计算 |
| `FT` | 最终信任值（多轮平均） |

### 恶意节点模拟 Malicious Node Simulation

| 特性 | 说明 |
|------|------|
| 虚假消息 | 恶意车辆以一定概率发送虚假消息 |
| 反向评价 | 恶意车辆对正常消息给出差评，对虚假消息给出好评 |
| 可配置比例 | 支持设置恶意车辆占比 |

### 示例场景配置 Scenario Configuration

| 文件 | 说明 |
|------|------|
| `omnetpp.ini` | OMNeT++仿真主配置文件 |
| `run` | 仿真启动脚本 |
| `JNU.*` | 暨南大学校园地图场景（核心研究场景） |
| `erlangen.*` | Erlangen城市场景对比 |
| `results/` | 仿真结果与信任值输出 |

---

## 信任模型算法流程 Trust Algorithm Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    信任模型架构                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐      ┌──────────────┐                     │
│  │ 车辆交互    │ ───▶ │ 反馈收集     │                     │
│  │ (消息收发)  │      │ (评分记录)   │                     │
│  └─────────────┘      └──────┬───────┘                     │
│                              │                              │
│                              ▼                              │
│  ┌─────────────────────────────────────────────┐           │
│  │         直接信任计算 (DT)                    │           │
│  │  基于历史交互反馈 + 当前轮反馈               │           │
│  └──────────────────┬──────────────────────────┘           │
│                     │                                        │
│  ┌──────────────────┴──────────────────────────┐           │
│  │         间接信任计算 (IT)                    │           │
│  │  基于邻居推荐 (至少3个共同评价目标)          │           │
│  └──────────────────┬──────────────────────────┘           │
│                     │                                        │
│                     ▼                                        │
│  ┌─────────────────────────────────────────────┐           │
│  │      综合信任 CT = f(DT, IT)                │           │
│  └──────────────────┬──────────────────────────┘           │
│                     │                                        │
│                     ▼                                        │
│  ┌─────────────────────────────────────────────┐           │
│  │      最终信任 FT = avg(CT) 多轮平均         │           │
│  └─────────────────────────────────────────────┘           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 依赖环境 Dependencies

- **OMNeT++** 5.x 或更高版本
- **SUMO** 1.x 或更高版本
- **C++11** 编译器

---

## 快速开始 Getting Started

```bash
# 1. 配置环境
./configure

# 2. 编译项目
make

# 3. 运行仿真示例
cd examples/veins
./run
```

---

## 技术特点 Features

- **多维度信任评估**：结合直接信任与间接信任
- **恶意节点检测**：有效识别发送虚假消息的恶意车辆
- **动态信任更新**：支持多轮信任值的迭代更新
- **校园场景定制**：基于暨南大学校园地图的仿真环境
- **可扩展架构**：模块化设计，便于添加新的信任算法

---

## 输出示例 Output Example

```
[Round 1] Direct & Indirect Trust Calculation...
Vehicle 0 -> 1 DT=0.8500 IT=0.7200 CT=0.7850
Vehicle 0 -> 2 DT=0.9200 IT=0.8800 CT=0.9000
...
[FT] Vehicle 0 FinalTrust=0.8425 (based on 4 records)
[FT] Vehicle 1 FinalTrust=0.6500 (based on 3 records)
```

---

## 项目声明 Project Statement

本项目的作者及单位：
The author and affiliation of this project:

```
项目名称（Project Name）：校园车联网仿真研究
项目作者（Author）：徐彦丰
作者单位（Affiliation）：暨南大学网络空间安全学院（College of Cyber Security, Jinan University）
```

---

## 参考资源 References

- Veins官方网站: http://veins.car2x.org/
- OMNeT++: https://omnetpp.org/
- SUMO: https://www.eclipse.org/sumo/

---

## 许可证 License

本项目遵循 GPL-2.0-or-later 许可证。详见 [COPYING](COPYING) 文件。
