#include <vector>
#include <string>
#include <iostream>
#include "message.hpp"
#include <mutex>
#include <queue>
#include <condition_variable>
#include <sstream>

class Node {
public:
    Node(int id, int totalNodes);
    ~Node(); 

    void setNetwork(const std::vector<Node*>& nodes);
    
    // 处理客户端请求
    void handleClientRequest(const PrePrepareMsg& request);
    
    // 处理接收到的消息
    void handleMessage(const PBFTMessage& msg);
    
    // 获取节点ID
    int getNodeId() const { return node_id_; }

    // 线程控制
    void startMessageLoop();
    void stopMessageLoop();

    // 新的共识轮，重置节点状态
    void resetState() {
        view_number_ = 0;
        sequence_number_ = 0;
        has_sent_commit_ = false;
        has_executed_ = false;
        pre_prepares_.clear();
        prepares_.clear();
        commits_.clear();
        
        std::unique_lock<std::mutex> lock(mutex_);
        while (!message_queue_.empty()) {
            delete message_queue_.front();
            message_queue_.pop();
        }
    }
    
private:
    int node_id_;
    int total_nodes_;
    int view_number_ = 0;
    int sequence_number_ = 0;
    bool has_sent_commit_ = false;
    bool has_executed_ = false;
    
    // 网络中的其他节点
    std::vector<Node*> network_;
    
    // 消息存储
    std::vector<PrePrepareMsg> pre_prepares_;
    std::vector<PrepareMsg> prepares_;
    std::vector<CommitMsg> commits_;
    
    // 线程控制
    std::queue<PBFTMessage*> message_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = false;

    // 输出控制
    static std::mutex cout_mutex_;
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex_);
        std::cout << message << std::endl;
    }

    std::string formatLog(const std::string& action, const std::string& details = "") {
        std::stringstream ss;
        ss << "节点 " << node_id_ << " " << action;
        if (!details.empty()) {
            ss << ": " << details;
        }
        return ss.str();
    }

    // 消息广播
    void broadcast(const PBFTMessage& msg);
    
    // 各阶段处理函数
    void handlePrePrepare(const PrePrepareMsg& msg);
    void handlePrepare(const PrepareMsg& msg);
    void handleCommit(const CommitMsg& msg);
    
    // 执行请求
    void executeRequest();
    
    // 检查是否达成条件（2f+1个相同消息）
    template<typename T>
    bool checkQuorum(const std::vector<T>& messages, int f) {
        return messages.size() >= 2 * f + 1;
    }
    
    // 计算最大容错节点数
    int getFaultyNodes() const;
    
    // 判断当前主节点
    int getPrimaryNode() const;
};