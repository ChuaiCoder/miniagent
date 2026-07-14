#include <cstdlib>
#include <iostream>

#include "miniagent/OpenAiProvider.hpp"

int main(int argc, char *argv[])
{
    // 用法：miniagent.exe "你的问题"
    if (argc < 2)
    {
        std::cout << "usage: miniagent <question>\n";
        return 1;
    }

    const char *key = std::getenv("LLM_API_KEY");
    if (!key)
    {
        std::cerr << "error: set LLM_API_KEY environment variable first\n";
        return 1;
    }

    // 换其他家就改这两行（baseUrl / model）
    miniagent::OpenAiProvider provider("https://api.deepseek.com", key);

    miniagent::ChatRequest req;
    req.model = "deepseek-chat";
    req.messages.push_back({"user", argv[1]});

    try
    {
        auto resp = provider.chat(req);
        std::cout << resp.content << "\n";
        std::cout << "[finish_reason: " << resp.finishReason << "]\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "chat failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
