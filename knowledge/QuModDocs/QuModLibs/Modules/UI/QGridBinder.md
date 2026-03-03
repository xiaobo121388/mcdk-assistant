# QGridBinder + QGridData 网格绑定（QuModLibs UI 网格开发必读）

> **QuModLibs 项目中所有网格/列表网格 UI 开发的标准方案。不要手动监听 `GridComponentSizeChangedClientEvent`。**

```python
from .QuModLibs.Modules.UI.Client import QGridBinder, QGridData
```

## 核心规则

1. **异步池化复用**：网格格子被引擎异步创建/回收/复用，同一控件不应该缓存处理
2. **所有操作必须在 `bindFunc` 回调中完成**：不要在回调外缓存路径操作格子
3. **格子根控件不可 SetVisible**：回调期间 `viewPath` 被引擎接管，只能操作其**子控件**可见性
4. **SetVisible 第二参数必须 False**：`SetVisible(True/False, False)` 避免每格触发全局 UI 刷新
5. **按钮绑定必须每次重做**：复用会破坏旧绑定，`setButtonClickHandler` 必须在 `bindFunc` 中每次调用
6. **只有 button 类型控件才能设置点击/触摸响应**：`setButtonClickHandler` 需要目标控件在 JSON UI 中为 `button` 类型
7. **99% 场景只需 `path` + `bindFunc`**：其他参数（`incrementalCallback` 等）按需使用

## QGridData 构造

```python
QGridData(
    path,                                   # str   网格路径（滚动网格填 ScrollView 路径）
    isScrollGrid=False,                     # bool  是否 ScrollView+Grid 模式
    bindFunc=lambda *_: None,               # (str, int) -> None  核心渲染回调
    incrementalCallback=lambda *_: None,    # (str, int) -> None  增量回调（首次渲染时，99%不需要）
    bindUpdateBeforeFunc=lambda *_: None,   # () -> None  每轮更新前置（极少用）
    bindUpdateFinishFunc=lambda *_: None,   # () -> None  每轮更新后置（极少用）
    bindGridConName="",                     # str   格子控件名含数字时显式声明
)
```

### QGridData 方法

```python
gridData.updateRender(uiNode)               # 触发完整渲染
gridData.updateOnceRender(viewPath, index)   # 单格渲染
gridData.clearIncrementalCache()             # 清理增量缓存
gridData.getRealPath(uiNode)                 # 获取真实路径（滚动网格自动取内容路径）
gridData.setGridPathBasedOnScrollView(path)  # ScrollView与Grid间有中间层时设置偏移
```

## QGridBinder 构造

```python
QGridBinder(uiNode, gridData=None, parentPath="")
# gridData 不为 None 时自动注册 RAII；为 None 时需 setGridData + start
```

### QGridBinder 方法（均返回 self 支持链式调用）

```python
binder.setGridData(gridData)                 # 绑定数据
binder.start() / binder.stop()               # 手动启停（非 RAII 时）
binder.updateRender()                        # 触发渲染
binder.setGridDimension((cols, rows))        # 设置维度（int 类型）
binder.setButtonClickHandler(path, onClick)  # 按钮点击绑定
binder.clearIncrementalCache()               # 清理增量缓存
binder.getRealPath()                         # 获取真实路径
```

## 最简用法（RAII 自动管理）

```python
@ScreenNodeWrapper.autoRegister("myui.main")
class MyUI(ScreenNodeWrapper):
    def __init__(self, namespace, name, param):
        ScreenNodeWrapper.__init__(self, namespace, name, param)
        self.dataList = ["A", "B", "C"]
        self.gridBinder = QGridBinder(self, QGridData(
            "/panel/grid", bindFunc=self.onRender
        ))

    def onRender(self, viewPath, index):
        # type: (str, int) -> None
        content = self.GetBaseUIControl(viewPath + "/content")
        if index < len(self.dataList):
            content.SetVisible(True, False)  # 第二参数 False！
            self.GetBaseUIControl(viewPath + "/content/label").asLabel().SetText(
                self.dataList[index]
            )
            # 按钮绑定必须每次重做（复用机制）
            self.gridBinder.setButtonClickHandler(
                viewPath + "/content/btn", lambda: self.onClick(index)
            )
        else:
            content.SetVisible(False, False)  # 隐藏多余格子

    def onClick(self, index):
        print("点击 {}".format(index))

    def Create(self):
        ScreenNodeWrapper.Create(self)  # 自动 start
        self.gridBinder.setGridDimension((1, len(self.dataList)))

    def Destroy(self):
        ScreenNodeWrapper.Destroy(self)  # 自动 stop
```

## 滚动网格（ScrollView + Grid）

```python
self.gridBinder = QGridBinder(self, QGridData(
    "/panel/scrollView",    # 填 ScrollView 路径，不是 Grid 路径
    isScrollGrid=True,
    bindFunc=self.onRender
))

# ScrollView 与 Grid 间有中间层时：
gridData.setGridPathBasedOnScrollView("/wrapper/grid")
```

## 多列网格行数计算（Py2 兼容）

```python
import math
COLUMNS = 3
count = len(dataList)
# Py2: math.ceil 返回 float，SetGridDimension 需要 int
rows = int(math.ceil(count / float(COLUMNS)))
# 或纯整数方案（推荐）：
rows = (count + COLUMNS - 1) // COLUMNS
self.gridBinder.setGridDimension((COLUMNS, rows))
# 网格渲染 COLUMNS*rows 个格子，超出数据量的用 SetVisible(False, False) 隐藏
```

## 手动管理（非 RAII）

```python
# Create 中
self.gridBinder = QGridBinder(self).setGridData(
    QGridData("/panel/grid", bindFunc=self.onRender)
).start()

# Destroy 中
self.gridBinder.stop()
```

## bindGridConName

格子控件名含数字尾缀时（如 `item2`），路径 `/grid/item21` 会误提取 index=21。声明 `bindGridConName="item2"` 修正为 index=1。

## FAQ

- **index 起始**：从 0 开始
- **动态改数量**：`setGridDimension` 后引擎自动触发渲染回调
- **滚动网格 path**：填 ScrollView 路径，QGridData 自动通过 `GetScrollViewContentPath()` 取真实路径
- **链式调用**：`binder.setGridData(data).start().setGridDimension((4,10))`
