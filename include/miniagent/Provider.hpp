#pragma once
// ↑ 防止同一个头文件被 include 两次导致重复定义，C++ 头文件第一行的固定习惯

#include <string>   // 用了 std::string 就必须引入
#include <vector>   // 用了 std::vector 就必须引入

namespace miniagent {

struct ChatRequest {
    std::string model;
    std::vector<Message> messages;
};

struct Message {
    std::string role;      // "system" | "user" | "assistant" | "tool"
    std::string content;
};

struct ChatResponse {
    std::string content;       // 模型回复的文字
    std::string finishReason;  // 为什么停："stop" 正常结束（后面会有 "tool_calls"）
};

}
