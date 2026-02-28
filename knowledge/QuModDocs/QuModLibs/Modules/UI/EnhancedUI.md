# 增强界面模块
`EnhancedUI`提供了一些增强用户界面的功能。

## QEScreenNode 界面类
`QEScreenNode`是一个基于`ScreenNodeWrapper`扩展的界面类。

### class 定义
```python
class QEScreenNode(ScreenNodeWrapper, TimerLoader, AnnotationLoader):
    """
    增强界面类，继承自ScreenNodeWrapper，TimerLoader和AnnotationLoader。
    提供了定时器和注解加载功能。
    """
```

### 注解支持
`QEScreenNode`支持通过注解来简化界面元素的定义和管理。
```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.Modules.UI.EnhancedUI import QEScreenNode

@QEScreenNode.autoRegister("xxx.main")
class TestUI(QEScreenNode):
    @QEScreenNode.OnClick("/xxx/button")
    def testClick(self):
        print("Button clicked!")

    # 除了使用内置的注解也可以自定义注解，可参考源代码继承LifecycleBind开发。
    @QEScreenNode.Listen("xxx")
    def gameEventTest(self, args={}):
        print("Game Event Triggered: {}".format(args))
```

### 定时器支持
`QEScreenNode`集成了定时器功能，可以方便地在界面中使用定时任务。
```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.Modules.UI.EnhancedUI import QEScreenNode

@QEScreenNode.autoRegister("xxx.main")
class TestUI(QEScreenNode):
    def __init__(self, *args):
        QEScreenNode.__init__(self, *args)
        # 定时器由UI Upate上下文驱动
        self.addTimer(QEScreenNode.Timer(self.testCallBack, time=1.0))
    
    def testCallBack(self):
        print("Timer triggered!")
```

## QECanvas RAII画布类
`QECanvas`是一个基于`QUICanvas`的RAII增强画布类。
### class 定义
```python
class QECanvas(QUICanvas, IBaseQECanvas):
    """ 增强版画布类 基于RAII机制 """
    def __init__(self, uiNode, parentPath=""):
        QUICanvas.__init__(self, uiNode, parentPath)

    def autoLoad(self):
        ...

    def autoRemove(self):
        ...

    def _loadResource(self):
        QRAIIDelayed._loadResource(self)
        # 自动load控件
        self.createControl()

    def _cleanup(self):
        QRAIIDelayed._cleanup(self)
        # 自动析构控件
        if self.getUiNode()._raiiCleanState:
            self.removeControl(True)
        else:
            self.removeControl(False)
```

## QEControlFuntion RAII控件管理类
`QEControlFuntion`是一个基于`QUIControlFuntion`的RAII增强控件管理类。
### class 定义
```python
class QEControlFuntion(QUIControlFuntion, IBaseQECanvas):
    """ 增强版UI控制功能类 基于RAII机制 """
    def __init__(self, uiNode, parentPath = ""):
        QUIControlFuntion.__init__(self, uiNode, parentPath)

    def autoLoad(self):
        ...

    def autoRemove(self):
        ...

    def _loadResource(self):
        QRAIIDelayed._loadResource(self)
        # 自动load控件功能
        self.createControl()

    def _cleanup(self):
        QRAIIDelayed._cleanup(self)
        # 自动析构控件功能
        self.removeControl()
```