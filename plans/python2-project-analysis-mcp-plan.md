# Python 2 项目分析 MCP Tool 规划

## 1. 目标

基于仓库内现有的 `tree-sitter` 与 `tree-sitter-python` 依赖，构建一个**面向网易我的世界 Mod Python 2 脚本**的项目分析能力，并将其拆分为：

1. **通用核心分析层**：负责解析、建模、依赖图、容错与说明生成。
2. **MCP Tool 暴露层**：仅在**非 server 环境**注册两个工具。

额外约束：

- 一个行为包中**可以同时存在多个 mod Python 包**；每个 `xxx/modMain.py` 都应视为一个独立 mod 包入口。
- 全量项目分析 tool 的输入是**行为包路径**，工具应自动检索其下所有 `modMain.py` 并分别递归分析。
- 指定 Python 文件/文件夹分析时，应自动向上解析作用域，但必须设置**明确的上溯边界**，避免错误路径导致无限向上搜索。
- Mod 脚本总体通常不超过 5MB，**不做更新检测与常驻缓存**，每次调用实时扫描。
- 即使存在**语法错误、不完整文件、混用 Python 2/伪 Python 写法**，仍要输出**模糊报告**，不能因单文件失败而整体失败。
- 分析输出目标不是“编译级正确性”，而是“帮助 AI 快速理解项目架构”。

---

## 2. 现有仓库接入点

### 2.1 Tool 注册接入点

- `src/app/server_runtime.cpp`
  - 当前统一在 `register_tools(...)` 内注册 MCP 工具。
  - 新工具建议以 `#ifndef MCDK_SERVER` 包裹，满足“仅在非 server 环境提供”。

### 2.2 Tool 实现风格参考

- `src/tools/register_search.hpp`
- `src/tools/register_jsonui.hpp`
- `src/tools/register_model.hpp`

建议复用当前风格：

- `mcp::tool_builder(...)`
- `with_description(...)`
- 参数定义使用 `with_string_param / with_number_param / with_boolean_param`
- 返回统一为 `{"content": [{"type":"text","text": ...}]}`

### 2.3 构建系统接入点

- 根 `CMakeLists.txt` 已统一编译 `src/*.cpp`
- 仓库已 vendored：
  - `libs/tree-sitter`
  - `libs/tree-sitter-python`

因此新增源码只需：

1. 在根 `CMakeLists.txt` 中显式 `add_subdirectory(libs/tree-sitter)`。
2. 为 Python 语法库提供独立 target。
3. 让主程序 target 链接 `tree-sitter` 与 Python grammar target。

---

## 3. Python 2 语法策略

## 3.1 现状判断

当前 `libs/tree-sitter-python/grammar.js` 已同时包含部分 Python 2 兼容规则，例如：

- `print_statement`
- `exec_statement`
- `<>`

但它同时也包含明显的 Python 3+ 语法：

- `await`
- `match_statement`
- `type_alias_statement`
- `nonlocal_statement`

这意味着**直接使用现成 grammar 会“过宽”**：

- 能解析 Python 2 目标代码，但会把部分不该属于 Py2 的语法也视为合法。
- 对“这是网易 Mod 的 Python 2 项目”这一上下文不够严格。

## 3.2 建议方案：维护一个本地 Py2 定制 grammar

建议新增：

- `libs/tree-sitter-python2/`

实现方式：

1. 以 `libs/tree-sitter-python` 为基础拷贝一份。
2. 修改 `grammar.js`，对齐用户给出的 Py2 grammar 目标。
3. 生成独立 `parser.c`，形成 `tree-sitter-python2` target。

### 需要保留的能力

- `print` 语句
- `exec` 语句
- `raise a, b, c`
- `except Exception, e`
- Py2 lambda / old_test 相关兼容形式
- 旧式列表/推导式语义

### 需要移除或禁用的能力

- `await`
- `async`
- `match`
- `type alias`
- `nonlocal`
- 仅 Py3 的 typing 风格节点

## 3.3 容错策略

无论 grammar 是否完全命中，都不能让工具因为解析失败而停止。通用核心层应采用三层容错：

1. **Tree-sitter 恢复树**
   - 接受 `ERROR` / `MISSING` 节点。
   - 继续提取可恢复的 import / class / def / assignment / call 结构。

2. **语法缺损补救扫描**
   - 对出错文件追加轻量文本回退：
     - 正则抓取 `import ...`
     - 正则抓取 `from ... import ...`
     - 正则抓取 `class X(` / `def x(`
   - 明确给结果打上 “fallback-derived” 标记。

3. **模糊结论输出**
   - 每份报告输出：
     - `confidence`
     - `parse_health`
     - `unresolved_imports`
     - `ambiguous_edges`
   - 让 AI 知道哪些推断是高置信，哪些只是可能。

---

## 4. 核心分析层与 Tool 层分离

## 4.1 推荐目录

建议新增目录：

```text
src/project_analysis/
  py2_language.hpp/.cpp           # tree-sitter parser/language 包装
  file_loader.hpp/.cpp            # 文件读取、编码/BOM/容错
  module_resolver.hpp/.cpp        # 路径 -> 模块名映射
  syntax_snapshot.hpp/.cpp        # 单文件 AST/错误摘要
  import_collector.hpp/.cpp       # import/from-import 提取
  symbol_collector.hpp/.cpp       # class/def/global 常量提取
  call_collector.hpp/.cpp         # 调用、属性访问的启发式提取
  project_index.hpp/.cpp          # 全项目索引
  dependency_graph.hpp/.cpp       # 正向/反向依赖图
  fuzzy_inference.hpp/.cpp        # 模糊推导与置信度
  architecture_formatter.hpp/.cpp # 面向 AI 的说明文案生成
```

Tool 层建议新增：

```text
src/tools/register_python_analysis.hpp
```

## 4.2 分层职责

### 通用核心层

输入：

- 项目根目录
- 目标路径
- 深度参数

输出：

- 文件级索引
- 模块图
- 反向依赖图
- 符号摘要
- 模糊边/未解析边
- 面向报告层的结构化 JSON/DTO

### Tool 层

负责：

- MCP 参数校验
- 路径规范化
- 调用核心分析 API
- 将结构化结果转成适合 LLM 阅读的文本
- 控制“输出篇幅”和“深度”

这样后续即使未来：

- 不走 MCP
- 改做 CLI
- 改做 server 内部诊断 API

都能复用核心逻辑。

---

## 5. 项目建模规则

## 5.1 文件发现

默认递归扫描：

- `*.py`
- 排除常见缓存/无关目录：`__pycache__`、`.git`、临时输出目录

## 5.2 模块命名规则

用户已指定关键约定：

- `行为包/xxxx/modMain.py` 会被游戏视为一个 mod 包，其包名为 `xxxx`
- 一个行为包中可能有多个这类包
- 某个 mod 包目录下的文件按 `xxxx.xxx` 解释

建议统一规则：

1. 如果文件路径命中 `<behavior_pack_root>/<mod_name>/modMain.py`
   - 记为入口模块：`<mod_name>.modMain`
   - 记为入口包：`<mod_name>`
   - 同时将 `<behavior_pack_root>/<mod_name>/` 视为该 mod 包的**Python 包作用域根**

2. 其他 `*.py`
   - 若位于某个已识别 mod 包作用域根下，则按该根目录计算相对模块名
   - 例如 `行为包/pymod/foo/bar.py -> pymod.foo.bar`
   - `__init__.py` 视为包模块本身

3. 对无 `__init__.py` 的目录
   - 仍允许作为“弱包”参与推断
   - 标记 `namespace_like = true`

4. 一个行为包下若发现多个 `modMain.py`
   - 应建立 `behavior_pack -> [mod_package_1, mod_package_2, ...]` 聚合关系
   - 全量报告先输出行为包级总览，再输出每个 mod 包的子报告

## 5.3 作用域判定与向上解析边界

对“指定 py 文件/文件夹分析”场景，建议采用**有限上溯 + 双锚点判定**：

1. 从 `target_path` 开始逐级向上查找。
2. 优先寻找最近的 `modMain.py` 所在父目录：
   - 若某级目录下存在 `modMain.py`
   - 则该目录视为**mod 包根目录**，其目录名即 mod 名
3. 继续向上最多再找一个行为包边界：
   - 若上层某级目录存在 `manifest.json`
   - 则该目录视为**行为包根目录**
4. 停止条件：
   - 到达文件系统根
   - 到达用户传入的扫描上限根目录
   - 上溯层数超过固定阈值（建议 12）
   - 已识别到 `manifest.json` 且其下包含当前 mod 包

推荐最优解：

- **文件/文件夹定向分析的主作用域使用最近的 mod 包根目录**。
- **若同时发现行为包根目录，则把它作为“完整上下文作用域”用于补充其他 mod 包关系，但不强制展开全部深度依赖**。

这样可以兼顾：

- 小目标分析时不至于把整个行为包都算进来
- 需要全局关联时又能知道该目标属于哪个行为包
- 不会出现错误路径一直向上扫到磁盘根目录的问题

## 5.4 依赖边类型

建议至少记录以下边：

- `import`：`import a.b`
- `from_import`：`from a import b`
- `dynamic_hint`：字符串/反射/`__import__` 推断
- `call_hint`：跨模块调用启发式
- `inherit`：类继承关系
- `entrypoint`：由 `modMain.py` 指向核心模块

每条边带：

- `confidence`：0~1
- `source_kind`：ast / fallback_regex / inferred
- `evidence`：源文件、行号、片段

---

## 6. 两个 MCP Tool 设计

## 6.1 Tool A：按行为包路径生成架构说明

### 名称建议

- `analyze_py2_behavior_pack_architecture`

### 注册范围

- 仅非 `MCDK_SERVER`

### 输入参数建议

- `behavior_pack_path: string`  行为包根目录绝对路径
- `depth: number = 2`
- `include_symbols: boolean = true`
- `include_unresolved: boolean = true`
- `static_third_party_packages: string[] = ["QuModLibs"]`

### depth 语义建议

- `0`：仅入口包与顶层模块摘要
- `1`：加入直接 import 关系
- `2`：加入二级依赖与关键类/函数
- `3`：加入较完整的跨模块链路与模糊推断
- `4+`：尽量完整输出，适合小项目全量理解

### 输出目标

生成“供 AI 阅读的架构说明”，结构建议：

1. **行为包概览**
   - 发现多少 mod 包 / 模块 / 可疑损坏文件
2. **入口模块与启动链**
   - 哪些 `modMain.py` 是入口
   - 每个入口属于哪个 mod 包
   - 各入口导入哪些核心模块
3. **模块分层摘要**
   - API 层 / 系统层 / UI 层 / 工具层 / 数据层（基于命名启发式）
4. **关键依赖图说明**
5. **主要类/函数职责摘要**
6. **模糊点与风险**
   - 动态导入
   - 解析错误
   - 未解析 import
   - 静态三方库引用

### 返回风格建议

以文本为主，同时内部保留结构化 JSON DTO 以便未来扩展。

---

## 6.2 Tool B：按包/文件分析被引用与间接引用关系

### 名称建议

- `analyze_py2_reference_architecture`

### 输入参数建议

- `target_path: string` 目标包目录或目标 `.py` 文件绝对路径
- `depth: number = 2`
- `reverse_only: boolean = false`
- `include_indirect: boolean = true`
- `include_call_hints: boolean = true`
- `max_scope_upward_levels: number = 12`
- `static_third_party_packages: string[] = ["QuModLibs"]`

### 分析目标

围绕目标对象输出：

1. **谁直接引用它**
2. **谁通过链路间接引用它**
3. **它在整体架构中属于什么角色**
4. **如果它损坏/修改，可能影响哪些模块**

作用域自动判定：

- 若向上找到最近 `modMain.py`：以该 mod 包作为主分析作用域
- 若进一步找到 `manifest.json`：将该行为包作为补充上下文作用域
- 若两者都找不到：退化为“仅目标路径局部分析”，并在报告中标明作用域不完整

### target_path 解析规则

- 如果是文件：按单模块分析
- 如果是文件夹：聚合该目录下所有模块为一个“包目标”

### 输出结构建议

1. 目标摘要
2. 直接引用者列表
3. 间接引用链（受 `depth` 控制）
4. 高频入口链路
5. 关键证据片段
6. 置信度与歧义说明

---

## 7. 模糊报告设计

这是本项目区别于“严格静态分析器”的关键。

## 7.1 文件级健康度

每个文件计算：

- `parse_ok`
- `error_node_count`
- `missing_node_count`
- `fallback_used`
- `confidence`
- `scope_resolution_mode`（mod-package / behavior-pack / local-only）

建议健康度规则：

- **A**：完整解析，边高可信
- **B**：少量错误，但 import / class / def 可恢复
- **C**：明显损坏，仅能部分提取结构
- **D**：只能从文本模式做极弱推断

## 7.2 边级置信度

- AST 明确 import：`0.95`
- AST 明确 from-import：`0.90`
- 通过 alias + call 推断：`0.65`
- 文本正则回退：`0.55`
- 动态字符串猜测：`0.35`
- 项目外静态三方库引用：`0.80`（仅确认“引用到该库”，不做库内深度图）
- 标准库/未知外部库名匹配：`0.50`

## 7.3 项目外引用处理

对于**项目外引用**，不做深度依赖计算，只提供模糊调用信息：

1. **Python 标准库**
   - 识别为 external-stdlib
   - 仅输出“哪些模块导入了它、可能用于什么用途”

2. **静态三方库**
   - 建议内置静态包名表，例如：`QuModLibs`
   - 若 import 命中这些名字：
     - 记为 external-third-party-static
     - 不递归解析其内部模块
     - 只在报告中输出调用入口与使用分布

3. **未知外部包**
   - 记为 unresolved-external
   - 只保留证据，不展开间接引用链

## 7.4 输出中的措辞分级

- 高置信：`可以确认`
- 中置信：`较可能`
- 低置信：`推测` / `可能存在`

要求报告文本显式区分，不要把模糊结论伪装成确定事实。

---

## 8. 架构说明文案生成策略

目标不是简单罗列 AST，而是生成对 AI 更友好的“项目理解说明”。

建议 `architecture_formatter` 生成以下风格：

1. **一句话结论**
   - 例如：该行为包以 `pymod.modMain` 为入口，整体呈现“入口注册 -> 系统服务 -> 功能模块”的分层结构。

2. **分段说明**
   - 入口
   - 核心模块
   - 依赖链
   - 风险与模糊点

3. **避免噪音**
   - 深度较低时不展开全部函数
   - 优先展示高入度/高出度模块

4. **适合 AI 二次推理**
   - 尽量输出“模块职责 + 证据”
   - 例如：
     - `xxx.server.system`：疑似系统注册层（被 6 个功能模块导入，且包含 `RegisterSystem` 调用）
5. **外部依赖单列输出**
   - 将标准库、`QuModLibs`、未知三方库单独列为“项目外依赖”
   - 避免与项目内模块混合，误导 AI 认为它们属于同一行为包内部结构

---

## 9. 实施阶段建议

## Phase 1：底座接入

1. 接入 `tree-sitter` 编译
2. 新增 `tree-sitter-python2` grammar target
3. 封装 parser 生命周期与 UTF-8 文本输入

交付标准：

- 能对单个 Py 文件输出 AST 健康摘要

## Phase 2：单文件结构提取

1. 提取 imports
2. 提取 class / def
3. 提取主要调用/赋值启发式
4. 接入错误节点统计

交付标准：

- 能对损坏文件输出“部分可用”的结构摘要

## Phase 3：全项目索引与依赖图

1. 扫描项目所有 `.py`
2. 构建模块表
3. 构建正向/反向依赖图
4. 支持 target package/file 聚合

交付标准：

- 能输出直接引用与间接引用链

## Phase 4：架构说明生成

1. 输出 AI 友好文本
2. 加入分层推断
3. 加入模糊结论与风险提示

交付标准：

- 报告可直接作为 AI 上下文输入

## Phase 5：MCP Tool 集成

1. 注册两个 tool
2. 非 server 环境开关
3. 参数校验与错误提示

交付标准：

- 本地 MCP 可直接调用分析工具

---

## 10. 推荐代码落点

### 新增文件

```text
src/project_analysis/py2_language.hpp
src/project_analysis/py2_language.cpp
src/project_analysis/file_loader.hpp
src/project_analysis/file_loader.cpp
src/project_analysis/module_resolver.hpp
src/project_analysis/module_resolver.cpp
src/project_analysis/syntax_snapshot.hpp
src/project_analysis/syntax_snapshot.cpp
src/project_analysis/import_collector.hpp
src/project_analysis/import_collector.cpp
src/project_analysis/symbol_collector.hpp
src/project_analysis/symbol_collector.cpp
src/project_analysis/call_collector.hpp
src/project_analysis/call_collector.cpp
src/project_analysis/project_index.hpp
src/project_analysis/project_index.cpp
src/project_analysis/dependency_graph.hpp
src/project_analysis/dependency_graph.cpp
src/project_analysis/fuzzy_inference.hpp
src/project_analysis/fuzzy_inference.cpp
src/project_analysis/architecture_formatter.hpp
src/project_analysis/architecture_formatter.cpp
src/tools/register_python_analysis.hpp
```

### 需要修改的现有文件

- `CMakeLists.txt`
- `src/app/server_runtime.cpp`

---

## 11. 风险与对应策略

## 风险 1：Py2/网易脚本存在大量非标准写法

策略：

- 不追求“编译器级正确”
- 采用恢复树 + 文本回退 + 模糊报告

## 风险 2：动态导入普遍存在

策略：

- 将动态导入独立标记为 `dynamic_hint`
- 不把它当成确定依赖

## 风险 3：`tree-sitter-python` 改造成纯 Py2 grammar 成本较高

策略：

- 第一版可先以当前 grammar 为底，配合“Py3 特性告警/降权”快速出原型
- 第二版再落地 `tree-sitter-python2` 定制 grammar

## 风险 4：报告过长影响 AI 读取效率

策略：

- 用 `depth` 控制展开层数
- 默认只展示高价值模块与关键链路

---

## 12. 最终建议

推荐执行路径：

1. **先做通用核心层**，不要把逻辑直接写进 MCP tool。
2. **先做容错与模糊报告机制**，这比“严格 Py2 语法纯度”更影响实际可用性。
3. **第一阶段允许基于现有 `tree-sitter-python` 快速原型**，但要把 Py3-only 节点识别为“非目标语法/低可信特性”。
4. **第二阶段再收敛到独立 `tree-sitter-python2` grammar**，提升判定一致性。
5. Tool 注册放在非 server 环境，符合当前仓库的能力边界与安全边界。

这个路线能以最小改动先得到“可用的 AI 架构理解工具”，再逐步提升 Py2 纯度与分析精度。
