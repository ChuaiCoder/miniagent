#include <cstdlib>
#include <iostream>
#include <string>

#include "miniagent/OpenAiProvider.hpp"

#ifdef _WIN32
#include <windows.h>

// Windows 中文环境下，命令行参数默认是 GBK 编码，而 JSON 要求 UTF-8。
// 做法：从系统拿到"宽字符(UTF-16)"的原始命令行，再转成 UTF-8，绕开 GBK。
static std::string wideToUtf8(const wchar_t *w)
{
    if (!w) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string out(len > 0 ? len - 1 : 0, '\0');
    if (len > 1)
        WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), len, nullptr, nullptr);
    return out;
}
#endif

int main(int argc, char *argv[])
{
    std::string question;

#ifdef _WIN32
    // 控制台按 UTF-8 输出，这样模型返回的中文才能正常显示
    SetConsoleOutputCP(CP_UTF8);

    // 用宽字符版命令行参数，转成 UTF-8
    int wargc = 0;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (wargv && wargc >= 2)
        question = wideToUtf8(wargv[1]);
    if (wargv)
        LocalFree(wargv);
#else
    if (argc >= 2)
        question = argv[1];
#endif

    // 用法：miniagent.exe "你的问题"
    if (question.empty())
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
    req.messages.push_back({"user", question});

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
