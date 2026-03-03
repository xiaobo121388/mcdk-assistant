#include "tools/ui_templates.h"
#include <algorithm>
#include <cctype>

namespace mcdk {

// 辅助：将 namespace 转为小写下划线风格的 Python 类名前缀
static std::string to_snake(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == ' ' || c == '-') r += '_';
        else r += (char)std::tolower((unsigned char)c);
    }
    return r;
}

// 辅助：将 snake_case 转为 PascalCase
static std::string to_pascal(const std::string& s) {
    std::string r;
    bool up = true;
    for (char c : s) {
        if (c == '_' || c == '-' || c == ' ') { up = true; continue; }
        r += up ? (char)std::toupper((unsigned char)c) : c;
        up = false;
    }
    return r;
}

// ── screen_basic ──────────────────────────────────────
static UiTemplateResult gen_screen_basic(const std::string& ns, const std::string& mod) {
    std::string cls = to_pascal(mod) + "ScreenNode";
    std::string snake = to_snake(mod);

    std::string json = R"({
  "namespace": ")" + ns + R"(",
  "main@netease_editor_template_namespace.netease_editor_root_panel": {
    "controls": [
      { "bg": { "type": "image", "size": ["100%", "100%"], "texture": "textures/ui/bg32", "alpha": 0.6, "layer": 0 } },
      {
        "panel": {
          "type": "panel", "size": ["80%", "80%"], "layer": 1,
          "controls": [
            {
              "workPanel": {
                "type": "panel", "size": ["100%", "100%"],
                "controls": [
                  {
                    "topPanel": {
                      "type": "stack_panel", "orientation": "horizontal",
                      "size": ["100%", 30], "anchor_from": "top_middle", "anchor_to": "top_middle",
                      "controls": [
                        { "label": { "type": "label", "text": ")" + ns + R"(", "color": [1,1,1], "font_scale_factor": 1.2, "size": ["fill", 30] } },
                        {
                          "closeBtn@common.button": {
                            "size": [20, 20],
                            "$pressed_button_name": "button.close",
                            "controls": [
                              { "icon": { "type": "image", "size": [16, 16], "texture": "textures/ui/cancel" } }
                            ]
                          }
                        }
                      ]
                    }
                  },
                  {
                    "content": {
                      "type": "panel",
                      "size": ["100%", "100%-30px"],
                      "anchor_from": "bottom_middle", "anchor_to": "bottom_middle"
                    }
                  }
                ]
              }
            }
          ]
        }
      }
    ]
  }
})";

    std::string py = R"(# -*- coding: utf-8 -*-
from mod.client.ui.screenNode import ScreenNode
import mod.client.extraClientApi as clientApi

def Register)" + cls + R"(():
    clientApi.RegisterUI(")" + mod + R"(", ")" + snake + R"(_screen", ")" + mod + R"(.ui.)" + cls + R"(." + cls + R"(", ")" + ns + R"(.main")

class )" + cls + R"((ScreenNode):
    def __init__(self, namespace, name, param):
        ScreenNode.__init__(self, namespace, name, param)

    def Create(self):
        self.AddTouchEventHandler("/panel/workPanel/topPanel/closeBtn", self.OnCloseButton)

    def OnCloseButton(self, args):
        touchEvent = args["TouchEvent"]
        if touchEvent == 0:
            self.SetScreenVisible(False)
)";

    return {json, py, "\"ui/" + snake + "_screen.json\"", ""};
}

// ── screen_list ───────────────────────────────────────
static UiTemplateResult gen_screen_list(const std::string& ns, const std::string& mod) {
    std::string cls = to_pascal(mod) + "ListScreenNode";
    std::string snake = to_snake(mod);

    std::string json = R"({
  "namespace": ")" + ns + R"(",
  "list_item": {
    "type": "panel", "size": ["100%", 40],
    "controls": [
      { "text": { "type": "label", "text": "#item_text", "color": [1,1,1], "size": ["100%", "100%"],
        "bindings": [{ "binding_name": "#item_text" }] } }
    ]
  },
  "main@netease_editor_template_namespace.netease_editor_root_panel": {
    "controls": [
      { "bg": { "type": "image", "size": ["100%", "100%"], "texture": "textures/ui/bg32", "alpha": 0.6, "layer": 0 } },
      {
        "panel": {
          "type": "panel", "size": ["80%", "80%"], "layer": 1,
          "controls": [
            {
              "scroll@common.scrolling_panel": {
                "size": ["100%", "100%"],
                "$scrolling_content": ")" + ns + R"(.list_item",
                "$scrolling_content_anchor_from": "top_left",
                "$scrolling_content_anchor_to": "top_left"
              }
            }
          ]
        }
      }
    ]
  }
})";

    std::string py = R"(# -*- coding: utf-8 -*-
from mod.client.ui.screenNode import ScreenNode
import mod.client.extraClientApi as clientApi

def Register)" + cls + R"(():
    clientApi.RegisterUI(")" + mod + R"(", ")" + snake + R"(_list", ")" + mod + R"(.ui.)" + cls + R"(." + cls + R"(", ")" + ns + R"(.main")

class )" + cls + R"((ScreenNode):
    def __init__(self, namespace, name, param):
        ScreenNode.__init__(self, namespace, name, param)
        self._items = []

    def Create(self):
        pass

    def SetItems(self, items):
        """正向接口：设置列表数据"""
        self._items = items
        # 通过正向接口逐个设置子控件文本
        for i, item in enumerate(items):
            ctrl = self.GetBaseUIControl("/panel/scroll/scroll_view/stack_panel/list_item[%d]/text" % i)
            if ctrl:
                ctrl.asLabel().SetText(item)
)";

    return {json, py, "\"ui/" + snake + "_list.json\"", ""};
}

// ── screen_grid ───────────────────────────────────────
static UiTemplateResult gen_screen_grid(const std::string& ns, const std::string& mod) {
    std::string cls = to_pascal(mod) + "GridScreenNode";
    std::string snake = to_snake(mod);

    std::string json = R"({
  "namespace": ")" + ns + R"(",
  "grid_item": {
    "type": "panel", "size": [50, 50],
    "controls": [
      { "icon": { "type": "image", "size": ["100%", "100%"], "texture": "textures/ui/cell_image_normal" } }
    ]
  },
  "main@netease_editor_template_namespace.netease_editor_root_panel": {
    "controls": [
      { "bg": { "type": "image", "size": ["100%", "100%"], "texture": "textures/ui/bg32", "alpha": 0.6, "layer": 0 } },
      {
        "panel": {
          "type": "panel", "size": ["80%", "80%"], "layer": 1,
          "controls": [
            {
              "grid": {
                "type": "grid", "size": ["100%", "100%"],
                "grid_dimensions": [6, 4],
                "collection_name": "grid_items",
                "controls": [
                  { "item@)" + ns + R"(.grid_item": {} }
                ]
              }
            }
          ]
        }
      }
    ]
  }
})";

    std::string py = R"(# -*- coding: utf-8 -*-
from mod.client.ui.screenNode import ScreenNode
import mod.client.extraClientApi as clientApi

def Register)" + cls + R"(():
    clientApi.RegisterUI(")" + mod + R"(", ")" + snake + R"(_grid", ")" + mod + R"(.ui.)" + cls + R"(." + cls + R"(", ")" + ns + R"(.main")

class )" + cls + R"((ScreenNode):
    def __init__(self, namespace, name, param):
        ScreenNode.__init__(self, namespace, name, param)

    def Create(self):
        pass
)";

    return {json, py, "\"ui/" + snake + "_grid.json\"", ""};
}

// ── screen_form ───────────────────────────────────────
static UiTemplateResult gen_screen_form(const std::string& ns, const std::string& mod) {
    std::string cls = to_pascal(mod) + "FormScreenNode";
    std::string snake = to_snake(mod);

    std::string json = R"({
  "namespace": ")" + ns + R"(",
  "main@netease_editor_template_namespace.netease_editor_root_panel": {
    "controls": [
      { "bg": { "type": "image", "size": ["100%", "100%"], "texture": "textures/ui/bg32", "alpha": 0.6, "layer": 0 } },
      {
        "panel": {
          "type": "panel", "size": ["60%", "50%"], "layer": 1,
          "controls": [
            {
              "form": {
                "type": "stack_panel", "orientation": "vertical", "size": ["100%", "100%"],
                "controls": [
                  { "title": { "type": "label", "text": ")" + ns + R"(", "color": [1,1,1], "size": ["100%", 30] } },
                  {
                    "inputBox": {
                      "type": "edit_box", "size": ["100%", 30],
                      "max_length": 100,
                      "$place_holder_text": "请输入...",
                      "$text_box_name": "#input_text"
                    }
                  },
                  { "spacer": { "type": "panel", "size": ["100%", 10] } },
                  {
                    "confirmBtn@common.button": {
                      "size": ["100%", 30],
                      "$pressed_button_name": "button.confirm",
                      "controls": [
                        { "label": { "type": "label", "text": "确认", "color": [1,1,1] } }
                      ]
                    }
                  }
                ]
              }
            }
          ]
        }
      }
    ]
  }
})";

    std::string py = R"(# -*- coding: utf-8 -*-
from mod.client.ui.screenNode import ScreenNode
import mod.client.extraClientApi as clientApi

def Register)" + cls + R"(():
    clientApi.RegisterUI(")" + mod + R"(", ")" + snake + R"(_form", ")" + mod + R"(.ui.)" + cls + R"(." + cls + R"(", ")" + ns + R"(.main")

class )" + cls + R"((ScreenNode):
    def __init__(self, namespace, name, param):
        ScreenNode.__init__(self, namespace, name, param)
        self._input_ctrl = None

    def Create(self):
        self._input_ctrl = self.GetBaseUIControl("/panel/form/inputBox")
        self.AddTouchEventHandler("/panel/form/confirmBtn", self.OnConfirm)

    def GetInputText(self):
        if self._input_ctrl:
            return self._input_ctrl.asEditBox().GetEditText()
        return ""

    def OnConfirm(self, args):
        touchEvent = args["TouchEvent"]
        if touchEvent == 0:
            text = self.GetInputText()
            # TODO: 处理表单提交
)";

    return {json, py, "\"ui/" + snake + "_form.json\"", ""};
}

// ── screen_tabbed ─────────────────────────────────────
static UiTemplateResult gen_screen_tabbed(const std::string& ns, const std::string& mod) {
    std::string cls = to_pascal(mod) + "TabbedScreenNode";
    std::string snake = to_snake(mod);

    std::string json = R"({
  "namespace": ")" + ns + R"(",
  "tab_content_0": { "type": "panel", "size": ["100%", "100%"] },
  "tab_content_1": { "type": "panel", "size": ["100%", "100%"], "visible": false },
  "main@netease_editor_template_namespace.netease_editor_root_panel": {
    "controls": [
      { "bg": { "type": "image", "size": ["100%", "100%"], "texture": "textures/ui/bg32", "alpha": 0.6, "layer": 0 } },
      {
        "panel": {
          "type": "panel", "size": ["80%", "80%"], "layer": 1,
          "controls": [
            {
              "tabs": {
                "type": "stack_panel", "orientation": "horizontal",
                "size": ["100%", 30], "anchor_from": "top_left", "anchor_to": "top_left",
                "controls": [
                  {
                    "tab0@common.button": {
                      "size": ["50%", 30], "$pressed_button_name": "button.tab0",
                      "controls": [{ "label": { "type": "label", "text": "标签1", "color": [1,1,1] } }]
                    }
                  },
                  {
                    "tab1@common.button": {
                      "size": ["50%", 30], "$pressed_button_name": "button.tab1",
                      "controls": [{ "label": { "type": "label", "text": "标签2", "color": [1,1,1] } }]
                    }
                  }
                ]
              }
            },
            {
              "content": {
                "type": "panel", "size": ["100%", "100%-30px"],
                "anchor_from": "bottom_middle", "anchor_to": "bottom_middle",
                "controls": [
                  { "page0@)" + ns + R"(.tab_content_0": {} },
                  { "page1@)" + ns + R"(.tab_content_1": {} }
                ]
              }
            }
          ]
        }
      }
    ]
  }
})";

    std::string py = R"(# -*- coding: utf-8 -*-
from mod.client.ui.screenNode import ScreenNode
import mod.client.extraClientApi as clientApi

def Register)" + cls + R"(():
    clientApi.RegisterUI(")" + mod + R"(", ")" + snake + R"(_tabbed", ")" + mod + R"(.ui.)" + cls + R"(." + cls + R"(", ")" + ns + R"(.main")

class )" + cls + R"((ScreenNode):
    def __init__(self, namespace, name, param):
        ScreenNode.__init__(self, namespace, name, param)
        self._pages = []
        self._current_tab = 0

    def Create(self):
        self._pages = [
            self.GetBaseUIControl("/panel/content/page0"),
            self.GetBaseUIControl("/panel/content/page1"),
        ]
        self.AddTouchEventHandler("/panel/tabs/tab0", self.OnTab0)
        self.AddTouchEventHandler("/panel/tabs/tab1", self.OnTab1)

    def _switch_tab(self, idx):
        for i, page in enumerate(self._pages):
            if page:
                page.SetVisible(i == idx)
        self._current_tab = idx

    def OnTab0(self, args):
        if args["TouchEvent"] == 0:
            self._switch_tab(0)

    def OnTab1(self, args):
        if args["TouchEvent"] == 0:
            self._switch_tab(1)
)";

    return {json, py, "\"ui/" + snake + "_tabbed.json\"", ""};
}

// ── hud_overlay ───────────────────────────────────────
static UiTemplateResult gen_hud_overlay(const std::string& ns, const std::string& mod) {
    std::string cls = to_pascal(mod) + "HudNode";
    std::string snake = to_snake(mod);

    std::string json = R"({
  "namespace": ")" + ns + R"(",
  "main@netease_editor_template_namespace.netease_editor_root_panel": {
    "is_modal": false,
    "should_steal_mouse": false,
    "absorbs_input": false,
    "always_accepts_input": false,
    "controls": [
      {
        "hud": {
          "type": "panel", "size": ["100%", "100%"], "layer": 10,
          "controls": [
            {
              "info_label": {
                "type": "label", "text": "", "color": [1,1,1],
                "size": ["100%", 20],
                "anchor_from": "top_left", "anchor_to": "top_left",
                "offset": [5, 5]
              }
            }
          ]
        }
      }
    ]
  }
})";

    std::string py = R"(# -*- coding: utf-8 -*-
from mod.client.ui.screenNode import ScreenNode
import mod.client.extraClientApi as clientApi

def Register)" + cls + R"(():
    clientApi.RegisterUI(")" + mod + R"(", ")" + snake + R"(_hud", ")" + mod + R"(.ui.)" + cls + R"(." + cls + R"(", ")" + ns + R"(.main")

class )" + cls + R"((ScreenNode):
    def __init__(self, namespace, name, param):
        ScreenNode.__init__(self, namespace, name, param)
        self._info_ctrl = None

    def Create(self):
        self._info_ctrl = self.GetBaseUIControl("/hud/info_label")

    def SetInfoText(self, text):
        if self._info_ctrl:
            self._info_ctrl.asLabel().SetText(text)
)";

    return {json, py, "\"ui/" + snake + "_hud.json\"", ""};
}

// ── widget_button ─────────────────────────────────────
static UiTemplateResult gen_widget_button(const std::string& ns, const std::string&) {
    std::string json = R"({
  "namespace": ")" + ns + R"(",
  "my_button@common.button": {
    "size": [120, 30],
    "$pressed_button_name": "button.my_action",
    "controls": [
      { "default@netease_editor_template_namespace.default": { "$default_texture": "textures/ui/button_borderless_light" } },
      { "hover@netease_editor_template_namespace.hover": { "$hover_texture": "textures/ui/button_borderless_lighthover" } },
      { "pressed@netease_editor_template_namespace.pressed": { "$pressed_texture": "textures/ui/button_borderless_lightpressed" } },
      { "label": { "type": "label", "text": "按钮", "color": [1,1,1], "layer": 3 } }
    ]
  }
})";
    return {json, "", "", ""};
}

// ── widget_progress ───────────────────────────────────
static UiTemplateResult gen_widget_progress(const std::string& ns, const std::string&) {
    std::string json = R"({
  "namespace": ")" + ns + R"(",
  "my_progress_bar": {
    "type": "panel", "size": [200, 20],
    "controls": [
      {
        "empty@netease_editor_template_namespace.empty_progress_bar": {
          "$progress_bar_empty_texture": "textures/ui/progress_bar_empty",
          "size": ["100%", "100%"]
        }
      },
      {
        "filled@netease_editor_template_namespace.filled_progress_bar": {
          "$progress_bar_filled_texture": "textures/ui/progress_bar_filled",
          "$clip_direction": "left",
          "$clip_ratio": 0.5,
          "size": ["100%", "100%"]
        }
      }
    ]
  }
})";
    return {json, "", "", ""};
}

// ── 入口 ──────────────────────────────────────────────
UiTemplateResult generate_ui_fullstack(const std::string& template_type, const std::string& ns, const std::string& mod_name) {
    if (template_type == "screen_basic")  return gen_screen_basic(ns, mod_name);
    if (template_type == "screen_list")   return gen_screen_list(ns, mod_name);
    if (template_type == "screen_grid")   return gen_screen_grid(ns, mod_name);
    if (template_type == "screen_form")   return gen_screen_form(ns, mod_name);
    if (template_type == "screen_tabbed") return gen_screen_tabbed(ns, mod_name);
    if (template_type == "hud_overlay")   return gen_hud_overlay(ns, mod_name);
    if (template_type == "widget_button") return gen_widget_button(ns, mod_name);
    if (template_type == "widget_progress") return gen_widget_progress(ns, mod_name);
    return {"", "", "", "unknown template_type: " + template_type + ". Valid: screen_basic, screen_list, screen_grid, screen_form, screen_tabbed, hud_overlay, widget_button, widget_progress"};
}

} // namespace mcdk
