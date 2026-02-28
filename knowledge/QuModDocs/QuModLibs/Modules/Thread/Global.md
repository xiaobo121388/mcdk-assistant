# 线程池模块 <Badge type="tip" text="^1.3.0" />
在一些高频子线程任务处理时，实时分配线程带来的调度开销极大，线程池可以提前分配一定的线程并通过队列安全的处理待执行任务。

```python
# 通用线程池
from .QuModLibs.Modules.Thread.Util import QThreadPool
# 游戏线程通信解决方案
from .QuModLibs.Modules.Thread.Server import RUN_IN_MAIN_THREAD
# from .QuModLibs.Modules.Thread.Client import RUN_IN_MAIN_THREAD
```

## 使用线程池
```python
from .QuModLibs.Server import *
from .QuModLibs.Modules.Thread.Util import QThreadPool
from .QuModLibs.Modules.Thread.Server import RUN_IN_MAIN_THREAD

threadPool = QThreadPool(2).start() # 一个线程池至少分配一个线程

def work(stopTime=2.0):
    from time import sleep
    sleep(stopTime)  # 模拟阻塞任务

    def backGameThread():
        for entityId in serverApi.GetEngineActor():
            # 调用游戏API 获取世界实体
            print(entityId)

    # 在需要涉及游戏API时调用RUN_IN_MAIN_THREAD函数提交到游戏线程执行队列
    RUN_IN_MAIN_THREAD(backGameThread)

threadPool.addTask(lambda: work(2.0))
```
::: tip 同理
`RUN_IN_MAIN_THREAD` 也可以在常规分配的线程中使用。
:::