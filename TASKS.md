# miniagent 复现任务书

> 规则：每个任务你自己写代码，写完告诉我，我 review + 帮你调通 + 布置下一个。
> 卡住超过 30 分钟就来问我，卡着不动是最低效的学法。
> 参考实现在 `E:\aicodeing\ionclaw\ionclaw`，原则：**先自己想，写不出来再去看它对应的文件，看懂后合上自己写**。

---

## ✅ M0：项目骨架（已完成，我搭的）

- CMake + nlohmann/json + cpp-httplib（带 OpenSSL 时支持 HTTPS）
- 构建命令：
  ```powershell
  cd E:\aicodeing\miniagent
  cmake --build build --config Release
  .\build\Release\miniagent.exe
  ```
- 你的任务：通读 `CMakeLists.txt`，每一行不懂的问我。

---

## ✅ 任务 1（M1）：调通一次 LLM API —— 已完成 🎉

实现了 `OpenAiProvider::chat()`：拼请求 JSON → HTTPS POST（带 Bearer 认证）→ 错误处理 → 解析响应。
过程中踩平三个 Windows 经典坑，都记下来：
1. **CMake 中文注释乱码** → MSVC 加 `/utf-8` 编译选项
2. **`'https' scheme is not supported`** → cpp-httplib 需要链接 OpenSSL 并开 `CPPHTTPLIB_OPENSSL_SUPPORT` 宏（已把 MSVC 版 OpenSSL 放进 `third_party/openssl/`，项目自包含）
3. **`invalid UTF-8 byte 0xC3`** → 命令行中文参数是 GBK，用 `CommandLineToArgvW` 拿 UTF-16 再转 UTF-8

**核心认知**：调大模型 = 一次带认证的 HTTPS POST。模型的智能在服务器，你的程序只做"拼请求→发→解析"。

### 完成后思考题（答案）
1. `const ChatRequest&` 用引用避免拷贝整个消息列表（可能很大），`const` 保证函数不会改动调用者的数据。
2. 不一样也不该一样：401 是"key 错，去查配置"，超时是"网络问题，重试可能就好"——不同原因要给用户不同的下一步。

---

## 🔨 任务 2（M2）：让模型学会调用工具 —— agent 的灵魂

**目标**：给模型一个 `list_dir` 工具（列目录文件），当你问"D盘有什么文件"时，模型请求调用它，你的程序执行并把结果喂回去，模型据此回答。

这是从"聊天机器人"到"agent"的关键一步。分几个小步走，仍然是**我讲你读**的节奏：

- **2.1** 扩展数据结构：`ChatRequest` 加"工具说明书"，`ChatResponse` 加"模型想调的工具"
- **2.2** 改 `chat()`：请求里带上工具列表，响应里解析出 `tool_calls`
- **2.3** 写第一个工具 `list_dir`
- **2.4** 写 Agent Loop：调模型 → 若要调工具则执行 → 结果塞回 → 再调模型，循环到出最终答案

每一步我都会先写代码 + 逐行讲解，你读懂后我们再往下。

### Agent Loop 流程（反复看这张图直到刻进脑子）
```
messages = [用户问题]
loop:
    resp = chat(messages, 工具列表)
    如果 resp 没有 tool_calls:  → 打印 resp.content，结束
    否则:
        对每个 tool_call:
            result = 执行对应工具(参数)
            messages 追加 {role:"tool", 内容:result}
        继续 loop  ← 带着工具结果再问一次模型
```

**下一步**：等你说"继续"，我就开始讲 2.1（扩展数据结构）。不急，先把上面这张 Agent Loop 图和任务 1 的三个坑消化一下。
