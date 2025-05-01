#include <iostream>
#include "node.hpp"
#include "message.hpp"
#include <vector>
#include <sys/time.h>
#include <thread>
#include <chrono>
#include <mutex>

// 用于主线程输出的互斥锁
// 防止输出顺序混乱
std::mutex main_cout_mutex;

void log(const std::string& message) {
    std::lock_guard<std::mutex> lock(main_cout_mutex);
    std::cout << message << std::endl;
}

int main() {
    const int N = 4;  // 总节点数（至少 4 个，容错 f=1）
    std::vector<Node*> nodes;
    std::vector<std::thread> node_threads;

    // 创建节点
    for (int i = 0; i < N; i++) {
        nodes.push_back(new Node(i, N));
    }
    for (auto node : nodes) {
        node->setNetwork(nodes);
    }
    
    // 为每个节点创建处理线程
    for (auto node : nodes) {
        node_threads.emplace_back([node]() {
            node->startMessageLoop();
        });
    }

    // 模拟客户端发起请求（由主节点0处理）
    PrePrepareMsg pre_prepare;
    pre_prepare.view_number = 0;
    pre_prepare.sequence_number = 1;
    pre_prepare.node_id = 0;
    pre_prepare.digest = "REQUEST_123";
    pre_prepare.request_data = "Transfer 100$ from A to B";
    
    log("开始PBFT共识流程...");

    // 记录开始时间
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // 主节点处理客户端请求并广播PrePrepare
    nodes[0]->handleClientRequest(pre_prepare);
    
    // 模拟共识过程，增加等待时间确保共识完成
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 记录结束时间并计算耗时
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    
    // 计算时间差（微秒）
    long time_diff = (end_time.tv_sec - start_time.tv_sec) * 1000000 + 
                    (end_time.tv_usec - start_time.tv_usec);
    
    std::stringstream ss;
    ss << "共识过程完成! 耗时: " << time_diff / 1000000.0 << "秒";
    log(ss.str());

    // 停止所有节点的消息循环
    for (auto node : nodes) {
        node->stopMessageLoop();
    }
    
    // 等待所有线程结束
    for (auto& thread : node_threads) {
        thread.join();
    }

    for (auto node : nodes) {
        delete node;
    }
    
    return 0;
}