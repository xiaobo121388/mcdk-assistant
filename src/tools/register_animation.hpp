#pragma once
// register_animation.hpp — 基岩版 animations.json MCP 工具注册
// 工具列表:
//   get_animation_reference    — 语法速查手册（无参数）
//   animation_parse            — 解析动画文件，输出所有动画摘要
//   animation_get_bone_channel — 获取指定骨骼通道的完整 keyframe 数据
//   anim_op                    — 核心增删改操作（支持批量 ops 数组）
#include "tools/animation_editor.hpp"
#include <mcp_server.h>
#include <mcp_tool.h>
#include <mcp_message.h>
#include <nlohmann/json.hpp>
#include <string>
#include <sstream>

namespace mcdk {

// ── 辅助：标准返回格式 ────────────────────────────────────
static inline mcp::json anim_ok(const std::string& text) {
    return {{"content", mcp::json::array({{{"type","text"},{"text", text}}})}};
}
static inline mcp::json anim_err(const std::string& text) {
    return {{"content", mcp::json::array({{{"type","text"},{"text", "[ERROR] " + text}}})}};
}

// ── 动画语法速查手册 ──────────────────────────────────────
static const char* ANIMATION_REFERENCE_TEXT = R"(
=== 基岩版 animations.json 全栈速查手册 ===

一、文件格式
  {
    "format_version": "1.8.0",   ← 推荐（兼容性最佳）
    "animations": {
      "animation.xxx.yyy": { ... },
      ...
    }
  }
  命名规范: 必须以 "animation." 开头，只能小写+点+下划线

二、动画对象属性
  loop                bool | "hold_on_last_frame"
                      — true: 循环播放（走路/待机）
                      — false: 播放一次
                      — "hold_on_last_frame": 播放后停在最后一帧（睡眠/死亡）
  animation_length    float  — 动画总时长（秒），不填则自动取最大keyframe时间
  blend_weight        float | molang — 混合权重（0=不影响，1=完全覆盖）
  override_previous_animation bool — 是否覆盖前序动画
  anim_time_update    molang — 自定义时间推进表达式（默认=query.anim_time）
  start_delay         float  — 开始延迟（秒）
  loop_delay          float  — 循环延迟（秒）

三、bones 骨骼通道
  "bones": {
    "boneName": {
      "rotation": <channel_value>,
      "position": <channel_value>,
      "scale":    <channel_value>
    }
  }
  rotation 单位：度（°），基准=骨骼在 geometry.json 中的 rotation
  position 单位：模型空间格，叠加在 geometry.json pivot 基础上
  scale    单位：倍数（1.0=原始大小）

四、channel_value 通道值格式
  1. 常量值（数组）: [x, y, z]
     示例: "rotation": [0, -90, 0]

  2. 常量 Molang（数组内字符串）:
     "rotation": ["math.sin(q.anim_time * 180) * 30", 0, 0]

  3. Keyframe 对象:
     "rotation": {
       "0.0":  [0, 0, 0],
       "0.3":  [-30, 0, 0],
       "0.6":  [0, 0, 0]
     }
     键名为时间（秒，字符串），值为 [x,y,z] 或 molang 数组

  4. Easing Keyframe（带插值控制）:
     "0.3": {
       "pre":  [0, 0, 0],
       "post": [-30, 0, 0],
       "lerp_mode": "catmullrom"   ← linear(默认)/catmullrom
     }

五、常用 Molang 查询（动画中最常用）
  q.anim_time              — 当前动画时间（秒，0→animation_length循环）
  q.life_time              — 实体存活时间（秒，不受loop影响）
  q.modified_distance_moved — 实体移动的距离（适合走路动画速度同步）
  q.modified_move_speed    — 移动速度（0=静止）
  q.is_on_ground           — 是否在地面（bool）
  q.is_moving              — 是否移动中（bool）
  variable.attack_time     — 攻击动画时间（0→1）
  math.sin(x)              — 正弦，x单位是度
  math.cos(x)              — 余弦
  math.abs(x)              — 绝对值

六、走路动画经典 Molang 公式
  腿部左右摆动（基于移动距离，静止自动停）:
    rotation: ["math.sin(q.modified_distance_moved * 38.17) * 30 * q.modified_move_speed", 0, 0]
  
  手臂反向摆动（与腿相反）:
    rotation: ["-math.sin(q.modified_distance_moved * 38.17) * 30 * q.modified_move_speed", 0, 0]

  完美循环正弦（基于 anim_time，速度150对应时长2.4s）:
    rotation: ["math.sin(q.anim_time * 150) * 30", 0, 0]

七、实体动画引用（client_entity.json）
  "animations": {
    "walk": "animation.my_entity.walk",
    "attack": "animation.my_entity.attack"
  },
  "scripts": {
    "animate": [
      {"walk": "q.modified_move_speed > 0"},
      "attack"
    ]
  }

八、anim_op 工具支持的 op 类型
  add_anim           — 新建动画 (anim_name, loop?, loop_str?, animation_length?)
  remove_anim        — 删除动画 (anim_name)
  set_anim_prop      — 修改动画属性 (anim_name, prop, value)
  set_keyframe       — 设置 keyframe (anim_name, bone_name, channel, time, value)
  remove_keyframe    — 删除 keyframe (anim_name, bone_name, channel, time)
  set_constant       — 设置常量通道 (anim_name, bone_name, channel, value)
  remove_bone_channel— 删除骨骼通道 (anim_name, bone_name, channel?)
  mirror_bone_rotation— 镜像骨骼旋转 (anim_name, source_bone, target_bone, phase_offset?, anim_length?)
  scale_time         — 缩放时间轴 (anim_name, time_scale)
)";

// ── MCP 工具注册函数 ───────────────────────────────────────

inline void register_animation_tools(mcp::server& srv) {

    // ════════════════════════════════════════════════════════
    // 1. get_animation_reference — 语法速查手册
    // ════════════════════════════════════════════════════════
    srv.register_tool(
        mcp::tool_builder("get_animation_reference")
            .with_description("获取基岩版 animations.json 完整语法速查手册，包含骨骼系统、keyframe格式、Molang查询、走路动画公式、client_entity引用方法")
            .with_read_only_hint(true).with_idempotent_hint(true).build(),
        [](const mcp::json&, const std::string&) -> mcp::json {
            return anim_ok(ANIMATION_REFERENCE_TEXT);
        });

    // ════════════════════════════════════════════════════════
    // 2. animation_parse — 解析动画文件摘要
    // ════════════════════════════════════════════════════════
    srv.register_tool(
        mcp::tool_builder("animation_parse")
            .with_description("解析 animations.json，输出所有动画的摘要（loop/时长/骨骼通道/帧数）。"
                              "支持 file_path（文件绝对路径）或 json_content（文本内容）二选一。"
                              "可通过 filter 按名称关键词过滤动画。")
            .with_string_param("file_path",    "animations.json 的绝对路径（与 json_content 二选一）", false)
            .with_string_param("json_content", "直接传入 animations.json 文本内容（与 file_path 二选一）", false)
            .with_string_param("filter",       "按动画名称关键词过滤（可选，不填则显示全部）", false)
            .with_read_only_hint(true).build(),
        [](const mcp::json& p, const std::string&) -> mcp::json {
            try {
                auto root = anim_resolve(p);
                std::string filter = p.value("filter", "");
                return anim_ok(anim_parse_summary(root, filter));
            } catch (const std::exception& e) { return anim_err(e.what()); }
        });

    // ════════════════════════════════════════════════════════
    // 3. animation_get_bone_channel — 获取骨骼通道详情
    // ════════════════════════════════════════════════════════
    srv.register_tool(
        mcp::tool_builder("animation_get_bone_channel")
            .with_description("获取指定动画中某骨骼的完整 keyframe 数据（精确时间+值），"
                              "用于 AI 分析和修改前的精确理解。"
                              "支持 file_path 或 json_content 二选一。")
            .with_string_param("file_path",    "animations.json 的绝对路径（与 json_content 二选一）", false)
            .with_string_param("json_content", "直接传入文本内容（与 file_path 二选一）", false)
            .with_string_param("anim_name",    "动画名称，如 animation.zombie.walk", true)
            .with_string_param("bone_name",    "骨骼名称，如 leftLeg", true)
            .with_string_param("channel",      "通道名称: rotation/position/scale（不填则输出全部）", false)
            .with_read_only_hint(true).build(),
        [](const mcp::json& p, const std::string&) -> mcp::json {
            try {
                auto root     = anim_resolve(p);
                std::string anim_name = p.value("anim_name", "");
                std::string bone_name = p.value("bone_name", "");
                std::string channel   = p.value("channel", "");
                if (anim_name.empty()) return anim_err("必须提供 anim_name");
                if (bone_name.empty()) return anim_err("必须提供 bone_name");
                return anim_ok(anim_get_bone_channel(root, anim_name, bone_name, channel));
            } catch (const std::exception& e) { return anim_err(e.what()); }
        });

    // ════════════════════════════════════════════════════════
    // 4. anim_op — 核心增删改操作（支持批量 ops 数组）
    // ════════════════════════════════════════════════════════

    // exec_one_anim_op: 执行单个 op，返回结果字符串，失败抛出异常
    auto exec_one_anim_op = [](json& root, const json& src) -> std::string {
        // 内部辅助：从 src 解析 JSON 参数
        auto pj = [&src](const char* key) -> json {
            if (!src.contains(key) || src[key].is_null()) return json(nullptr);
            if (src[key].is_string()) {
                std::string s = src[key].get<std::string>();
                if (s.empty()) return json(nullptr);
                try { return json::parse(s, nullptr, true, true); } catch (...) {}
            }
            return src[key];
        };

        std::string op        = src.value("op", "");
        std::string anim_name = src.value("anim_name", "");

        if (op == "add_anim") {
            bool loop            = src.value("loop", true);
            std::string loop_str = src.value("loop_str", "");
            double anim_length   = src.contains("animation_length") && src["animation_length"].is_number()
                                   ? src["animation_length"].get<double>() : 0.0;
            if (anim_name.empty()) throw std::runtime_error("add_anim 需要 anim_name");
            return op_anim_add_anim(root, anim_name, loop, loop_str, anim_length);

        } else if (op == "remove_anim") {
            if (anim_name.empty()) throw std::runtime_error("remove_anim 需要 anim_name");
            return op_anim_remove_anim(root, anim_name);

        } else if (op == "set_anim_prop") {
            if (anim_name.empty()) throw std::runtime_error("set_anim_prop 需要 anim_name");
            std::string prop = src.value("prop", "");
            if (prop.empty()) throw std::runtime_error("set_anim_prop 需要 prop 参数");
            json value = pj("value");
            if (value.is_null()) throw std::runtime_error("set_anim_prop 需要 value 参数");
            json& anim = get_anim(root, anim_name);
            return op_anim_set_prop(anim, prop, value);

        } else if (op == "set_keyframe") {
            if (anim_name.empty()) throw std::runtime_error("set_keyframe 需要 anim_name");
            std::string bone_name = src.value("bone_name", "");
            std::string channel   = src.value("channel", "rotation");
            std::string time_str  = src.value("time", "");
            if (bone_name.empty()) throw std::runtime_error("set_keyframe 需要 bone_name");
            if (time_str.empty())  throw std::runtime_error("set_keyframe 需要 time（秒，如 '0.3'）");
            json value = pj("value");
            if (value.is_null()) throw std::runtime_error("set_keyframe 需要 value");
            json& anim = get_anim(root, anim_name);
            return op_anim_set_keyframe(anim, bone_name, channel, time_str, value);

        } else if (op == "remove_keyframe") {
            if (anim_name.empty()) throw std::runtime_error("remove_keyframe 需要 anim_name");
            std::string bone_name = src.value("bone_name", "");
            std::string channel   = src.value("channel", "rotation");
            std::string time_str  = src.value("time", "");
            if (bone_name.empty()) throw std::runtime_error("remove_keyframe 需要 bone_name");
            if (time_str.empty())  throw std::runtime_error("remove_keyframe 需要 time");
            json& anim = get_anim(root, anim_name);
            return op_anim_remove_keyframe(anim, bone_name, channel, time_str);

        } else if (op == "set_constant") {
            if (anim_name.empty()) throw std::runtime_error("set_constant 需要 anim_name");
            std::string bone_name = src.value("bone_name", "");
            std::string channel   = src.value("channel", "rotation");
            if (bone_name.empty()) throw std::runtime_error("set_constant 需要 bone_name");
            json value = pj("value");
            if (value.is_null()) throw std::runtime_error("set_constant 需要 value");
            json& anim = get_anim(root, anim_name);
            return op_anim_set_constant(anim, bone_name, channel, value);

        } else if (op == "remove_bone_channel") {
            if (anim_name.empty()) throw std::runtime_error("remove_bone_channel 需要 anim_name");
            std::string bone_name = src.value("bone_name", "");
            std::string channel   = src.value("channel", "");
            if (bone_name.empty()) throw std::runtime_error("remove_bone_channel 需要 bone_name");
            json& anim = get_anim(root, anim_name);
            return op_anim_remove_bone_channel(anim, bone_name, channel);

        } else if (op == "mirror_bone_rotation") {
            if (anim_name.empty()) throw std::runtime_error("mirror_bone_rotation 需要 anim_name");
            std::string src_bone  = src.value("source_bone", "");
            std::string dst_bone  = src.value("target_bone", "");
            double phase_offset   = src.contains("phase_offset") && src["phase_offset"].is_number()
                                    ? src["phase_offset"].get<double>() : 0.0;
            double anim_length    = src.contains("anim_length") && src["anim_length"].is_number()
                                    ? src["anim_length"].get<double>() : 0.0;
            if (src_bone.empty()) throw std::runtime_error("mirror_bone_rotation 需要 source_bone");
            if (dst_bone.empty()) throw std::runtime_error("mirror_bone_rotation 需要 target_bone");
            json& anim = get_anim(root, anim_name);
            return op_anim_mirror_bone_rotation(anim, src_bone, dst_bone, phase_offset, anim_length);

        } else if (op == "scale_time") {
            if (anim_name.empty()) throw std::runtime_error("scale_time 需要 anim_name");
            double time_scale = src.contains("time_scale") && src["time_scale"].is_number()
                                ? src["time_scale"].get<double>() : 1.0;
            json& anim = get_anim(root, anim_name);
            return op_anim_scale_time(anim, time_scale);

        } else {
            throw std::runtime_error("未知 op: " + op +
                "  有效值: add_anim/remove_anim/set_anim_prop/set_keyframe/remove_keyframe/"
                "set_constant/remove_bone_channel/mirror_bone_rotation/scale_time");
        }
    };

    srv.register_tool(
        mcp::tool_builder("anim_op")
            .with_description(
                "对 animations.json 执行增删改操作，操作完立即写回文件。\n"
                "必须提供 file_path（绝对路径）。\n"
                "【批量模式】传 ops（JSON数组），每项含 op 字段及对应参数，全部执行后一次写回文件。\n"
                "【单次模式】直接传 op + 参数（向后兼容）。\n"
                "op 可选值:\n"
                "  add_anim           — 新建动画 (anim_name, loop?=true, loop_str?, animation_length?)\n"
                "  remove_anim        — 删除动画 (anim_name)\n"
                "  set_anim_prop      — 修改动画属性 (anim_name, prop, value)\n"
                "    props: loop/animation_length/blend_weight/override_previous_animation/anim_time_update/start_delay/loop_delay\n"
                "  set_keyframe       — 设置keyframe (anim_name, bone_name, channel='rotation', time, value)\n"
                "    value: JSON数组字符串如'[0,-30,0]'或molang数组'[\"math.sin(q.anim_time*180)*30\",0,0]'\n"
                "  remove_keyframe    — 删除keyframe (anim_name, bone_name, channel, time)\n"
                "  set_constant       — 设置常量通道 (anim_name, bone_name, channel, value)\n"
                "  remove_bone_channel— 删除骨骼/通道 (anim_name, bone_name, channel?)\n"
                "  mirror_bone_rotation— 镜像旋转 (anim_name, source_bone, target_bone, phase_offset?=0, anim_length?)\n"
                "  scale_time         — 缩放时间轴 (anim_name, time_scale)")
            .with_string_param("file_path",   "animations.json 的绝对路径（必须）", true)
            // 批量模式
            .with_string_param("ops",         "批量操作数组（JSON字符串），每项含op字段，与单次op二选一", false)
            // 单次模式
            .with_string_param("op",          "操作类型（见描述）", false)
            // 公共参数
            .with_string_param("anim_name",   "动画名称，如 animation.zombie.walk", false)
            .with_string_param("bone_name",   "骨骼名称", false)
            .with_string_param("channel",     "通道: rotation/position/scale（默认rotation）", false)
            .with_string_param("time",        "keyframe 时间（秒，字符串如 '0.3'）", false)
            .with_string_param("value",       "通道值（JSON数组字符串或molang字符串）", false)
            .with_string_param("prop",        "动画属性名（set_anim_prop用）", false)
            .with_boolean_param("loop",       "是否循环（add_anim用，默认true）", false)
            .with_string_param("loop_str",    "loop字符串值，如 hold_on_last_frame（add_anim用）", false)
            .with_number_param("animation_length", "动画时长（秒）", false)
            .with_string_param("source_bone", "源骨骼（mirror_bone_rotation用）", false)
            .with_string_param("target_bone", "目标骨骼（mirror_bone_rotation用）", false)
            .with_number_param("phase_offset","相位偏移（秒，mirror_bone_rotation用，默认0）", false)
            .with_number_param("anim_length", "动画长度用于相位取模（mirror_bone_rotation用）", false)
            .with_number_param("time_scale",  "时间缩放倍数（scale_time用）", false)
            .build(),
        [exec_one_anim_op](const mcp::json& p, const std::string&) -> mcp::json {
            try {
                std::string fpath = p.value("file_path", "");
                if (fpath.empty()) return anim_err("anim_op 必须提供 file_path（绝对路径）");

                // 加载文件（如果不存在则创建新文件）
                json root;
                try {
                    root = anim_load_file(fpath);
                } catch (...) {
                    // 文件不存在时自动创建
                    root = anim_create_file();
                }

                std::ostringstream out;

                bool has_ops = p.contains("ops") && !p["ops"].is_null();
                if (has_ops) {
                    // 批量模式
                    json ops_arr;
                    if (p["ops"].is_string()) {
                        ops_arr = json::parse(p["ops"].get<std::string>(), nullptr, true, true);
                    } else {
                        ops_arr = p["ops"];
                    }
                    if (!ops_arr.is_array())
                        return anim_err("ops 必须是 JSON 数组，每项含 op 字段");

                    int n = static_cast<int>(ops_arr.size());
                    for (int i = 0; i < n; ++i) {
                        out << "[" << (i+1) << "/" << n << "] ";
                        out << exec_one_anim_op(root, ops_arr[i]) << "\n";
                    }
                } else {
                    // 单次模式
                    std::string op = p.value("op", "");
                    if (op.empty()) return anim_err("请提供 op（单次）或 ops（批量）参数");
                    out << exec_one_anim_op(root, p) << "\n";
                }

                anim_save_file(fpath, root);
                out << "[已写回文件: " << fpath << "]";
                return anim_ok(out.str());

            } catch (const std::exception& e) { return anim_err(e.what()); }
        });

} // register_animation_tools

} // namespace mcdk
