import veins.base.utils.Coord;
import veins.modules.messages.BaseFrame1609_4;
import veins.base.utils.SimpleAddress;

namespace veins;

packet LongMessage extends BaseFrame1609_4 {
    Coord senderPos;                     // 发送者位置
    double timestamp ;           // 时间戳
    string content;                     // 长消息内容（例如数据摘要）
    LAddress::L2Type senderAddress = -1; // 发送者 ID
    bool isFake = false;
}
