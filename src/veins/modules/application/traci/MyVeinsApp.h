#pragma once

#include "veins/veins.h"

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"

#include <vector>
#include <tuple>
#include <map>
#include <cmath>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>

using namespace omnetpp;

namespace veins {

// ==== 信任评估 ====
struct Feedback {
    int senderId;   //评价者
    int receiverId; //被评价车辆
    char msgType; // 'S' or 'L'
    int score;    // 1=好评, 0=差评
    int round;
};

struct DirectTrust {
    double MPR_S;
    double MPR_L;
    double DT_S;
    double DT_L;
    double DT;
    double beta;
};
// ---- 直接信任计算器 ----
class DirectTrustCalculator {
public:
    double eta = 0.1;  // 时间衰减速率
    double zeta = 0.6;   // 短期权重


    std::map<std::pair<int,int>, std::vector<std::pair<int,double>>> mpr_hist_S;
    std::map<std::pair<int,int>, std::vector<std::pair<int,double>>> mpr_hist_L;

    double alpha(int t_prev, int k_now) { return std::exp(eta * (t_prev - k_now)); }
    double beta(double prev_CT) { return (prev_CT < 0 ? 0.0 : 1.0 - prev_CT); }
    double ratio(int pos, int neg) { return (pos+neg)>0 ? double(pos)/(pos+neg) : 1.0; }

    DirectTrust compute_round_dt(
        int vi, int vj, int k_now,
        const std::vector<Feedback>& feedbacks,
        double prev_CT = 0.8
    ) {
        int short_pos=0, short_neg=0, long_pos=0, long_neg=0;
        for (auto& f : feedbacks) {
            if (f.msgType == 'S') {
                if (f.score==1) short_pos++; else short_neg++;
            } else if (f.msgType == 'L') {
                if (f.score==1) long_pos++; else long_neg++;
            }
        }

        double MPR_S_k = ratio(short_pos, short_neg);
        double MPR_L_k = ratio(long_pos, long_neg);

        auto key = std::make_pair(vi, vj);
        mpr_hist_S[key].push_back({k_now, MPR_S_k});
        mpr_hist_L[key].push_back({k_now, MPR_L_k});

        double b = beta(prev_CT);

        // ---- 短期 ----
        double num_S_hist=0, den_S_hist=0;
        for (size_t i=0;i+1<mpr_hist_S[key].size();i++) {
            double a = alpha(mpr_hist_S[key][i].first, k_now);
            num_S_hist += mpr_hist_S[key][i].second * a;
            den_S_hist += a;
        }
        double num_S = num_S_hist + MPR_S_k * (1 - b);
        double den_S = den_S_hist + 1;
        double DT_S = den_S>0 ? num_S/den_S : MPR_S_k;

        // ---- 长期 ----
        double sum_L_hist = 0;
        for (size_t i=0;i+1<mpr_hist_L[key].size();i++)
            sum_L_hist += mpr_hist_L[key][i].second;
        double num_L = sum_L_hist + MPR_L_k * (1 - b);
        double N_m = std::max(1, k_now);
        double DT_L = num_L / N_m;

        double DT = zeta * DT_S + (1 - zeta) * DT_L;

        return {MPR_S_k, MPR_L_k, DT_S, DT_L, DT, b};
    }
};

/**
 * @brief 邻居车辆结构体，用于存储间接信任计算所需数据。
 * 每一个邻居 f 都包含：
 *   - vf：邻居车辆 ID
 *   - s_fj：邻居对目标车辆 vj 的评价次数
 *   - RT_fj：邻居对目标车辆的好评率（即直接信任）
 *   - objs：邻居与当前车辆 vi 共同评价过的对象集合（用于相似度计算）
 */


/**
 * @brief 间接信任计算器
 * 基于 DS 理论计算 IT_{i,j}：即车辆 vi 通过邻居 f 对目标 vj 的信任。
 *
 * 核心公式：
 *    IT_{i,j} = (Σ w_f * r_if * (b_f + λ * u_f)) / (Σ w_f * r_if)
 * 其中：
 *    - w_f = s_fj / Σ s_fj               邻居权重
 *    - r_if                              当前车与邻居车的相似度
 *    - b_f = w_f * RT_fj                 邻居对目标车的信任信号
 *    - u_f = 1 - w_f * RT_fj             不确定度
 *    - λ                                 不确定度调节系数
 */
class IndirectTrustCalculator {
public:
    struct EvalCount {
        int vj;       // 被评价车辆ID
        int count;    // 评价次数
    };
    struct EvalStat {
        int targetId;   // myTargets中的目标车辆ID
        int count;      // 邻居对该目标的评价次数
    };

    struct NeighborData {
        int vf;                         // 邻居车辆ID
        std::vector<EvalStat> s_fj;     // 邻居对myTargets中每个目标的统计
        std::vector<int> objs;          // vi和vf共同评价过的对象集合
    };

    double lambda_val = 0.5; // 不确定度调节系数，可根据场景调整

    /**
     * @brief 计算车辆 vi 对目标车辆 vj 的间接信任 IT_{i,j}
     *
     * @param vi         当前车辆 ID（评价者）
     * @param vj         目标车辆 ID（被评价者）
     * @param neighbor_data  当前车辆的邻居数据（每个邻居包含 s_fj、RT_fj、objs）
     * @param dt_map     存放所有车辆之间的直接信任分（用于相似度计算）
     * @return double    返回 IT_{i,j}（范围 0~1）
     */
    std::tuple<double, double,double> compute_it(
        int vi, int vj,
        const std::vector<NeighborData>& neighbor_data,
        const std::map<std::pair<int,int>, double>& dt_map,
        const std::vector<EvalCount>& eval_count
    ) {
        if (neighbor_data.empty()) return {get_dt(dt_map, vi, vj),0.0,get_dt(dt_map, vi, vj)};

        // === 1️⃣ 统计所有邻居对 vj 的评价次数总和 ===
        double total_sj = 0.0;
        std::map<int, int> neighbor_sfj; // vf -> s_fj（该邻居对vj的评价次数）

        for (const auto& nd : neighbor_data) {
            int s_fj = 0;
            // 查找该邻居是否对 vj 有统计
            for (const auto& stat : nd.s_fj) {
                if (stat.targetId == vj) {
                    s_fj = stat.count;
                    break;
                }
            }
            if (s_fj > 0) {
                neighbor_sfj[nd.vf] = s_fj;
                total_sj += s_fj;
            }
        }

        if (total_sj <= 0) return {get_dt(dt_map, vi, vj),0.0,get_dt(dt_map, vi, vj)}; // 无邻居对 vj 评价

        // === 2️⃣ 遍历每个邻居，计算贡献 ===
        double numerator = 0.0;
        double denominator = 0.0;

        for (const auto& nd : neighbor_data) {
            // 获取该邻居对 vj 的评价次数
            auto it = neighbor_sfj.find(nd.vf);
            if (it == neighbor_sfj.end()) continue; // 没有评价 vj 的邻居跳过
            double s_fj = it->second;

            double w_f = s_fj / total_sj; // 权重占比

            // ---- (1) 计算相似度 r_{i,f} ----
            double r_if = 1.0;
            if (!nd.objs.empty()) {
                double diff_sum = 0.0;
                for (int x : nd.objs) {
                    double dt_ix = get_dt(dt_map, vi, x);
                    double dt_fx = get_dt(dt_map, nd.vf, x);
                    diff_sum += std::abs(dt_ix - dt_fx);
                }
                r_if = std::max(0.0, 1.0 - (diff_sum / nd.objs.size()));
            }

            // ---- (2) 估计邻居 vf 对 vj 的信任度 ----
            // 若有直接信任，则取其DT作为RT_fj近似
            double RT_fj = get_dt(dt_map, nd.vf, vj);

            // ---- (3) 三元质量分模型 ----
            double b_f = w_f * RT_fj;       // 信任部分
            double u_f = 1.0 - w_f * RT_fj; // 不确定部分

            numerator += w_f * r_if * (b_f + lambda_val * u_f);
            denominator += w_f * r_if;

        }

        // === 3️⃣ 返回最终间接信任 ===
        double IT = (denominator > 0) ? numerator / denominator : 0.0;
        double count = 0.0;
        // === 4️⃣ 计算综合信任 CT ===

        for (const auto& ec : eval_count) {
            if (ec.vj == vj) {
                count = ec.count;
                break; // 找到就退出循环
            }
        }
        double DT = get_dt(dt_map, vi, vj);
        double theta = count/(total_sj + count);
        double CT = theta * DT + (1 - theta) * IT;

        return {DT,IT,CT};
    }

private:
    /**
     * @brief 从直接信任映射表 dt_map 中安全取出指定车辆对的信任值
     * @param dt_map 存放 (vi, vj) -> DT值 的映射
     * @param vi 车辆 i
     * @param vj 车辆 j
     * @return double 信任值，如果不存在则返回默认 0.5（中立）
     */
    double get_dt(const std::map<std::pair<int,int>, double>& dt_map, int vi, int vj) {
        auto it = dt_map.find({vi, vj});
        if (it != dt_map.end()) return it->second;
        else return 0.5; // 默认中立信任
    }
};



class VEINS_API MyVeinsApp : public DemoBaseApplLayer {
public:
    void initialize(int stage) override;
    void finish() override;

protected:
    void onBSM(DemoSafetyMessage* bsm) override;
    void onWSM(BaseFrame1609_4* wsm) override;
    void onWSA(DemoServiceAdvertisment* wsa) override;

    void handleSelfMsg(cMessage* msg) override;
    void handlePositionUpdate(cObject* obj) override;

    void sendShortMessage();
    void sendLongMessage();
    void onSM(ShortMessage* sm) override;
    void onLM(LongMessage* lm) override;

    static void initializeMaliciousList(int totalVehicles, double ratio, int seed);
    void evaluateMessage(int senderId, int msgId, bool isFake, bool senderMalicious,char msgType);
protected:
    simtime_t lastShortMsgTime;
    simtime_t lastLongMsgTime;
    cMessage* shortMsgTimer;
    cMessage* longMsgTimer;
    int shortMsgCount = 0;   // 短消息发送计数
    int longMsgCount = 0;    // 长消息发送计数

    int shortMsgRecvCount = 0;  // 接收到的短消息数量
    int longMsgRecvCount = 0;   // 接收到的长消息数量
    //malicious vehicles setting
    static std::vector<int> maliciousVehicles;  // 全局恶意车列表
    static bool initializedMalicious;           // 是否已初始化
    bool isMalicious = false;  // 当前车辆是否恶意
    double maliciousRatio = 0.2; // 恶意车辆比例（0..1）
    int maliciousSeed = 20251111;   // 全局 seed（可在 ini 中改）
    int totalVehicles = 50;
    static std::set<int> activeVehiclesThisRound;//活跃车辆数量
    double fake_msg_Prob = 0.5;        // 每次发送伪造的概率
    double time_round = 20;     //每一轮的时间间隔
    struct Rating {
        int senderId;
        int msgId;
        int score; // 1=好评，0=差评
        char msgType; //'s'=short,'l'=long
    };

    struct TrustRecord {
        int vi;       // 评价者
        int vj;       // 被评价者
        double DT;    // 直接信任
        double IT;    // 间接信任
        double CT;    // 综合信任
    };
    static std::vector<TrustRecord> trust_list;
    static std::vector<TrustRecord> trust_list_pre;  // 上一轮的信任数据备份
    std::vector<Rating> rate_list; // 每车的评价列表


    std::vector<Feedback> feedback_list;  // 每辆车独立的反馈表
    static std::vector<Feedback> allFeedback; // 保存上一轮反馈
    static std::map<std::pair<int,int>, double> globalDT; // 全局直接信任表
    int currentRound = 0;
    cMessage* trustTimer = nullptr;
    DirectTrustCalculator dtCalc;   //直接信任计算
    IndirectTrustCalculator itCalc; // 间接信任计算器

    // 全局：上一轮固定快照 / 本轮实时收集
    static std::vector<Feedback> gPrevRoundFeedback;
    static std::vector<Feedback> gCurrRoundFeedback;

    // 每轮结束的“栅栏”：统计有多少车完成了本轮计算
    static int gFinishedThisRound;
};


} // namespace veins
