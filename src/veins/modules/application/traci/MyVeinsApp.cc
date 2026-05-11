

#include "veins/modules/application/traci/MyVeinsApp.h"
#include <algorithm>
#include <random>
#include <numeric>
using namespace veins;

Define_Module(veins::MyVeinsApp);
// malicious Vehicles settings
std::vector<int> veins::MyVeinsApp::maliciousVehicles;
bool veins::MyVeinsApp::initializedMalicious = false;

std::vector<veins::Feedback> veins::MyVeinsApp::allFeedback;
std::map<std::pair<int,int>, double> MyVeinsApp::globalDT;

std::vector<veins::Feedback> veins::MyVeinsApp::gPrevRoundFeedback;
std::vector<veins::Feedback> veins::MyVeinsApp::gCurrRoundFeedback;
int MyVeinsApp::gFinishedThisRound = 0;

std::vector<MyVeinsApp::TrustRecord> MyVeinsApp::trust_list;
std::vector<MyVeinsApp::TrustRecord> MyVeinsApp::trust_list_pre;

std::set<int> MyVeinsApp::activeVehiclesThisRound;
void MyVeinsApp::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
            lastShortMsgTime = simTime();
            lastLongMsgTime = simTime();
            // 创建两个自消息（定时器）
            shortMsgTimer = new cMessage("shortMsgTimer");
            longMsgTimer  = new cMessage("longMsgTimer");

            // 启动定时器：1 秒后触发短消息，5 秒后触发长消息
            double shortT = 1.0;
            double longT  = 5.0;
            scheduleAt(simTime() + uniform(0, shortT), shortMsgTimer);   // 随机起相
            scheduleAt(simTime() + uniform(0, longT),  longMsgTimer);    // 随机起相

/* 恶意节点部分*/
            // 初始化恶意车辆列表（只执行一次）
            initializeMaliciousList(totalVehicles, maliciousRatio, maliciousSeed);

            // 当前车辆的 external ID(SUMO id)
            std::string extId = mobility->getExternalId();
            int vid = std::stoi(extId);  // 例如 "12" -> 12
            // 判断是否恶意
            isMalicious = (std::find(maliciousVehicles.begin(),
                                     maliciousVehicles.end(),
                                     vid) != maliciousVehicles.end());

            std::cout << "Vehicle " << mobility->getExternalId() << " -> isMalicious=" << isMalicious << std::endl;
            // （可选）把恶意节点写到全局文件，或由 manager 模块收集
/* 恶意节点部分*/
            //trust evaluation
            currentRound = 1;
            trustTimer = new cMessage("TrustRoundTimer");
            scheduleAt(simTime() + time_round, trustTimer); // 每10秒触发一次

            std::cout << "[" << simTime() << "s] MyVeinsApp initialized." << std::endl;
        }
}

void MyVeinsApp::finish()
{
    cancelAndDelete(shortMsgTimer);
    cancelAndDelete(longMsgTimer);
    // 输出每辆车的发送与接收统计结果
    std::cout << "Vehicle [" << mobility->getExternalId()
              << "] sent ShortMessages: " << shortMsgCount
              << ", LongMessages: " << longMsgCount
              << " | received ShortMessages: " << shortMsgRecvCount
              << ", LongMessages: " << longMsgRecvCount
              << std::endl;
    DemoBaseApplLayer::finish();
    // statistics recording goes here

}

void MyVeinsApp::onBSM(DemoSafetyMessage* bsm)
{
    // Your application has received a beacon message from another car or RSU
    // code for handling the message goes here
}

void MyVeinsApp::onWSM(BaseFrame1609_4* wsm)
{
    // Your application has received a data message from another car or RSU
    // code for handling the message goes here, see TraciDemo11p.cc for examples

}

void MyVeinsApp::onWSA(DemoServiceAdvertisment* wsa)
{
    // Your application has received a service advertisement from another car or RSU
    // code for handling the message goes here, see TraciDemo11p.cc for examples
}

void MyVeinsApp::handleSelfMsg(cMessage* msg)
{
    double shortT = 1.0;
    double longT  = 5.0;
    if (msg == shortMsgTimer) {
            sendShortMessage();
            scheduleAt(simTime() + shortT + uniform(0, 0.2*shortT), shortMsgTimer); // 每次再加点轻微抖动
        }
        else if (msg == longMsgTimer) {
            sendLongMessage();
            scheduleAt(simTime() + longT + uniform(0, 0.2*longT),  longMsgTimer);
        }
        else if (msg == trustTimer){
            std::cout << "[Round " << currentRound << "] Direct & Indirect Trust Calculation...\n";

            int vi = stoi(mobility->getExternalId());

            // === 1️⃣ Direct Trust 计算 ===
            std::map<int, std::vector<Feedback>> grouped;
            // 直接信任分数应该使用当前轮车的反馈数据呀
            for (auto& fb : gCurrRoundFeedback) {  // 使用当前轮的反馈数据
                if (fb.senderId == vi) grouped[fb.receiverId].push_back(fb);
            }

            for (auto& [vj, feedbacks] : grouped) {
                // 这里还少上传了一个参数pre_CT
                double CT_prev = 0.6;   // 默认值（没找到时）

                for (const auto& rec : MyVeinsApp::trust_list_pre) {
                    if (rec.vi == vi && rec.vj == vj) {
                        CT_prev = rec.CT;
                        break;   // 找到立即退出
                    }
                }
                auto result = dtCalc.compute_round_dt(vi, vj, currentRound, feedbacks,CT_prev);
                globalDT[{vi, vj}] = result.DT; // 更新全局直接信任表

            }
            // === 2️⃣ 邻居选取：基于本轮反馈 ===
            // 还是用当前论的数据好一些
            std::map<int, std::set<int>> ratedTargets; // 记录每个车评价过的目标
            for (auto& fb : gCurrRoundFeedback)
                ratedTargets[fb.senderId].insert(fb.receiverId);

            std::set<int> myTargets = ratedTargets[vi];
            std::vector<IndirectTrustCalculator::NeighborData> neighbors;

            for (auto& [vf, targets] : ratedTargets) {
                if (vf == vi) continue;
                std::vector<int> commonObjs;
                for (int target : targets)
                    if (myTargets.count(target)) commonObjs.push_back(target);
                int overlap = commonObjs.size();
                if (overlap >= 3) { // 至少有3个共同目标
                    IndirectTrustCalculator::NeighborData nd;
                    nd.vf = vf;
                    nd.objs = commonObjs;

                    // === 统计 vf 对每个共同目标的评价次数 ===
                    for (int vj : commonObjs) {
                        int totalEval = 0;

                        for (const auto& fb : gCurrRoundFeedback) {
                            if (fb.senderId == vf && fb.receiverId == vj)
                                totalEval++;
                        }

                        // 若你只想存有评价的目标，可跳过0计数项
                        if (totalEval > 0) {
                            IndirectTrustCalculator::EvalStat stat;
                            stat.targetId = vj;
                            stat.count = totalEval;
                            nd.s_fj.push_back(stat);
                        }
                    }

                    neighbors.push_back(nd);
                }
            }


            std::cout << "Vehicle " << vi << " found " << neighbors.size() << " neighbors." << std::endl;
            if (!neighbors.empty()) {
                std::cout << "  Neighbor IDs: ";
                for (auto& nb : neighbors) {
                    std::cout << nb.vf << " ";
                }
                std::cout << std::endl;
            } else {
                std::cout << "  No neighbors found." << std::endl;
            }

            // === 统计 vi 对每个共同目标的评价次数 ===
            std::vector<IndirectTrustCalculator::EvalCount> evalCounts;
            for (int vj : myTargets) {
                int totalEval = 0;

                for (const auto& fb : gCurrRoundFeedback) {
                    if (fb.senderId == vi && fb.receiverId == vj)
                        totalEval++;
                }
                // 若你只想存有评价的目标，可跳过0计数项
                if (totalEval > 0) {
                    IndirectTrustCalculator::EvalCount ec;
                    ec.vj = vj;
                    ec.count = totalEval;
                    evalCounts.push_back(ec);
                }
            }

            // === 3️⃣ Indirect Trust 计算 ===
            for (auto& [vj, feedbacks] : grouped) {
                if (vj == vi) continue;
                std::tuple<double,double,double> result = itCalc.compute_it(vi, vj, neighbors, globalDT,evalCounts);
                double DT = std::get<0>(result);
                double IT = std::get<1>(result);
                double CT = std::get<2>(result);
                std::cout << "  Vehicle " << vi << " -> " << vj
                        << " DT=" << DT
                          << " IT=" << IT
                          << " CT=" << CT << std::endl;
                trust_list.push_back({vi, vj, DT, IT, CT});
            }

            // 每个车辆算完本轮后：
            MyVeinsApp::gFinishedThisRound++;
            currentRound++;
            // 当所有车辆都完成了本轮计算（用你设置的 totalVehicles）：
            if (MyVeinsApp::gFinishedThisRound >= MyVeinsApp::activeVehiclesThisRound.size()) {
                MyVeinsApp::gCurrRoundFeedback.clear();
                MyVeinsApp::globalDT.clear();

                // ===== 计算每辆车的最终信任 FT =====
                std::cout << "\n===== Final Trust (FT) Calculation =====" << std::endl;

                // 存储累计CT与计数
                std::map<int, double> sumCT;
                std::map<int, int> countCT;

                // 遍历trust_list，累计每个被评价者的CT
                for (const auto& record : MyVeinsApp::trust_list) {
                    sumCT[record.vj] += record.CT;
                    countCT[record.vj]++;
                }

                // 遍历所有车辆
                for (int vj = 0; vj < totalVehicles; ++vj) {
                    double FT = 0.0;
                    if (countCT.find(vj) != countCT.end()) {
                        FT = sumCT[vj] / countCT[vj];  // 平均CT
                    } else {
                        FT = 0.0;  // 没有被评价过的车辆
                    }

                    std::cout << "[" << simTime() << "s] " << "[FT] Vehicle " << vj
                              << " FinalTrust=" << std::fixed << std::setprecision(4)
                              << FT << " (based on " << countCT[vj] << " records)"
                              << std::endl;
                }
                std::vector<MyVeinsApp::TrustRecord> trust_list_pre = MyVeinsApp::trust_list;
                trust_list.clear();
                // 进下一轮
                //currentRound++;
                MyVeinsApp::gFinishedThisRound = 0;

                std::cout << "=== Round switch done. Now enter Round " << currentRound << " ===\n";
            }

            // 继续调度下一次
            feedback_list.clear(); // 清空本地列表
            scheduleAt(simTime() + time_round, trustTimer);



        }
        else {
            DemoBaseApplLayer::handleSelfMsg(msg);
        }

}

void MyVeinsApp::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);
    // the vehicle has moved. Code that reacts to new positions goes here.
    // member variables such as currentPosition and currentSpeed are updated in the parent class
    //这个好像没有了呀，在selfmesg里面已经处理过了呀
    if (simTime() - lastShortMsgTime > 1) {
        sendShortMessage();
        lastShortMsgTime = simTime();
    }
    if (simTime() - lastLongMsgTime > 5) {
        sendLongMessage();
        lastLongMsgTime = simTime();
    }
}
void MyVeinsApp::sendShortMessage()
{
    ShortMessage* sm = new ShortMessage();
    populateWSM(sm);
    int send_sumo_id = stoi(mobility->getExternalId());
    sm->setSenderAddress(send_sumo_id);
    std::string content = "short_" + mobility->getExternalId();
    sm->setContent(content.c_str());

    if(isMalicious){
        bool msgisFake = uniform(0,1) < fake_msg_Prob;
        sm->setIsFake(msgisFake);
    } else{
        sm->setIsFake(false);
    }
    sendDown(sm);
    shortMsgCount++;  // 发送计数 +1
}

void MyVeinsApp::sendLongMessage()
{
    LongMessage* lm = new LongMessage();
    populateWSM(lm);
    int send_sumo_id = stoi(mobility->getExternalId());
    lm->setSenderAddress(send_sumo_id);

    lm->setTimestamp(simTime().dbl());
    std::string content = "long" + mobility->getExternalId();
    lm->setContent(content.c_str());
    if(isMalicious){
        bool msgisFake = uniform(0,1) < fake_msg_Prob;
        lm->setIsFake(msgisFake);
    } else{
        lm->setIsFake(false);
    }
    sendDown(lm);
    longMsgCount++;  // 发送计数 +1
}

void MyVeinsApp::onSM(ShortMessage* sm)
{
    int senderId = sm->getSenderAddress();   // 发送者ID
    int msgId = sm->getId();                 // 消息系统ID
    bool isFake = sm->isFake();           // 消息是否伪造
    char msgType = 'S';
    shortMsgRecvCount++;  // 短消息接收计数 +1

    // === 判断发送者是否恶意 ===
    bool senderMalicious = false;
    if (std::find(maliciousVehicles.begin(), maliciousVehicles.end(), senderId) != maliciousVehicles.end()) {
        senderMalicious = true;
    }

    // === 调用评价逻辑 ===
    evaluateMessage(senderId, msgId, isFake, senderMalicious,msgType);
}

void MyVeinsApp::onLM(LongMessage* lm)
{
    int senderId = lm->getSenderAddress();   // 发送者ID
    int msgId = lm->getId();                 // 消息系统ID
    bool isFake = lm->isFake();           // 消息是否伪造
    char msgType = 'L';
    longMsgRecvCount++;  // 长消息接收计数 +1

    // 这里不可以直接使用初始化中的isMalicious来判断吗
    bool senderMalicious = false;
    if (std::find(maliciousVehicles.begin(), maliciousVehicles.end(), senderId) != maliciousVehicles.end()) {
        senderMalicious = true;
    }

    // === 调用评价逻辑 ===
    evaluateMessage(senderId, msgId, isFake, senderMalicious,msgType);
}

void MyVeinsApp::initializeMaliciousList(int totalVehicles, double ratio, int seed) {
    if (initializedMalicious) return;  // 防止重复初始化
    initializedMalicious = true;

    int numMal = std::max(1, int(totalVehicles * ratio));
    std::vector<int> indices(totalVehicles);
    std::iota(indices.begin(), indices.end(), 0);  // [0, 1, 2, ..., N-1]

    std::mt19937 rng(seed);  // 固定 seed
    std::shuffle(indices.begin(), indices.end(), rng);

    maliciousVehicles.assign(indices.begin(), indices.begin() + numMal);

    std::cout << "=== Initialized Malicious Vehicles ===" << std::endl;
    for (int id : maliciousVehicles)
        std::cout << "Vehicle " << id << " marked as malicious" << std::endl;
}

void MyVeinsApp::evaluateMessage(int senderId, int msgId, bool isFake, bool senderMalicious,char msgType) {
    double r = uniform(0, 1);
    int score = 1; // 1=好评, 0=差评

    // 当前车辆自己是否恶意（即“评价者”是否恶意）
    bool selfMalicious = isMalicious;

    if (!selfMalicious) {
        // 正常车辆的评价逻辑
        if (senderMalicious && isFake) {
            // 收到恶意车的虚假消息 → 0.8 概率差评
            score = (r < 0.8) ? 0 : 1;
        } else {
            // 其他消息（正常消息）→ 0.95 概率好评
            score = (r < 0.95) ? 1 : 0;
        }
    }
    else {
        // 恶意车辆的“反向评价”逻辑
        if (senderMalicious && isFake) {
            // 对虚假消息反向 → 0.2 概率差评，否则好评
            score = (r < 0.2) ? 0 : 1;
        } else {
            // 对正常消息反向 → 0.2 概率好评，否则差评
            score = (r < 0.2) ? 1 : 0;
        }
    }

    //记录评价结果
    rate_list.push_back({senderId, msgId, score, msgType});

    // --- 填充反馈列表 ---
    Feedback fb;
    fb.senderId = stoi(mobility->getExternalId());  // 当前车辆（评价者）
    fb.receiverId = senderId;                      // 被评价车辆
    fb.msgType = msgType;                          // 消息类型
    fb.score = score;                              // 评价分数
    fb.round = currentRound;                       // 当前轮次
    feedback_list.push_back(fb);
    gCurrRoundFeedback.push_back(fb);
    allFeedback.push_back(fb);
    MyVeinsApp::activeVehiclesThisRound.insert(fb.senderId);

}


