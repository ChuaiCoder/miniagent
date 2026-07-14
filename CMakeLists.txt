# ============================================================================
# CMake 构建脚本：miniagent 项目
# 作用：声明项目信息、配置 C++ 标准、拉取第三方依赖、生成可执行目标
# ============================================================================

# 指定构建本项目所需的最低 CMake 版本为 3.20
# 低于此版本的 CMake 将拒绝构建，以保证 FetchContent 等特性可用
cmake_minimum_required(VERSION 3.20)

# 声明项目名称为 miniagent，并指定项目语言为 C++
# LANGUAGES CXX：只启用 C++ 编译器，不检查 C 编译器，避免环境缺失 C 编译器时报错
project(miniagent LANGUAGES CXX)

# 设定 C++ 标准为 C++17
# C++17 提供了 structured bindings、optional、string_view、filesystem 等实用特性
set(CMAKE_CXX_STANDARD 17)

# 强制要求使用设定的 C++ 标准，禁止编译器降级到更早的标准
# 若编译器不支持 C++17，构建将直接失败而非静默降级
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源码含中文注释（UTF-8），强制 MSVC 按 UTF-8 解析源文件；
# 否则中文 Windows 上 MSVC 默认按 GBK 读取，会产生大量诡异的语法错误
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ============================================================================
# 依赖管理：使用 FetchContent 在配置阶段拉取第三方库
# 策略：先用最少的两个核心依赖（JSON + HTTP 客户端），后续按里程碑逐步增加
# ============================================================================
include(FetchContent)

# ----------------------------------------------------------------------------
# 依赖 1：nlohmann/json —— 流行的 header-only JSON 解析库（与 IonClaw 项目同款）
# 选用原因：API 直观、文档完善、社区活跃，便于解析 LLM 返回的 JSON 响应
# ----------------------------------------------------------------------------
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# ----------------------------------------------------------------------------
# 依赖 2：cpp-httplib —— 轻量级 header-only HTTP/HTTPS 客户端库
# 选用原因：比 Poco 等重型库更轻，足够用来调用 LLM API
# 备注M7：若后续需求扩展（如连接池、复杂重试），再考虑切换到更完善方案
# ----------------------------------------------------------------------------
FetchContent_Declare(cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3)
FetchContent_MakeAvailable(cpp-httplib)

# ============================================================================
# 可执行目标定义
# 源文件：src/main.cpp（后续按里程碑逐步添加其他源文件到此列表）
# ============================================================================
add_executable(miniagent
    src/main.cpp
    src/OpenAiProvider.cpp
)

# 为 miniagent 目标添加私有头文件包含目录 include/
# PRIVATE：该包含路径仅在编译 miniagent 自身源码时生效，不传递给依赖它的目标
target_include_directories(miniagent PRIVATE include)

# 链接第三方库到 miniagent 目标
# nlohmann_json::nlohmann_json：JSON 库的 CMake imported target
# httplib::httplib：cpp-httplib 库的 CMake imported target
# PRIVATE：这两个库仅在 miniagent 实现中使用，不通过头文件向外暴露
target_link_libraries(miniagent PRIVATE nlohmann_json::nlohmann_json httplib::httplib)

# ============================================================================
# HTTPS 支持：调用 LLM API（如 OpenAI/Claude）必须走 HTTPS，需要 TLS 库
# 策略：优先使用系统已安装的 OpenSSL；若未找到，则回退到 Windows 原生 schannel
# ----------------------------------------------------------------------------
# find_package(OpenSSL QUIET)：静默查找系统 OpenSSL，未找到也不报错
find_package(OpenSSL QUIET)

# 若找到 OpenSSL，则启用 cpp-httplib 的 OpenSSL 支持并链接相关库
if(OpenSSL_FOUND)
    # 定义宏 CPPHTTPLIB_OPENSSL_SUPPORT，让 cpp-httplib 在编译时启用 HTTPS 支持
    target_compile_definitions(miniagent PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    # 链接 OpenSSL 的 SSL 与 Crypto 两个组件，提供 TLS 握手与加密原语
    target_link_libraries(miniagent PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# ============================================================================
# Windows 平台专属链接库
# ws2_32  ：Winsock2 库，提供 socket/网络通信底层 API（cpp-httplib 在 Windows 上依赖）
# crypt32 ：Windows 加密 API 库，用于证书解析等功能（HTTPS/schannel 路径可能需要）
# ----------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(miniagent PRIVATE ws2_32 crypt32)
endif()
