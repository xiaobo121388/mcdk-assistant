**项目计划梳理（初版）：**

**目标：** 为 Minecraft 开发构建本地 MCP 服务端（mcdk-assistant）

**技术栈：**
- cpp-mcp 库（HTTP SSE，无 SSL，本地 127.0.0.1）
- cppjieba 中文分词
- BM25 算法实现语义相似度排序

**目录结构：**
```
mcdk-assistant/
├── dicts/          # jieba 词典文件
├── knowledge/      # Markdown 资料库
├── src/
│   ├── core/       # MCP 服务器核心
│   ├── search/     # 文档搜索引擎（当前阶段）
│   └── tools/      # 未来：资源操作工具
└── plans/          # 项目计划文档
```

**架构设计：**
- 模块化分层，各功能独立实现
- 统一通过 MCP tool 注册接口暴露
- 便于后续扩展资源操作等功能

**当前阶段（Phase 1）：**
实现文档搜索工具 `search_docs(keyword, start_index?, end_index?)`
- jieba 分词 + BM25 排序
- 返回：`[{content, file, line_start, line_end, score}]`
- 不指定范围返回全部

**后续阶段（Phase 2+）：**
- mcmod 资源快速操作工具
- 其他开发辅助功能