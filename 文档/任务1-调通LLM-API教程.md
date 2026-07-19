# 任务 1 教程：从零调通第一次 LLM API

> 对应代码：`include/miniagent/Provider.hpp`、`include/miniagent/OpenAiProvider.hpp`、`src/OpenAiProvider.cpp`、`src/main.cpp`、`CMakeLists.txt`
> 读法：开着对应源文件，一节一节对照读。每节先讲"为什么"，再逐行讲"是什么"。

---

## 0. 任务 1 做成了什么

一个命令行程序：你给它一个问题，它把问题发给 DeepSeek 大模型，把回答打印出来。

```powershell
$env:LLM_API_KEY = "sk-..."
.\build\Release\miniagent.exe "用一句话解释什么是TCP"
# → TCP（传输控制协议）是一种面向连接、可靠、基于字节流的传输层通信协议……
# → [finish_reason: stop]
```

数据流全景（先记住这张图，后面每一节都在讲图里的某一段）：

```
main.cpp                OpenAiProvider::chat()                DeepSeek 服务器
   │                          │                                    │
   │ 组装 ChatRequest          │                                    │
   ├─────────────────────────▶│                                    │
   │                          │ ① 拼 JSON 请求体                    │
   │                          │ ② HTTPS POST ─────────────────────▶│
   │                          │                                    │（模型推理）
   │                          │ ③ ◀──────────────── JSON 响应       │
   │                          │ ④ 错误检查                          │
   │                          │ ⑤ 解析出回答文字                     │
   │ ◀─────────────────────── │ 返回 ChatResponse                   │
   │ 打印                      │                                    │
```

**核心认知**：所谓"调用大模型"，本质就是一次带认证头的 HTTPS POST 请求。模型的全部智能都在服务器那边；你的程序只负责三件事——**拼请求、发请求、解析响应**。

---

## 1. 数据结构：`include/miniagent/Provider.hpp`

### 为什么先定义数据结构？

C++ 是强类型语言，先把"对话由什么组成"用结构体固定下来，后面所有函数都围绕这些类型工作。这三个结构体直接对应 OpenAI API 的请求/响应格式。

### 逐段讲解

```cpp
#pragma once
```
防止同一个头文件在一次编译中被 include 两次（否则结构体会被定义两遍，报重定义错误）。C++ 头文件第一行的固定习惯。

```cpp
#include <string>
#include <vector>
```
**用了谁就要引入谁的头**。下面的代码用了 `std::string` 和 `std::vector`，就必须引这两个——你最初漏了它们，编译器报 `"vector": 不是 "std" 的成员`。记住这个报错长相，下次见到就知道是缺 include。

```cpp
namespace miniagent {
```
命名空间 = 给我们所有的类型和函数加统一前缀，避免和第三方库重名冲突。项目里所有代码都住在 `miniagent` 里。

```cpp
struct Message {
    std::string role;      // "system" | "user" | "assistant" | "tool"
    std::string content;
};
```
**对话中的一条消息**。`role` 表示说话人：
- `system`：给模型的隐藏指令（人设、规则）
- `user`：你说的话
- `assistant`：模型说的话
- `tool`：工具执行结果（任务 2 才会用到）

为什么历史消息也要发过去？因为 **LLM 是无状态的**——服务器不记得上一轮说了什么，每次请求都要把完整对话历史带上，模型才能"记得"上下文。

```cpp
struct ChatRequest {
    std::string model;                  // 用哪个模型，如 "deepseek-chat"
    std::vector<Message> messages;      // 完整对话历史
};
```
一次请求 = 选哪个模型 + 到目前为止的所有消息。

```cpp
struct ChatResponse {
    std::string content;       // 模型回复的文字
    std::string finishReason;  // 模型为什么停下来
};
```
`finishReason` 现在只会看到 `"stop"`（正常说完了）。任务 2 之后会出现 `"tool_calls"`（模型想调用工具而不是直接回答）——**这个字段就是 Agent Loop 判断"继续循环还是结束"的依据**，现在埋下伏笔。

---

## 2. 类的声明：`include/miniagent/OpenAiProvider.hpp`

```cpp
#pragma once

#include <string>
#include "miniagent/Provider.hpp"   // 要用 ChatRequest/ChatResponse，必须引入定义它们的头

namespace miniagent {

class OpenAiProvider {
public:
    OpenAiProvider(std::string baseUrl, std::string apiKey);  // 构造函数
    ChatResponse chat(const ChatRequest& req);                // 核心：发一次请求
private:
    std::string baseUrl;   // 形如 https://api.deepseek.com
    std::string apiKey;    // 你的密钥
};

}
```

要点：

1. **头文件只放声明（有什么），实现放 .cpp（怎么做）**。别人想用这个类，看头文件 15 行就够了，不用读 .cpp。这就是之前笔记说的"读代码先读 .hpp"的原因。
2. `baseUrl`/`apiKey` 放 `private`：外部不需要也不应该直接改它们，构造时给一次就固定。
3. **`const ChatRequest& req` 为什么这么写**（思考题 1 的答案）：
   - `&`（引用）：不拷贝。`messages` 里可能有几十条长消息，按值传参会整个复制一遍，浪费。引用相当于"借给函数看"，零拷贝。
   - `const`：承诺"只看不改"。调用者不用担心自己的数据被函数偷偷改掉。
   - 组合起来 `const T&` 是 C++ 传"只读大对象"的标准姿势，以后会见到无数次。
4. 类名叫 **OpenAi**Provider 但我们连的是 DeepSeek？因为 DeepSeek/Kimi/通义等几乎所有国产模型都兼容 **OpenAI 的接口格式**——同一套代码，换个 `baseUrl` 和 `model` 就能用任何一家。这就是"OpenAI 兼容"的含义，也是把类叫这个名字的原因。

---

## 3. 核心实现：`src/OpenAiProvider.cpp`

### 文件开头

```cpp
#include "miniagent/OpenAiProvider.hpp"   // 自己的声明

#include <iostream>
#include <stdexcept>      // std::runtime_error（抛异常用）

#include "httplib.h"           // cpp-httplib：HTTP 客户端库
#include "nlohmann/json.hpp"   // JSON 库

using json = nlohmann::json;   // 起个短别名，后面写 json 就行，不用写 nlohmann::json
```

```cpp
OpenAiProvider::OpenAiProvider(std::string baseUrl, std::string apiKey)
    : baseUrl(std::move(baseUrl)), apiKey(std::move(apiKey))
{
}
```
构造函数用了两个语法点：
- `: baseUrl(...), apiKey(...)` 是**初始化列表**——在成员变量诞生时直接赋值（比在函数体里赋值少一次默认构造）。
- `std::move`：把参数的内容"搬"给成员变量而不是复制（参数马上就没用了，搬走更省）。现阶段理解为"高效版赋值"即可。

### ① 拼 JSON 请求体

目标形状（OpenAI 格式，所有兼容厂商通用）：
```json
{
  "model": "deepseek-chat",
  "messages": [
    { "role": "user", "content": "用一句话解释什么是TCP" }
  ]
}
```

代码：
```cpp
json body;                              // 造一个空 json 对象（想象成空盒子）
body["model"] = req.model;              // 放入 model 字段
body["messages"] = json::array();       // messages 是数组，先放个空数组 []
for (const auto &m : req.messages) {    // 遍历每条历史消息
    json jm;                            // 每条消息一个小盒子
    jm["role"] = m.role;
    jm["content"] = m.content;
    body["messages"].push_back(jm);     // 塞进数组末尾
}
```

语法点：
- `body["model"] = req.model;`——nlohmann/json 让 C++ 写 JSON 像 Python 字典：键不存在就创建，存在就覆盖。
- `for (const auto &m : req.messages)`——**范围 for 循环**，读作"对 messages 里的每个元素 m"。`auto` 让编译器自动推断 m 的类型（Message），`const &` 同样是"只读、不拷贝"。

**你当时的两个错，错在哪**：
- `body["model"] = deepseek-chat;`——没引号，编译器把它当变量名找，找不到就报错。但就算加了引号也不该写死：用 `req.model`，换模型时只改 main.cpp，Provider 不动。**"配置和逻辑分离"**，这是写一切程序的通用原则。
- `for()` 空括号——C++ 的 for 必须写明遍历什么。

### ② 发 HTTPS 请求

```cpp
httplib::Client cli(baseUrl);           // 创建 HTTP 客户端，指向 https://api.deepseek.com
cli.set_read_timeout(120);              // 最多等 120 秒
httplib::Headers headers = {
    {"Authorization", "Bearer " + apiKey}
};
auto res = cli.Post("/chat/completions", headers, body.dump(), "application/json");
```

逐行：
- `set_read_timeout(120)`：LLM 生成回答要几秒到几十秒，默认超时太短会误判失败。**调 LLM 的超时永远要给足**。
- `Authorization: Bearer sk-xxx`：HTTP 世界的"出示证件"标准格式。注意 `"Bearer "` 里有个空格。服务器靠它认出你是谁、有没有余额。
- `body.dump()`：网络上只能传**文本**，`.dump()` 把 json 对象序列化成字符串 `{"model":"deepseek-chat",...}`。
- 最后一个参数 `"application/json"` 是 Content-Type，告诉服务器"我发的是 JSON"。
- `auto res`：类型让编译器推断（实际是 `httplib::Result`）。

### ③ 错误处理——区分"两种失败"

```cpp
if (!res) {     // 情况 A：网络层失败
    throw std::runtime_error("network error: " + httplib::to_string(res.error()));
}
if (res->status != 200) {    // 情况 B：服务器拒绝
    throw std::runtime_error("http " + std::to_string(res->status) + ": " + res->body);
}
```

两种失败**本质不同**（思考题 2 的答案）：

| | 情况 A：`!res` | 情况 B：`status != 200` |
|---|---|---|
| 含义 | 请求根本没送到（断网/超时/证书错） | 送到了，服务器说"不行" |
| 有无响应体 | 没有 | 有，且**里面写着具体原因** |
| 典型例子 | 超时 | 401 key 错、402 欠费、429 太频繁 |
| 用户该干嘛 | 查网络、重试 | 查 key、查账户 |

所以情况 B 一定要把 `res->body` 带进报错——你测试假 key 时看到的 `"Authentication Fails, Your api key ... is invalid"` 就是服务器写在响应体里的原因。**报错信息带上下文，是排错效率的分水岭**。

`throw` 抛出异常后，函数立即终止，一路向上传到 main.cpp 的 `catch` 处被接住打印。异常的好处：错误处理代码集中在一处，主逻辑不用每行都写 if。

### ④ 解析响应

服务器返回的 JSON 长这样（可以临时加 `std::cout << res->body;` 亲眼看）：
```json
{
  "choices": [
    {
      "message": { "role": "assistant", "content": "TCP（传输控制协议）是……" },
      "finish_reason": "stop"
    }
  ],
  "usage": { "prompt_tokens": 12, "completion_tokens": 45 }
}
```

代码：
```cpp
json j = json::parse(res->body);   // 文本 → json 对象（dump 的逆操作）
ChatResponse out;
out.content = j["choices"][0]["message"]["content"].get<std::string>();
out.finishReason = j["choices"][0]["finish_reason"].get<std::string>();
return out;
```

- `json::parse`：响应也是文本，先解析成 json 对象才能按键取值。
- `j["choices"][0][...]`：一层层往里剥。`choices` 是数组（理论上可以要多个候选回答），取第 0 个。
- `.get<std::string>()`：明确说"把这个 json 值当字符串取出"。
- 顺带注意 `usage` 字段——记录这次花了多少 token（=多少钱）。以后做"token 用量统计"（简历亮点之一）就是从这里取数。

---

## 4. 入口：`src/main.cpp`

职责：收集输入（命令行参数、环境变量里的 key）→ 组装 `ChatRequest` → 调 `chat()` → 打印。

```cpp
const char *key = std::getenv("LLM_API_KEY");
```
**key 为什么放环境变量而不是写进代码**：写进代码 = 提交到 git = 泄露给所有能看到代码的人（泄露的 key 会被人刷爆余额）。环境变量只存在于当前终端会话，代码里永远只有"读它"的逻辑。这是行业铁律。

```cpp
try {
    auto resp = provider.chat(req);
    std::cout << resp.content << "\n";
} catch (const std::exception &e) {
    std::cerr << "chat failed: " << e.what() << "\n";
    return 1;
}
```
`try/catch` 接住 `chat()` 里 `throw` 的异常。`e.what()` 就是我们当时塞进 `runtime_error` 的那句话。`std::cerr` 是错误输出流（和 `std::cout` 分开，方便重定向）；`return 1` 表示程序异常退出（0 = 成功，惯例）。

### Windows 中文编码特殊处理（main.cpp 开头那段 `#ifdef _WIN32`）

```cpp
static std::string wideToUtf8(const wchar_t *w) { ... }   // UTF-16 → UTF-8 转换
SetConsoleOutputCP(CP_UTF8);                               // 控制台按 UTF-8 显示输出
LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);  // 拿 UTF-16 版命令行参数
```

为什么需要这段，见下一节"坑 3"。`#ifdef _WIN32 ... #endif` 是条件编译：这段代码只在 Windows 上编译，将来移植到 Linux 时自动消失。

---

## 5. 构建脚本：`CMakeLists.txt` 要点

- **FetchContent**：CMake 配置时自动从 GitHub 下载 nlohmann/json 和 cpp-httplib，不用手动装依赖。
- **`add_executable(miniagent src/main.cpp src/OpenAiProvider.cpp)`**：新加 .cpp 文件必须加进这个列表，否则链接时报"无法解析的外部符号"（函数声明了但找不到实现）。
- **`target_include_directories(miniagent PRIVATE include)`**：告诉编译器 `#include "miniagent/xxx.hpp"` 去 `include/` 目录找。
- **OpenSSL 三段式**：优先系统 OpenSSL → 否则用 `third_party/openssl/` 项目自带的 → 都没有就警告（HTTPS 不可用）。

---

## 6. 三个坑的复盘（比正确代码更值钱）

### 坑 1：中文注释导致上百个诡异编译错误
- **现象**：`"vector": 不是 "std" 的成员` 等大量不合理报错，代码明明没问题。
- **原因**：源文件是 UTF-8，但中文 Windows 上的 MSVC 默认按 GBK 解析。某些中文字符的 UTF-8 字节序列在 GBK 视角下会"吞掉"后面的换行/代码。
- **解法**：CMake 里 `if(MSVC) add_compile_options(/utf-8) endif()`，强制按 UTF-8 读源码。
- **规律**：报错离谱且成片、错误位置都在中文注释附近 → 先怀疑编码。

### 坑 2：`'https' scheme is not supported`
- **原因**：cpp-httplib 默认只会 http。https 需要 TLS 加密，要在编译期链接 OpenSSL 并定义宏 `CPPHTTPLIB_OPENSSL_SUPPORT`。我们的 `find_package(OpenSSL)` 没找到就静默跳过了。
- **解法**：把 MSVC 格式的 OpenSSL 静态库（libssl.lib/libcrypto.lib + 头文件）放进 `third_party/openssl/`，CMake 找不到系统库时用它。
- **规律**：C++ 库的"可选功能"常靠编译期宏开关，功能缺失不一定报编译错，可能到运行时才暴露。

### 坑 3：`invalid UTF-8 byte at index 1: 0xC3`
- **原因**：中文控制台把参数按 GBK 编码放进 `argv`（"用"= `D3 C3`），而 JSON 标准强制 UTF-8，`body.dump()` 校验失败抛异常——程序崩在发送**之前**。
- **解法**：不用 GBK 的 `argv`，改用 `CommandLineToArgvW` 拿 UTF-16 原始参数，`WideCharToMultiByte` 转成 UTF-8；输出侧 `SetConsoleOutputCP(CP_UTF8)`。
- **规律**：Windows + 中文 + 崩在编码 ≈ GBK 和 UTF-8 打架。看到 `0xC3`/`0xD3` 这类"invalid byte"直接往这想。

**三个坑其实是同一个主题**：程序世界里存在多种文字编码（GBK/UTF-8/UTF-16），任何两段代码交接数据时（编译器读源码、控制台传参数、库校验字符串），编码对不上就出事。**统一成 UTF-8** 是现代软件的通用答案。

---

## 7. 自测清单（都能答上来才算过关）

1. 从敲下回车到看到回答，数据经过了哪几步？（不看图能画出来）
2. `role` 有哪四种？为什么每次请求要带完整历史？
3. `const ChatRequest&` 三个符号各起什么作用？
4. `!res` 和 `res->status != 200` 分别代表什么失败？为什么后者要把 `res->body` 放进报错？
5. `body.dump()` 和 `json::parse()` 是什么关系？
6. API key 为什么不能写进代码？
7. 新增一个 .cpp 文件后编译报"无法解析的外部符号"，第一反应查什么？
8. （预习）`finish_reason` 除了 `"stop"` 还会有什么值？它将决定什么？

---

下一篇：任务 2 教程——工具调用与 Agent Loop（完成任务 2 后编写）。
