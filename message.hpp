#ifndef MESSAGE_HPP
#define MESSAGE_HPP
#include <string>
#include <vector>

// PBFT 消息基类
struct PBFTMessage {
    int view_number;      // 当前视图编号
    int sequence_number;  // 请求序号
    int node_id;          // 发送节点ID
    std::string digest;   // 请求摘要
    
    virtual ~PBFTMessage() {}  
};

struct PrePrepareMsg : public PBFTMessage {
    std::string request_data;  // 客户端请求数据
};

struct PrepareMsg : public PBFTMessage {};

struct CommitMsg : public PBFTMessage {};

#endif