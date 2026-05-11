import veins.base.utils.Coord;
import veins.modules.messages.BaseFrame1609_4;
import veins.base.utils.SimpleAddress;

namespace veins;

packet ShortMessage extends BaseFrame1609_4 {
    Coord senderPos;                     // 发送者位置
    //double senderSpeed @unit(mps);       // 当前车速
    string content;                    // 短消息内容（例如临时警告）
    LAddress::L2Type senderAddress = -1; // 发送者 ID
    bool isFake = false;
}
