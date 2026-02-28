# 基本UI界面类 <Badge type="tip" text="1.3.6+" />
新版界面类提供了一系列与原生界面直接关联交互的基本能力（如`绑定`），以解决旧式类`EasyScreenNodeCls`的历史遗留问题。

## 基本界面类
在`QuModLibs.UI`模块下提供了一个`ScreenNodeWrapper`类，是对原版界面类的重新封装实现。
### 示例代码
```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.UI import ScreenNodeWrapper

@ScreenNodeWrapper.autoRegister("JSON_FILE.main")
class MyScreen(ScreenNodeWrapper):
    def __init__(self, namespace, name, param):
        ScreenNodeWrapper.__init__(self, namespace, name, param)
        print("界面对象构造完毕，此时还未完成绘制，不得操作控件")

    def Create(self):
        # 请确保重写生命周期方法后必定调用父类方法，确保内部的资源管理正常进行（除非完全不涉及相关需求）
        ScreenNodeWrapper.Create(self)
        print("UI绘制完毕，此时可以操作控件对象")

# 跳一跳打开界面
@Listen("ClientJumpButtonReleaseEvent")
def ClientJumpButtonReleaseEvent(_={}):
    MyScreen.pushScreen()  # 或者：MyScreen.createUI()
```
通过类方法装饰器`@ScreenNodeWrapper.autoRegister(...)`，可以实现界面的自动化注册。

### 注意事项
`autoRegister`仅用于将注册任务提交到`UIFinish`事件。如果相关模块是在UI加载完成后动态导入的，则该装饰器不会生效。

## 基本界面创建
您可以使用类方法`pushScreen`创建窗口UI，亦或者`createUI`创建HUD界面。
```python
class MyScreen(ScreenNodeWrapper):
    ...

# 请确保在UIFinish之后创建
MyScreen.pushScreen()
MyScreen.createUI()
```
### 关于内部隔离
`ScreenNodeWrapper`默认对内隔离（按MOD名以及类路径），可以观察到相关方法带有 **uiKey=""** 的默认参数，得益于隔离机制的存在，空key也能安全的操作界面。（直接使用）

## 封装方法
快捷注册按钮点击回调。
```python
class MyScreen(ScreenNodeWrapper):
    def Create(self):
        ScreenNodeWrapper.Create(self)
        # 快速注册按钮点击回调（无args）
        self.setButtonClickHandler("/panel/button", lambda: self.SetRemove())

        # 局部装饰器动态注册点击回调（随时可用）
        @self.bindButtonClickHandler("/panel/button2")
        def onClick():
            ...
```

## 获取界面实例
使用`getUiNode`可以在外部直接获取对应的UI节点实例（如果存在，否则返回None）。
```python
uiNode = MyScreen.getUiNode()
if uiNode:
    ...
```

## 销毁界面
对于界面本身，您可以直接使用原版的`SetRemove`方法完成界面销毁，对于外部访问则可以用对应的类方法`removeClsUI`安全的销毁处理。
```python
MyScreen.removeClsUI()  # 将会自动判断界面是否存在实例，否则什么也不会发生
```

## 使用原生绑定
使用`ScreenNodeWrapper`可以直接兼容原版界面类的一切功能，包括不限于`绑定注解`。
```python
@ViewBinder.binding(ViewBinder.BF_xxxx, "#xxx")
```

## RAII机制 <Badge type="tip" text="1.3.8+" />
在`1.3.8`版本开始，ScreenNodeWrapper默认继承`QDRAIIEnv`环境类，支持上下文资源生命周期管理。
```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.Modules.UI.Client import QGridBinder, QGridData

@ScreenNodeWrapper.autoRegister("JSON_FILE.main")
class MyScreen(ScreenNodeWrapper):
    def __init__(self, namespace, name, param):
        ScreenNodeWrapper.__init__(self, namespace, name, param)
        # RAII自动管理释放问题（界面销毁时自动触发特定逻辑）
        self.gridBinder = QGridBinder(self, QGridData("/panel/grid", False, self.renderUpdate))

    def renderUpdate(self, viewPath="", index=0):
        # 网格渲染逻辑
        ...
```
若有需要，您可以自定义class并继承`QRAIIDelayed`托管UI资源生命周期。

## 高级UI界面类 <Badge type="tip" text="1.3.8+" />
在`1.3.8`版本开始，`QuModLibs.Modules.UI.EnhancedUI`提供了更为高级的`QEScreenNode`界面类。
### 示例代码
```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.Modules.UI.EnhancedUI import QEScreenNode
# QEScreenNode 继承于 ScreenNodeWrapper, TimerLoader, AnnotationLoader
# 支持自定义注解（内部提供一部分现成的实现，允许自定义继承开发），自有定时器等功能。

@QEScreenNode.autoRegister("xxx.main")
class MY_UI(QEScreenNode):
    def __init__(self, namespace, name, param):
        QEScreenNode.__init__(self, namespace, name, param)
        # PS：得益于内部缓存优化机制，注解只会被完整的扫描一次，后续直接匹配历史目标，因此热更新可能无法计入新的注解。（或者禁用缓存，但不推荐）

    @QEScreenNode.Listen("OnKeyPressInGame")
    def OnKeyPressInGame(self, _={}):
        # 跟随UI生命周期的监听，UI销毁后监听自动取消
        self.SetRemove()

    @QEScreenNode.OnClick("/panel/close")
    def onClose(self):
        # 不管是push界面还是create界面，注解都只在Create期间被扫描，确保最大兼容。（Destroy期间回收）
        self.SetRemove()

    @QEScreenNode.LoopTimer(0.5)
    def testLoop(self):
        print("0.5s执行一次")
    
    @ViewBinder.binding(ViewBinder.BF_xxxx, "#xxx")
    def nativeBinding(self):
        # 支持原生绑定
        return ...

@Listen("ClientJumpButtonReleaseEvent")
def ClientJumpButtonReleaseEvent(_={}):
    MY_UI.pushScreen()
```
增强界面类`QEScreenNode`被化分在了一个单独的模块，需要额外导入使用，适用于复杂场景需求。

::: tip 关于 EasyScreenNodeCls
旧版UI界面类因其历史原因难以维护，现已不推荐使用（预计兼容至`1.5.x`版本后移除）。
:::

## 多态继承
使用`ScreenNodeWrapper`并不一定要构建完整的界面类，您也可以多态编程，高效管理项目结构。
```python
# 并不一定要注册
class UI1(QEScreenNode):
    def renderList(self):
        ...

    def getRenderDatas(self):
        return [...]

@QEScreenNode.autoRegister(...)
class UI2(UI1):
    def getRenderDatas(self):
        # 重写，快速整合功能
        return [...]
```