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

## 🔨 任务 1（M1 第一步）：调通一次 LLM API —— 填空模式

**我已经把整个框架搭好了**，你只需要填 `src/OpenAiProvider.cpp` 里的 **4 个空格**（标了 `TODO(你来写)`）。
框架现在能编译通过（空格空着时函数返回空结果），所以你可以填一个、编译一次、慢慢来。

- **空格 1**：把 `req` 拼成请求 JSON（约 6 行，含一个 for 循环）
- **空格 2**：用 httplib 发 POST 请求（约 5 行，注释里给了骨架）
- **空格 3**：错误处理（两个 if + 两个 throw）
- **空格 4**：解析响应，取出 content 和 finish_reason（约 4 行）

每个空格上方的注释写清了：目标、形状、提示代码、大概几行。**强烈建议先填空格 1、2、3，然后在空格 3 后面临时加一行 `std::cout << res->body;` 编译运行，亲眼看看真实响应长什么样，再写空格 4。**

### 编译 & 运行
```powershell
cd E:\aicodeing\miniagent
cmake --build build --config Release
$env:LLM_API_KEY = "sk-..."     # 你的 key
.\build\Release\miniagent.exe "用一句话解释什么是TCP"
```

### 验收标准
输出模型对问题的真实回答 + `[finish_reason: stop]`。

### 每填完一个空格，或卡住了，就把 OpenAiProvider.cpp 发我，我帮你看。
不要憋着——填空阶段的目标是"你写出来"，不是"你独自写出来"。

### 完成后思考题（下次我会问）
1. 为什么 `chat()` 的入参用 `const ChatRequest&` 而不是 `ChatRequest`？
2. 你的错误处理里，"HTTP 401" 和 "网络超时" 用户看到的信息一样吗？应该一样吗？

---

（任务 2 预告：给 Provider 加上"工具说明书"字段和 tool_calls 解析——完成任务 1 后解锁。）
