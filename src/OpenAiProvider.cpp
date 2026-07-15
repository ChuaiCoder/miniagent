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
    // ================== 空格 1：拼请求 JSON ==================
    // 目标形状：{ "model":"deepseek-chat",
    //            "messages":[ {"role":"user","content":"你好"} ] }
    json body;                              // 造一个空的 json 对象，像空盒子
    body["model"] = req.model;              // 往盒子里放 model 字段，值用传进来的 req.model
    body["messages"] = json::array();       // messages 是个数组，先放一个空数组 []
    for (const auto &m : req.messages) {    // 遍历 req 里的每一条消息 m
        json jm;                            // 每条消息也造一个小盒子
        jm["role"] = m.role;               // 放 role（"user"/"assistant"...）
        jm["content"] = m.content;         // 放 content（消息正文）
        body["messages"].push_back(jm);    // 把小盒子塞进 messages 数组末尾
    }

    // ================== 空格 2：发 HTTP 请求 ==================
    httplib::Client cli(baseUrl);           // 创建一个 HTTP 客户端，指向 baseUrl
    cli.set_read_timeout(120);              // LLM 慢，最多等 120 秒再判定超时
    httplib::Headers headers = {            // 请求头（HTTP 的"信封上的备注"）
        {"Authorization", "Bearer " + apiKey}   // 认证：固定前缀 "Bearer " 拼上你的 key
    };
    // 发 POST：路径 /chat/completions，带上 headers，body 转成字符串，声明是 JSON
    auto res = cli.Post("/chat/completions", headers, body.dump(), "application/json");

    // ================== 空格 3：错误处理 ==================
    if (!res) {                             // res 为空 = 连不上/超时/证书错，网络层就失败了
        throw std::runtime_error("network error: " + httplib::to_string(res.error()));
    }
    if (res->status != 200) {               // 连上了但服务器返回非 200 = key 错/参数错/欠费
        throw std::runtime_error("http " + std::to_string(res->status) + ": " + res->body);
    }

    // ================== 空格 4：解析响应 ==================
    // 响应形状：{ "choices":[ { "message":{"content":"回答"},
    //                          "finish_reason":"stop" } ] }
    json j = json::parse(res->body);        // 把返回的文本解析成 json 对象
    ChatResponse out;                       // 造一个要返回的结果
    out.content = j["choices"][0]["message"]["content"].get<std::string>();
    out.finishReason = j["choices"][0]["finish_reason"].get<std::string>();
    return out;                             // 交回给调用者
}

} // namespace miniagent
