#ifndef MESSAGE_HPP
#define MESSAGE_HPP
#include <string>
#include <vector>

// PBFT 消息基类
struct PBFTMessage {
    int view_number;      // 当前视图编号
    int sequence_number;  // 请求序号
    int node_id;          // 发送节点ID
    std::string digest;   // 请求摘要（简化实现可用字符串代替哈希）
    
    virtual ~PBFTMessage() {}  // 添加虚析构函数以支持多态
};

// 预准备消息
struct PrePrepareMsg : public PBFTMessage {
    std::string request_data;  // 客户端请求数据
};

// 准备消息
struct PrepareMsg : public PBFTMessage {};

// 提交消息
struct CommitMsg : public PBFTMessage {};

#endif // MESSAGE_HPP