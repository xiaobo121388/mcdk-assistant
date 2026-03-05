# 基岩版 JSON 模型 MCP 工具设计方案（Phase 3）

> **后期扩展方向**：Phase 4 动画开发（animations.json）将基于本工具集提供的骨骼信息（pivot + parent层级）进行 keyframe 编写辅助。

## 目标

让 AI 具备**理解**和**高效操作**基岩版 JSON 模型（`geometry.json`）的能力，等价于一位熟练的 Blockbench 操作者：

| 能力类比 | MC JSON 模型等价能力 |
|---|---|
| 打开/查看模型 | 解析 geometry.json 结构，报告骨骼树、cube 数量、贴图尺寸 |
| 添加/删除骨骼 | `model_add_bone` / `model_remove_bone` |
| 添加/编辑 cube | `model_add_cube` / `model_edit_cube` |
| 调整 pivot/rotation | `model_set_bone_transform` |
| UV 编辑 | `model_set_uv` |
| 镜像/对称操作 | `model_mirror_bone` |
| 保存模型 | `model_save` |
| 语法参考手册 | `get_model_reference` |

---

## 基岩版 JSON 模型格式速览

```
geometry.json (format_version: 1.12.0 / 1.16.0)
└── minecraft:geometry[]
    └── description
    │     identifier: "geometry.xxx"
    │     texture_width: 64
    │     texture_height: 64
    │     visible_bounds_width / height / offset
    └── bones[]
          name: string
          parent?: string
          pivot: [x, y, z]         # 旋转中心（MC坐标系：Y轴向上）
          rotation?: [x, y, z]
          mirror?: bool             # UV镜像
          cubes[]
            origin: [x, y, z]      # 左下前角
            size:   [x, y, z]
            inflate?: float         # 膨胀/收缩
            uv: [u, v]             # 简单UV（整数）
            # 或 per-face UV:
            uv:
              north/south/east/west/up/down:
                uv: [u, v]
                uv_size: [w, h]
```

### 关键坐标系约定
- MC坐标系：Y轴向上，Z轴向玩家
- `origin` = cube 最小角（左-下-前）
- `pivot` = 骨骼旋转轴心，子骨骼继承父骨骼的变换

---

## 工具设计（Phase 3）

### Phase 3a：`get_model_reference` — 语法速查手册（无参数，硬编码）

**定位**：让 AI 随时获取 geometry.json 完整语法规范，类似调用 `get_netease_diff`。

**返回内容**（精炼版，约200行）：
```
一、文件格式版本
  1.12.0 / 1.16.0 的差异（per-face UV 支持）
  format_version 选择建议（网易版兼容性）

二、description 字段速查
  identifier 命名规则（geometry.xxx.yyy）
  texture_width / texture_height
  visible_bounds_width / height / offset

三、bones 骨骼系统
  parent 层级关系（树形结构）
  pivot [x,y,z] — 旋转轴心
  rotation [x,y,z] — 默认旋转
  mirror — UV水平镜像
  binding — 骨骼绑定（高级）

四、cubes 立方体
  origin / size / inflate
  简单UV vs per-face UV 完整对比
  UV 边界计算公式

五、locators 定位点
  用途：特效/骨骼挂接点
  格式：{ "name": [x, y, z] }

六、网易版特殊注意事项
  format_version 推荐 1.12.0（兼容性最高）
  与 render_controller 的配合方式
  骨骼 binding 与 ModelComp API 的关系

七、常见错误
  骨骼循环引用
  UV超出贴图边界
  cube size 小于1导致贴图黑块
```

---

### Phase 3b：`model_parse` — 解析模型结构

**参数**：
```
path: string  # 绝对路径（.geo.json 或 .json）
```

**返回**（摘要文本）：
```
=== geometry.custom_sword ===
贴图尺寸: 64x32
骨骼树:
  root (pivot: 0,0,0)
  └── blade (pivot: 0,8,0)  — 3 cubes
  └── guard (pivot: 0,4,0)  — 1 cube
  └── handle (pivot: 0,0,0) — 2 cubes
总 cubes: 6
```

**用途**：AI先解析现有模型，再决定如何修改。

---

### Phase 3c：`model_get_bone` — 获取单骨骼详情

**参数**：
```
path: string       # 模型文件绝对路径
bone_name: string  # 骨骼名称
```

**返回**：完整 JSON 骨骼定义（含所有 cubes 的详细数据），供 AI 读懂再修改。

---

### Phase 3d：`model_op` — 核心增删改操作（高效 OP）

> **设计哲学**：单个工具处理一类 CRUD，用 `op` 字段区分操作类型，减少工具数量、降低 AI 调用成本。

**工具名**：`model_op`

**通用参数**：
```
path: string   # 模型文件绝对路径（操作完立即写回）
op: string     # 操作类型（见下表）
```

#### 操作一览

| op 值 | 功能 | 额外参数 |
|---|---|---|
| `add_bone` | 新增骨骼 | `bone_name`, `parent?`, `pivot`, `rotation?` |
| `remove_bone` | 删除骨骼（级联删子骨骼） | `bone_name`, `cascade?=true` |
| `rename_bone` | 重命名骨骼 | `bone_name`, `new_name` |
| `set_bone_transform` | 设置骨骼 pivot/rotation | `bone_name`, `pivot?`, `rotation?` |
| `add_cube` | 向骨骼添加 cube | `bone_name`, `origin`, `size`, `uv`, `inflate?` |
| `remove_cube` | 按索引删除 cube | `bone_name`, `cube_index` |
| `edit_cube` | 修改 cube 的任意字段 | `bone_name`, `cube_index`, `origin?`, `size?`, `uv?`, `inflate?` |
| `mirror_bone` | 生成对称骨骼（X轴镜像） | `bone_name`, `new_name`, `mirror_uv?=true` |
| `set_texture_size` | 修改贴图尺寸 | `width`, `height` |
| `add_locator` | 添加定位点 | `bone_name`, `locator_name`, `position` |

**写回策略**：操作成功后立即将修改后的 JSON 写回原文件，返回操作摘要。

**返回示例**：
```
OK: added cube to bone 'blade' (index=3)
  origin: [-1, 8, -1], size: [2, 8, 2], uv: [0, 0]
```

---

### Phase 3e：`model_mirror_symmetry` — 快速对称建模

**参数**：
```
path: string        # 模型文件
source_bone: string # 源骨骼（如 left_arm）
target_bone: string # 目标骨骼名（如 right_arm）
axis: string        # 对称轴，默认 "X"
mirror_uv: bool     # 是否镜像UV，默认 true
```

**功能**：一键把左臂的所有 cube 镜像复制到右臂，并自动修正 origin.x（`new_x = -origin.x - size.x`）。这是最高频的角色建模操作之一。

---

### Phase 3f：`model_create_from_template` — 快速创建模板模型

**参数**：
```
path: string          # 保存路径
identifier: string    # 几何体ID（如 geometry.my_sword）
template: string      # 模板类型（见下表）
texture_width: int    # 默认 64
texture_height: int   # 默认 64
namespace?: string    # 命名空间前缀
```

**模板类型**：

| template | 说明 | 初始骨骼 |
|---|---|---|
| `blank` | 空模型 | root |
| `humanoid` | 人形骨骼框架 | head/body/left_arm/right_arm/left_leg/right_leg/waist |
| `sword` | 剑/近战武器 | root/blade/guard/handle |
| `item_flat` | 平面物品（类似原版物品） | root/item |
| `quadruped` | 四足生物基础框架 | head/body/leg0~3/tail |

**用途**：AI 无需从零手写 JSON，直接生成骨骼框架再精细调整。

---

## 系统架构

```
src/
└── tools/
    ├── register_model.hpp         # 工具注册（新增）
    └── model_editor.hpp           # 模型编辑核心逻辑（新增）

        ModelEditor
        ├── load(path)             → nlohmann::json
        ├── save(path, json)       → bool
        ├── parse_summary(json)    → string
        ├── get_bone(json, name)   → json&
        ├── add_bone(...)
        ├── remove_bone(...)
        ├── add_cube(...)
        ├── edit_cube(...)
        ├── mirror_bone(...)
        └── create_template(...)   → json
```

**依赖**：仅 `nlohmann/json`（已有），无新增外部依赖。

---

## 与现有系统的集成

```cpp
// src/main.cpp 新增一行
#include "tools/register_model.hpp"
// ...
mcdk::register_model_tools(srv);
```

---

## AI 工作流示例

### 场景1：从零制作一把剑的模型

```
1. model_create_from_template(
       path="D:/mod/RP/models/entity/sword.geo.json",
       identifier="geometry.my_sword",
       template="sword",
       texture_width=32, texture_height=32
   )
2. model_op(op="add_cube", bone_name="blade",
       origin=[-0.5, 8, -0.5], size=[1, 14, 1], uv=[0, 0])
3. canvas_new(width=32, height=32)   # 配合像素画工具绘制贴图
4. model_parse(path="...")           # 验证结构
```

### 场景2：修改现有生物模型

```
1. model_parse(path="...")           # 读懂结构
2. model_get_bone(bone_name="head")  # 精确查看头部所有cube
3. model_op(op="add_cube", ...)      # 添加装饰
4. model_mirror_symmetry(...)        # 对称到另一侧
```

### 场景3：调试UV错位

```
1. model_get_bone(bone_name="body")  # 查看当前UV值
2. model_op(op="edit_cube",
       cube_index=0, uv=[16, 0])     # 修正UV
3. get_model_reference()             # 查阅UV计算公式
```

---

## 实现优先级

| 阶段 | 工具 | 实现复杂度 | 价值 |
|---|---|---|---|
| **P0**（必做） | `get_model_reference` | ⭐ 极低（硬编码文本） | ⭐⭐⭐⭐⭐ |
| **P0**（必做） | `model_parse` | ⭐⭐ 低 | ⭐⭐⭐⭐⭐ |
| **P0**（必做） | `model_op`（核心op） | ⭐⭐⭐ 中 | ⭐⭐⭐⭐⭐ |
| **P1** | `model_get_bone` | ⭐ 极低 | ⭐⭐⭐⭐ |
| **P1** | `model_mirror_symmetry` | ⭐⭐ 低 | ⭐⭐⭐⭐ |
| **P2** | `model_create_from_template` | ⭐⭐ 低 | ⭐⭐⭐⭐ |
| **P2** | `model_mirror_symmetry` 扩展 | ⭐⭐ 低 | ⭐⭐⭐ |

---

## 文件新增清单

```
src/tools/model_editor.hpp      # 核心编辑逻辑（纯头文件，~400行）
src/tools/register_model.hpp    # MCP工具注册（~200行）
```

修改：
```
src/main.cpp                    # 新增 #include 和 register_model_tools(srv)
```
