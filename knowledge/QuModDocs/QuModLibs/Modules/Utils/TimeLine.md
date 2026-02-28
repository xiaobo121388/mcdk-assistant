# 时间线运算模块
用于实现关键帧时间插值处理。
```python
from .QuModLibs.Modules.Utils.TimeLine import QTimeLine
```

## 创建时间线
时间线基本用法。
```python
from .QuModLibs.Modules.Utils.TimeLine import QTimeLine

timeline = QTimeLine({
    0.0: 10,
    0.5: 3,
    0.8: 12,
    1.0: -1
})

s = 0.0
while s < timeline.getMaxFpsTime():
    # 模拟时间更新(仅演示)
    s += 0.01
    value = timeline.computeFrameAtTime(s)
    print(value)    # computeFrameAtTime用于计算出线性插值(也可以自己实现)

```

## FArray 数组
若有需求，也可以自定义类型支持(需重载运算符)。
```python
from .QuModLibs.Modules.Utils.TimeLine import QTimeLine

timeline = QTimeLine({
    0.0: QTimeLine.FArray([0, 1, 0]),
    0.5: QTimeLine.FArray([1, 1, 1]),
    1.0: QTimeLine.FArray([2, -1, 1])
})  # 使用QTimeLine.FArray时应确保所有的FArray长度等宽

s = 0.0
while s < timeline.getMaxFpsTime():
    s += 0.01
    value = timeline.computeFrameAtTime(s)
    x, y, z = value.getTuple()
    print(x, y, z)
```

## 左右节点匹配
`getLRTimeNode`基于时间匹配左右关键帧，其中computeFrameAtTime以此实现。
```python
from .QuModLibs.Modules.Utils.TimeLine import QTimeLine

timeline = QTimeLine({
    0.0: 1,
    0.5: 1.5,
    1.0: -1
})

leftNode, rightNode = timeline.getLRTimeNode(0.6)

# Node参数类型定义
# class Args:
#     def __init__(self, timeValue=0.0, data=None):
#         # type: (float, object | None) -> None
#         self.timeValue = timeValue    # 关键帧
#         self.data = data              # 引用数据
```