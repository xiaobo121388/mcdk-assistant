# MCDK-ASSISTANT

为 MCMOD / 网易 MC 开发场景提供一组面向 AI 的 MCP 工具，重点覆盖资料检索、JSON UI、模型、动画、像素画和网易版差异说明等能力。

## 项目定位

这个项目本质上是一个本地 MCP 服务端，供支持 MCP 的客户端接入。接入后，AI 可以直接调用项目内封装好的工具，而不是只靠纯文本回答。

当前能力主要分为几类：

- 文档与知识库搜索：[`src/search/search_service.hpp`](src/search/search_service.hpp)
- MCP 工具注册：[`src/tools/`](src/tools)
- 启动与运行时装配：[`src/app/`](src/app)
- 路径与编码处理：[`src/common/path_utils.hpp`](src/common/path_utils.hpp)
- Server 版统计能力：[`src/server/tool_stats.hpp`](src/server/tool_stats.hpp)

## 运行模式

项目目前维护三个可执行目标：

- 完整版：[`mcdk-assistant`](CMakeLists.txt)
  - 包含搜索、JSON UI、模型、动画、像素画等完整工具集。
- Lite 版：[`mcdk-asst-lite`](CMakeLists.txt)
  - 适合只需要知识库检索和基础说明工具的场景。
- Server 版：[`mcdk-asst-server`](CMakeLists.txt)
  - 面向部署使用，包含工具调用统计与 HTTP 查询端点。

默认监听端口为 `18766`，SSE 地址通常为 `http://localhost:18766/sse`。

## 目录说明

当前代码结构已经按职责做过一轮整理：

- [`src/main.cpp`](src/main.cpp) 负责启动编排
- [`src/app/runtime_paths.cpp`](src/app/runtime_paths.cpp) 负责运行目录探测
- [`src/app/server_runtime.cpp`](src/app/server_runtime.cpp) 负责服务配置、工具装配与 server 端点注册
- [`src/common/path_utils.hpp`](src/common/path_utils.hpp) 负责路径与 UTF-8 转换边界
- [`src/search/index_cache.hpp`](src/search/index_cache.hpp) 负责索引缓存读写
- [`src/search/search_service.hpp`](src/search/search_service.hpp) 负责知识库加载、缓存恢复与搜索服务

资源目录约定：

- [`dicts/`](dicts) 为分词词典目录
- [`knowledge/`](knowledge) 为知识库目录

运行时默认按可执行文件相邻目录查找这些资源，而不是依赖当前工作目录。

## 路径与编码约定

为了避免 Windows 下中文路径和本地代码页问题，当前实现采用以下约定：

- 模块内部优先传递 [`std::filesystem::path`](src/common/path_utils.hpp)
- 只有在日志输出、JSON 返回、MCP 文本协议边界时，才转换成 UTF-8 [`std::string`](src/common/path_utils.hpp)
- Windows 获取可执行文件路径时使用宽字符 API，而不是 ANSI 版本

如果后续新增文件读写逻辑，建议优先复用 [`src/common/path_utils.hpp`](src/common/path_utils.hpp) 的工具函数和约定，不要重新混用裸路径字符串。

## MCP 客户端配置

支持标准 MCP 客户端接入，下面以 `Roo Code` 为例。

```jsonc
{
  "mcpServers": {
    "minecraft_mod_assistant": {
      "url": "http://localhost:18766/sse",
      "name": "Minecraft Mod Assistant MCP Server (MCDK)",
      "alwaysAllow": ["*"],
      "timeout": 30
    }
  }
}
```

### VSCode（Copilot）

VSCode 当前不能直接连接 SSE，需要通过 `mcp-remote` 做桥接。配置文件通常放在 [`.vscode/mcp.json`](.vscode/mcp.json)：

```jsonc
{
  "servers": {
    "minecraft_mod_assistant": {
      "command": "npx",
      "args": [
        "mcp-remote",
        "http://localhost:18766/sse",
        "--transport",
        "sse-only"
      ]
    }
  }
}
```

> 这里依赖 Node.js 环境。

## 构建说明

项目使用 [`CMakeLists.txt`](CMakeLists.txt) 管理构建，当前已把通用 target 配置抽成公共函数，Lite / Server / 完整版共用一套平台参数逻辑。

常见流程：

1. 配置生成目录
2. 选择目标，如 `mcdk-assistant`、`mcdk-asst-lite` 或 `mcdk-asst-server`
3. 生成后保证可执行文件旁边存在 [`dicts/`](dicts) 与 [`knowledge/`](knowledge)

如果你是部署使用，建议重点检查：

- SSE 端口是否可访问
- 资源目录是否和可执行文件处于预期相对位置
- Server 版是否需要读取 [`datas/tool_stats.json`](src/server/tool_stats.hpp)

## 维护建议

如果后续继续扩展项目，建议遵循下面几条：

- 启动逻辑优先放在 [`src/app/`](src/app)
- 路径与编码逻辑优先放在 [`src/common/path_utils.hpp`](src/common/path_utils.hpp)
- 搜索缓存和索引恢复逻辑优先放在 [`src/search/`](src/search)
- 工具注册层放在 [`src/tools/`](src/tools)，不要把启动装配重新堆回 [`main()`](src/main.cpp:9)

这样可以尽量保持当前这轮重构后的模块边界，不再回到所有逻辑挤在一个入口文件里的状态。
