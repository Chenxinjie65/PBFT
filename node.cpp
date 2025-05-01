#include "node.hpp"
#include <algorithm>
#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <sstream>

std::mutex Node::cout_mutex_;

Node::Node(int id, int totalNodes) : node_id_(id), total_nodes_(totalNodes) {
}

Node::~Node() {
    stopMessageLoop();
    while (!message_queue_.empty()) {
        delete message_queue_.front();
        message_queue_.pop();
    }
}

void Node::setNetwork(const std::vector<Node*>& nodes) {
    network_ = nodes;
}
void Node::startMessageLoop() {
    running_ = true;
    while (running_) {
        PBFTMessage* msg = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { 
                return !message_queue_.empty() || !running_; 
            });
            
            if (!running_) break;
            
            if (!message_queue_.empty()) {
                msg = message_queue_.front();
                message_queue_.pop();
            }
        }
        
        if (msg) {
            handleMessage(*msg);
            delete msg;
        }
    }
}

void Node::stopMessageLoop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_all();
}

void Node::handleClientRequest(const PrePrepareMsg& request) {
    if (node_id_ == getPrimaryNode()) {
        log(formatLog("(主节点) 收到客户端请求", request.request_data));
        
        // 创建PrePrepare消息
        PrePrepareMsg pre_prepare = request;
        pre_prepare.node_id = node_id_;
        log(formatLog("广播PrePrepare消息"));
        broadcast(pre_prepare);
        
        // 主节点自己也处理这个PrePrepare消息
        handlePrePrepare(pre_prepare);
    }
}

void Node::broadcast(const PBFTMessage& msg) {
    for (auto node : network_) {
        if (node->getNodeId() != node_id_) {
            PBFTMessage* msg_copy = nullptr;
            if (const PrePrepareMsg* pre = dynamic_cast<const PrePrepareMsg*>(&msg)) {
                msg_copy = new PrePrepareMsg(*pre);
            } else if (const PrepareMsg* prep = dynamic_cast<const PrepareMsg*>(&msg)) {
                msg_copy = new PrepareMsg(*prep);
            } else if (const CommitMsg* com = dynamic_cast<const CommitMsg*>(&msg)) {
                msg_copy = new CommitMsg(*com);
            }
            
            if (msg_copy) {
                std::unique_lock<std::mutex> lock(node->mutex_);
                node->message_queue_.push(msg_copy);
                node->cv_.notify_one();
            }
        }
    }
}

void Node::handleMessage(const PBFTMessage& msg) {
    if (const PrePrepareMsg* pre_prepare = dynamic_cast<const PrePrepareMsg*>(&msg)) {
        if (node_id_ != getPrimaryNode()) { // 非主节点才处理PrePrepare
            handlePrePrepare(*pre_prepare);
        }
    } 
    else if (const PrepareMsg* prepare = dynamic_cast<const PrepareMsg*>(&msg)) {
        handlePrepare(*prepare);
    } 
    else if (const CommitMsg* commit = dynamic_cast<const CommitMsg*>(&msg)) {
        handleCommit(*commit);
    }
}

void Node::handlePrePrepare(const PrePrepareMsg& msg) {
    if (msg.node_id == getPrimaryNode() && 
        msg.sequence_number == sequence_number_ + 1) {
            
        pre_prepares_.push_back(msg);
        sequence_number_ = msg.sequence_number;
        
        if (msg.node_id != node_id_) {
        std::stringstream ss;
        ss << "收到PrePrepare消息 (来自节点 " << msg.node_id << ")";
        log(formatLog(ss.str()));
        }
        
        // 广播 Prepare 消息
        PrepareMsg prepare;
        prepare.view_number = view_number_;
        prepare.sequence_number = sequence_number_;
        prepare.node_id = node_id_;
        prepare.digest = msg.digest;
        
        log(formatLog("广播Prepare消息"));
        broadcast(prepare);
        
        // 自己也处理这个Prepare消息
        handlePrepare(prepare);
    }
}

void Node::handlePrepare(const PrepareMsg& msg) {
    bool exists = false;
    for (const auto& p : prepares_) {
        if (p.node_id == msg.node_id && p.sequence_number == msg.sequence_number) {
            exists = true;
            break;
        }
    }
    
    if (!exists) {
        prepares_.push_back(msg);
        if (msg.node_id != node_id_) {
        std::stringstream ss;
        ss << "收到Prepare消息 (来自节点 " << msg.node_id << ")";
        log(formatLog(ss.str()));
        }
    }
    
    if (!has_sent_commit_ && checkQuorum(prepares_, getFaultyNodes())) {
        log(formatLog("收集到足够的Prepare消息"));
        
        // 广播 Commit 消息
        CommitMsg commit;
        commit.view_number = view_number_;
        commit.sequence_number = sequence_number_;
        commit.node_id = node_id_;
        commit.digest = msg.digest;
        
        log(formatLog("广播Commit消息"));
        broadcast(commit);
        has_sent_commit_ = true;
        
        // 自己也处理这个Commit消息
        handleCommit(commit);
    }
}

void Node::handleCommit(const CommitMsg& msg) {
    bool exists = false;
    for (const auto& c : commits_) {
        if (c.node_id == msg.node_id && c.sequence_number == msg.sequence_number) {
            exists = true;
            break;
        }
    }
    
    if (!exists) {
        commits_.push_back(msg);
        if (msg.node_id != node_id_) {
        std::stringstream ss;
        ss << "收到Commit消息 (来自节点 " << msg.node_id << ")";
        log(formatLog(ss.str()));
        }
    }
    
    if (!has_executed_ && checkQuorum(commits_, getFaultyNodes())) {
        log(formatLog("收集到足够的Commit消息"));
        
        // 执行客户端请求
        executeRequest();
        has_executed_ = true;
        
        resetState();
    }
}

void Node::executeRequest() {
    if (!pre_prepares_.empty()) {
        log(formatLog("执行请求", pre_prepares_[0].request_data));
    }
}

// 计算最大容错节点数 f = floor((N-1)/3)
int Node::getFaultyNodes() const {
    return (total_nodes_ - 1) / 3;
}

// 判断当前主节点
int Node::getPrimaryNode() const {
    return view_number_ % total_nodes_;
}
