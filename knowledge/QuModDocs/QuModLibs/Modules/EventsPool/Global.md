# 事件池化监听 <Badge type="tip" text="^1.3.0" />
常规的动态事件监听`ListenForEvent`，`UnListenForEvent`受框架及网易的复杂封装导致调度开销较大，不适合高频率使用，因此诞生了池化事件组。
```python
from .QuModLibs.Modules.EventsPool.Server import POOL_ListenForEvent, POOL_UnListenForEvent
# from .QuModLibs.Modules.EventsPool.Client import POOL_ListenForEvent, POOL_UnListenForEvent
```

## 演示代码
```python
from .QuModLibs.Modules.EventsPool.Server import POOL_ListenForEvent, POOL_UnListenForEvent

def TEST_FUNC():
    pass

for _ in range(100000): # 模拟高频场景
    # 注: 该函数会自动判断重复 同一个回调函数不会在单个事件下注册多次
    POOL_ListenForEvent("OnScriptTickServer", TEST_FUNC)
    POOL_UnListenForEvent("OnScriptTickServer", TEST_FUNC)
```
池化事件监听内部使用`有序哈希表`完成，因此在确保高性能的同时依然能保持良好的调用顺序。