# UI动画系统模块
Anims模块基于Modules.UI中的`QUIAutoControlFuntion`复合实现，在`1.4.0+`版本中支持`QRAIIDelayed`协议。

## class QAnimsControl
### 动画控件功能
基于 `QUIAutoControlFuntion` 实现的动画托管对象，允许您为指定的控件添加多种动画变换效果。

```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.UI import EasyScreenNodeCls
from .QuModLibs.Modules.UI.Anims import QAnimsControl, QPosTransform, QSizeTransform

# 此处为旧版UI类的手动管理策略，仅参考组合类使用，不建议再引用老版本界面类本身，自动管理另见下文。
@EasyScreenNodeCls.Binding("testUI3.main")
class TEST_UI3(EasyScreenNodeCls):
    def __init__(self):
        self.imgPath = "/panel/image"
        self.imgAnimObj = QAnimsControl.bindControl(self, self.imgPath)	# 绑定控件关联

	@EasyScreenNodeCls.Listen(Events.OnScriptTickClient)
    def onTick(self, _={}):
        self.imgAnimObj.update()	# 更新动画运算

    @EasyScreenNodeCls.OnClick("/panel/move")
    def onMove(self):
        # 播放移动动画与大小变换动画
        self.imgAnimObj.changeTransformAnim(QPosTransform(self.imgAnimObj.getPos(), (400, 100), useTime=0.8))
        self.imgAnimObj.changeTransformAnim(QSizeTransform(self.imgAnimObj.getScale(), self.imgAnimObj.controlInfo._baseScale, useTime=0.8, resizeChildren=True))

```
```text
Q: update方法是干什么的?
答: update方法用于管理内部的时间推进，通常放置在tick/帧事件中更新绘制。

Q: 手动调用更新是不是有点太麻烦了?
答: 是的，所以在下文我们提供了一个封装好的管理器。
```

### addTransformAnim

添加变换动画对象到当前控件

```python
# -*- coding: utf-8 -*-
#.... 篇幅问题省略UI的完整代码
self.imgAnimObj.addTransformAnim(
    QPosTransform(...)	# 任意一种变换对象
)
# 此操作不会计算重复类型动画覆盖 因此大多数情况下建议使用另外一种 changeTransformAnim

```
::: tip 注意事项
单个变换对象只能同时应用在一个控件上 若有复用需求请使用copy方法克隆
:::

### changeTransformAnim

添加变换动画对象到当前控件并覆盖同类型动画

```python
# -*- coding: utf-8 -*-

self.imgAnimObj.changeTransformAnim(
    QPosTransform(...)	# 使用该方法可以很好的避免同类型动画叠加进行导致的异常
)

```



### removeTransformAnim

移除指定的动画变换对象

```python
# -*- coding: utf-8 -*-

anim = QPosTransform(...)
self.imgAnimObj.removeTransformAnim(anim)	# 如果正在播放的动画集合中存在则会移除

```



### matchTransformWithClass

基于class匹配已存在的动画对象

```python
# -*- coding: utf-8 -*-

self.imgAnimObj.matchTransformWithClass(
    QPosTransform
) # 匹配 QPosTransform类型(或子类)的实例 不存在则返回None

```



### update

更新内部运算逻辑

```python
# -*- coding: utf-8 -*-

self.imgAnimObj.update(0.033, forceUpdate = True)	# 注意forceUpdate仅作为可见参数下传到所有播放的动画变换对象 如果相关动画对象并没有涉及forceUpdate的实现 该参数则是无效的 (目前为止内置的动画受API限制是不具有forceUpdate功能的)

```



### (属性) controlInfo

控件信息对象 包含了第一次构造的信息记忆

```python
# -*- coding: utf-8 -*-

pos = self.imgAnimObj.controlInfo._basePos		# 初始位置
scale = self.imgAnimObj.controlInfo._baseScale	# 初始大小

```



## class QTransform

动画变换基类 所有的动画类型实现均继承自QTransform (篇幅问题仅介绍个别常用方法)

### setFinishAnimBackCall

[链式方法] 设置动画完整播放完毕后的回调

```python
# -*- coding: utf-8 -*-

# 设置回调可以实现动画循环/多段轨迹
QPosTransform(...).setFinishAnimBackCall(
	lambda: xxx
)

```



### setEasingMode

[链式方法] 设置动画缓动类型

```python
# -*- coding: utf-8 -*-
from .QuModLibs.Modules.UI.Anims import QPosTransform, QTransform

QPosTransform(...).setEasingMode(
	QTransform.EasingMode.EaseInSide	# 使用EaseInSide缓动函数 默认为线性
).setFinishAnimBackCall(...)

# 您也可以直接修改全局默认缓动类型 如下:
# QTransform.EasingMode.Default = QTransform.EasingMode.EaseInSide

"""
    现阶段一共提供了3种缓动机制 可根据需求使用
    QTransform.EasingMode.Linear (默认)
    QTransform.EasingMode.EaseInSide
    QTransform.EasingMode.EaseOutQuad
"""

```



### 内置动画类型
若有需要 也可以自定义动画类型 继承自 QTransform 实现 详细可参考源代码

```python
# -*- coding: utf-8 -*-
from .QuModLibs.Modules.UI.Anims import QTransform, QPosTransform, QSizeTransform, QAlphaTransform, QuTypeWriter

QTransform.EasingMode.Default = QTransform.EasingMode.EaseOutQuad	# 设置全局缓动类型

# =============== 内置动画示例 ===============
QPosTransform((0,0), (100, 100), 3.0)	# 坐标动画 从 0,0 到 100,100 用时3s

QSizeTransform(
    (10,10), (100, 100), 3.0, resizeChildren=False
)										# 大小动画同上 resizeChildren是否重新计算子节点

QAlphaTransform(0.0, 1.0, 3.0)			# 不透明度动画 从0.0到1.0用时3s (仅支持图片/文字)

QuTypeWriter("你好, 世界", 2.0, childPath="", syncSize=False)	# 打字机动画 2s内逐一呈现文字(仅支持文字控件)

```



## class QAnimManager
动画管理器，基于`QUIAutoControlFuntion` + `QRAIIDelayed`实现自动释放。

### 新式界面/符合RAII协议的界面
基于QRAIIDelayed的自动管理释放。
```python
# -*- coding: utf-8 -*-
from .QuModLibs.Client import *
from .QuModLibs.UI import ScreenNodeWrapper # 新式UI基类，支持RAII
from .QuModLibs.Modules.UI.Anims import QAnimsControl, QPosTransform, QSizeTransform, QAnimManager

@ScreenNodeWrapper.autoRegister("testUINew.main") # 新式UI注册
class NEW_TEST_UI(ScreenNodeWrapper):
    def __init__(self, *args):
        ScreenNodeWrapper.__init__(self, *args)
        self.imgPath = "/panel/image"
        # RAIIDelayed协议会确保延后到Create再自动调用 因此可以直接在init下绑定
        self.animSystem = QAnimManager.bindRAIINode(self)	# 使用bindRAIINode方法绑定(1.4.0+)

    @ScreenNodeWrapper.OnClick("/panel/move")
    def onMove(self):
        # 播放移动动画与大小变换动画
        animObj = self.animSystem.getControlAnimObj(self.imgPath)	# 通过管理器拿到特定控件的动画功能对象
        animObj.changeTransformAnim(QPosTransform(animObj.getPos(), (400, 100), useTime=0.8))
        sanimObj.changeTransformAnim(QSizeTransform(animObj.getScale(), animObj.controlInfo._baseScale, useTime=0.8, resizeChildren=True))
```