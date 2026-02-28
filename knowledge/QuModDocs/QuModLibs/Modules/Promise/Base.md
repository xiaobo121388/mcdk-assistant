# 异步组件模块 <Badge type="tip" text="^1.4.0" />
通用的异步组件模块 用于处理异步相关的业务逻辑。
```python
from .QuModLibs.Modules.Promise.Server import *
# from .QuModLibs.Modules.Promise.Client import *
```

## 异步 sleepAsync 组件
通过 `asyncRunner` 装饰器可修饰函数以支持异步等待。
```python
# 每过一秒打印一次 'async sleep'
@asyncRunner
def loopLog():
    for i in range(3):
        print("async sleep")
        yield sleepAsync(1.0) 
```

::: tip 注意事项
只有被 asyncRunner 装饰的函数才能使用异步组件。
:::

## 异步 event 组件
Promise模块中提供了一个 event 组件 用于等待某个事件的触发。
```python
# 等待某个事件
@asyncRunner
def awaitEvent(entityId):
    def eventBack(args):
        if args["entityId"] == entityId:
            return True
    """
        每次触发等待的事件时都会调用callBack的函数
        如果callBack返回True这个promise成功 value为dict参数
        如果超过最大时间, promise失败 value 为 None
        参数:
            @name [str]: 需要等待的事件名
            @callBack [function]: 事件触发时的回调函数
            @time [float | int]: 最大等待时间
        返回:
            [bool]: promise是否成功
            [dict | None]: 最后一次触发事件时的参数
    """
    state, value = yield waitEvent("ActuallyHurtServerEvent", eventBack, 3.0)
    # 根据promise的状态执行后续代码
```





