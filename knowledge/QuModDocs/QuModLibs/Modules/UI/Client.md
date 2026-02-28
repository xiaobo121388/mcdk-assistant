# UI基本功能扩展
UI功能扩展，实现对象化管理界面控件/分页。

## QGridData

### 网格对象参数
通过网格对象可以快速的构建一个网格控件的资源管理，搭配相关更新事件完成异步刷新响应。

```python
# -*- coding: utf-8 -*-
from .QuModLibs.Modules.UI.Client import QGridData

gridDataObj = QGridData("/panel/grid", isScrollGrid = False, bindFunc = lambda *_: None, bindUpdateFinishFunc = lambda *_: None, bindGridConName = "")	# 网格对象参数

# isScrollGrid - 声明为滚动网格 (列表直接嵌套网格)
# bindFunc - 绑定一个回调函数类型为 (str, int) -> None 当发起更新时将会计算所有渲染下的控件并调用
# bindUpdateFinishFunc - 绑定一个更新结束回调函数 () -> None 当每轮更新结束(即bindFunc循环执行完毕)后置执行 可用于实现延迟刷新
# bindGridConName - 绑定网格控件名称 (涉及网格渲染控件pos分析) 绝大多数情况下可以不绑定 将使用后置数字分析方案完成

```

### 手动实现网格管理
基于原版游戏事件 **GridComponentSizeChangedClientEvent** 手动完成对接。

```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.Modules.UI.Client import QGridData

gridDataObj = QGridData("/panel/grid", ...)	# 网格对象参数

# 监听网格变化事件
@Listen(Events.GridComponentSizeChangedClientEvent)
def GridComponentSizeChangedClientEvent(args={}):
    path = str(args["path"])
    uiNode = ...	# 提供一个uiNode作上下文 通过getRealComponentPath方法计算刷新判定
    if path.endswith(gridDataObj.getRealComponentPath(uiNode)):
        # 当符合条件时调用updateRender触发绑定的bindFunc完成网格处理关联
        gridDataObj.updateRender(uiNode)
# 为了方便动态的使用 通常需要 手动 ListenForEvent / UnListenForEvent 管理 此处仅作演示

```

大多数情况下不需要您手动实现网格管理，例如UI类中内置了这一机制`QUICanvas`类中也提供了相关方法，
该业务逻辑被单独抽离出来以便一些更为复杂的需求定制。

## QUICanvas
QUI画布绘制类，提供控件逻辑的封装复用功能，同时充当一个协议。

### 构建一个基本Canvas

```python
# -*- coding: utf-8 -*-
from .QuModLibs.UI import ScreenNodeWrapper
from .QuModLibs.Modules.UI.Client import QUICanvas

# ======== 自定义Canvas ========
class MyCanvas(QUICanvas):
    def __init__(self, uiNode, parentPath):
        QUICanvas.__init__(self, uiNode, parentPath)
        self.drawDefName = "namespace.controlName"	# drawDefName 属性用于关联绑定一个JSON自定义控件

    def onCreate(self):
        QUICanvas.onCreate(self)	# 父类的onCreate方法实现了self.drawDefName控件的创建也可以不使用默认的机制
        print("成功绘制, 路径为: " + self._conPath)
        # self.getBaseUIControl() 也可以通过该方法直接拿到控件对象

    def onDestroy(self):
        QUICanvas.onDestroy(self)
        print("外部调用removeControl时触发 通常做后置回收处理 大多数情况下不需要 在一些涉及事件监听的需求中用到")

# ======== 自定义UI应用自定义Canvas ========
@ScreenNodeWrapper.autoRegister("xxx.main")
class MyUI(ScreenNodeWrapper):
    def __init__(self, *args):
        ScreenNodeWrapper.__init__(self, *args)
        self._myCanvas = MyCanvas(self, "/panel")	# 绑定到当前UI的特定父节点

    def Create(self):
        ScreenNodeWrapper.Create(self)
        self._myCanvas.createControl()	# 创建Canvas

	def Destroy(self):
        ScreenNodeWrapper.Destroy(self)
        # removeControl(justOnDestroy = False)		# justOnDestroy默认False 将会执行控件删除逻辑
        self._myCanvas.removeControl(True)			# UI界面销毁时请置为True(不执行控件删除将跟随UI一起销毁)

```
```text
Q: 我是否一定要调用removeControl完成回收工作 ?
答: 并不一定 倘若此Canvas并不涉及需要回收处理的逻辑且不需要动态删除则可以忽略调用 (为防歧义最好统一调用)
```

### 进阶版Canvas
在Canvas中操作uiNode，并注册网格监听事件，完成渲染处理。

```python
# -*- coding: utf-8 -*-
from .QuModLibs.UI import ScreenNodeWrapper
from .QuModLibs.Modules.UI.Client import QUICanvas

# ======== 自定义Canvas ========
class MyCanvas(QUICanvas):
    def __init__(self, uiNode, parentPath):
        QUICanvas.__init__(self, uiNode, parentPath)
        self.drawDefName = "namespace.controlName"
        self._QGridData = None	# type: QGridData | None
        self.dataList = []		# 绑定数据列表

    def renderUpdate(self, viewPath, i):
        # type: (str, int) -> None
        # 异步渲染加载触发 其中i代表下标 viewPath返回控件路径
        data = self.dataList[i]
        
    def onCreate(self):
        self.clearParent()		  # 该方法用于清理parent中所有的控件 以便独占位置
        QUICanvas.onCreate(self)
        uiNode = self.getUiNode() # 为防止内存泄漏 父节点的引用储存采用弱引用机制 该方法用于解包弱引用
        
        # 调用uiNode的方法 实现一个带有网格列表渲染的Canvas
        self._QGridData = QGridData(self._conPath, True, bindFunc=self.renderUpdate)
        # 设置网格渲染数目
        uiNode.GetBaseUIControl(
            self._QGridData.getRealPath(uiNode)
        ).asGrid().SetGridDimension((1, len(self.dataList)))
        
        # 使用内置的实现方法监听网格渲染
        self.listenQGridRender(self._QGridData)

    def onDestroy(self):
        QUICanvas.onDestroy(self)
        # 使用内置的实现方法回收监听网格渲染
        self.unListenQGridRender(self._QGridData)
```

## QUIControlFuntion
QUI画布绘制类的派生类，相比QUICanvas，该class更多是处理已有控件逻辑，为其赋予特定功能而不是构建新的控件。

### 基本了解
```python
# -*- coding: utf-8 -*-
from .QuModLibs.UI import ScreenNodeWrapper
from .QuModLibs.Modules.UI.Client import QUIControlFuntion

class MyControlFuntion(QUIControlFuntion):
    def onCreate(self):
        QUIControlFuntion.onCreate(self)	# QUIControlFuntion并不会为您创建新的控件
        uiNode = self.getUiNode()
        parentPath = self._parentPath
        # 为parentPath编写业务
        ...
```
