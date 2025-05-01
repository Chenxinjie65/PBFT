# PBFT DEMO

A simple C++ implementation of the Practical Byzantine Fault Tolerance (PBFT) consensus algorithm. This project demonstrates the basic workflow of PBFT, including Pre-prepare, Prepare, and Commit phases.

## Features

- Complete PBFT consensus workflow simulation
- Basic fault tolerance mechanism, allowing up to (N-1)/3 nodes to be faulty
- Thread-safe message queue for inter-node communication

## Getting Started

1. Clone the repository:
```bash
git clone https://github.com/Chenxinjie65/PBFT_DEMO.git
cd PBFT_DEMO
```
2. Compile the project:
```
g++ -std=c++11 main.cpp node.cpp -o pbft_simulation -pthread
```

3. Running the Simulation,Execute the compiled binary:
```
./pbft_simulation
```
The simulation will:
1. Initialize 4 nodes (N=4, supporting f=1 Byzantine faults)
2. Start a message processing thread for each node
3. Simulate a client request
4. Demonstrate the complete PBFT consensus process
5. Show execution time statistics

## Output Example

The program will display the consensus process with detailed logs:

```plaintext
开始PBFT共识流程...
节点 0 (主节点) 收到客户端请求: Transfer 100$ from A to B
节点 0 广播PrePrepare消息
...
共识过程完成! 耗时: x.xxx秒
```
## Project Structure

- main.cpp : Entry point, node initialization, and client request simulation
- node.hpp/cpp : Node class implementation with core PBFT protocol logic
- message.hpp : Message type definitions (Pre-prepare, Prepare, Commit)