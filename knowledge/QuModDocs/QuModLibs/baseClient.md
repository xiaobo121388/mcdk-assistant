# 基本客户端API
包含在基础客户端模块的默认功能，如Request。

## IsServerUser 常量
在QuMod加载器初始化环节将辅助标记房主玩家。
```python
# Client.py
from .QuModLibs.Client import *

@Listen(Events.ClientItemTryUseEvent)
def ClientItemTryUseEvent(args):
    if not IsServerUser:
        # 非房主成员不得执行以下代码
        return
    itemDict = args["itemDict"]
    itemName = itemDict["itemName"]
    if itemName == "minecraft:diamond_sword":
        # 实现房主独占逻辑
        ...
```
::: warning 网络游戏须知
在联机大厅/服务器类网络游戏下，将不存在"玩家房主"，此时该常量将返回False。
:::

## Request 远程请求
相较于Call，它支持回调式返回请求结果。
```python
# Client.py
from .QuModLibs.Client import *

# 函数定义
# def Request(Key, args=tuple(), kwargs={}, onResponse=lambda *_: None):
#     pass

def callBack(data):
    print("返回数据结果: {}".format(data))

Request("ServerFunc", ("张三",), {}, callBack)
# 注意: 单个args参数时请使用 (xxx,) 而不是 (xxx) 后者不会视为元组 
```

## CallOTClient 调用其他玩家
```python
# Client.py
from .QuModLibs.Client import *

CallOTClient(playerId, "FuncName", ...) # 将发包到服务端并转发到其他玩家客户端
```