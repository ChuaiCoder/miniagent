#pragma once

#include <string>

#include "miniagent/Provider.hpp"  // 要用到 ChatRequest/ChatResponse，必须引入定义它们的头

namespace miniagent {   // 和 Provider.hpp 同一个命名空间

class OpenAiProvider {
public:
    OpenAiProvider(std::string baseUrl, std::string apiKey);
    ChatResponse chat(const ChatRequest& req);
private:
    std::string baseUrl;
    std::string apiKey;
};

}
