#include "miniagent/OpenAiProvider.hpp"

#include <iostream>
#include <stdexcept>

#include "httplib.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace miniagent {

OpenAiProvider::OpenAiProvider(std::string baseUrl, std::string apiKey)
    : baseUrl(std::move(baseUrl)), apiKey(std::move(apiKey))
{
}

ChatResponse OpenAiProvider::chat(const ChatRequest &req)
{
    // ======================================================================
    // 空格 1：把 req 拼成 OpenAI 格式的 JSON 请求体
    // 目标形状：{ "model": "deepseek-chat",
    //            "messages": [ {"role":"user","content":"你好"}, ... ] }
    //
    // 提示：
    //   body["model"] = ...;
    //   body["messages"] = json::array();
    //   然后 for 循环遍历 req.messages：
    //       for (const auto &m : req.messages) { ... }
    //   循环体里造一个 json 对象，塞 role 和 content 两个键，
    //   再 body["messages"].push_back(它);
    // ======================================================================
    json body;
    // TODO(你来写)：大约 6 行
    body["model"] = ChatRequest.model;


    // ======================================================================
    // 空格 2：发 HTTP 请求
    // 提示（去掉注释符号就是能用的代码骨架，但 ??? 要自己想）：
    //   httplib::Client cli(baseUrl);              // baseUrl 形如 https://api.deepseek.com
    //   cli.set_read_timeout(120);                 // LLM 响应慢，超时给足 120 秒
    //   httplib::Headers headers = {
    //       {"Authorization", ???}                 // Bearer 认证："Bearer " 拼上 apiKey
    //   };
    //   auto res = cli.Post("/chat/completions", headers, body.dump(), "application/json");
    // ======================================================================
    // TODO(你来写)：大约 5 行，最后一行必须得到名为 res 的变量



    // ======================================================================
    // 空格 3：错误处理（先想清楚两种失败的区别再写）
    //   情况 A：!res → 网络层失败（连不上/超时/证书问题）
    //           throw std::runtime_error("network error: " + httplib::to_string(res.error()));
    //   情况 B：res->status != 200 → 服务器拒绝（key 错/参数错/欠费）
    //           把 res->body 放进异常信息——服务器会告诉你具体原因！
    // ======================================================================
    // TODO(你来写)：大约 6 行（两个 if + 两个 throw）



    // ======================================================================
    // 空格 4：解析响应，填充 ChatResponse
    // 响应 JSON 长这样（建议先在空格 3 之后临时加一行
    //     std::cout << res->body << "\n";  亲眼看一次真实响应再写）：
    //   { "choices": [ { "message": { "content": "回答文字" },
    //                    "finish_reason": "stop" } ], ... }
    // 提示：
    //   json j = json::parse(res->body);
    //   j["choices"][0]["message"]["content"].get<std::string>()
    // ======================================================================
    ChatResponse out;
    // TODO(你来写)：大约 4 行

    return out;
}

} // namespace miniagent
