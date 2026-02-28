# 通用系统端API
适用于服务端/客户端的通用基本功能，其中 `serverApi`，`clientApi`，`levelId` 之类的常用功能将默认被包含以便快速开发。

## 游戏事件监听
事件监听是MOD开发的基础组成部分 QuMod提供了多种事件处理机制选择。

### 装饰器静态监听
```python
# Server/Client.py
from .QuModLibs.Server import *
# from .QuModLibs.Client import *

@Listen("OnScriptTickServer")
def OnScriptTickServer(_={}):
    # 此处将监听游戏Tick 持续执行该函数
    # 静态监听的内容不支持运行时动态取消 在游戏关闭时自动销毁
    print("tick!!")

# 也可以通过内置的Events类表注册监听 方便开发工具的补全提示
@Listen(Events.OnCarriedNewItemChangedServerEvent)
def OnCarriedNewItemChangedServerEvent(args={}):
    # type: (dict) -> None
    playerId = args["playerId"] # 触发事件的玩家
    comp = serverApi.GetEngineCompFactory().CreateCommand(levelId)
    comp.SetCommand("/say 我切换了手持物品", playerId)

```
::: tip 游戏事件
相关游戏事件触发时机及参数可查询[网易我的世界API文档表](https://mc.163.com/dev/mcmanual/mc-dev/mcdocs/1-ModAPI/%E4%BA%8B%E4%BB%B6/%E4%BA%8B%E4%BB%B6%E7%B4%A2%E5%BC%95%E8%A1%A8.html?catalog=1)。
:::
### 动态监听与反监听
```python
# Server/Client.py
from .QuModLibs.Server import *
# from .QuModLibs.Client import *

# 在class中使用 可手动管理注销监听
class TestCls:
    def __init__(self):
        # 初始化时注册监听
        ListenForEvent("OnScriptTickServer", self, self.onTick)
    
    def onTick(self, args={}):
        # tick事件
        print("tick")

    def free(self):
        # 释放方法 销毁监听(反监听)
        UnListenForEvent("OnScriptTickServer", self, self.onTick)
```
## 游戏销毁处理
当游戏关闭时 一些业务逻辑需要做回收处理 例如用户数据保存。
### 装饰器声明 <Badge type="tip" text="^1.3.0" />
```python
# 双端通用
@DestroyFunc
def onGameClose():
    print("当游戏关闭时触发该函数")

```

### 旧版方案 <Badge type="danger" text="已废弃" />
```python
# 定义QuDestroy函数 将在游戏关闭时调用(仅适用于动态注册的文件)
def QuDestroy():
    print("当游戏关闭时触发该函数")

```
## 跨系统端通信
跨端之间的通信需要通过网络发包的方式完成，QuMod中简化了此流程。

### AllowCall 调用权限声明
```python
# 双端通用
@AllowCall
def thisAFucntion():
    # 使用AllowCall装饰器声明的函数将被登记到通信调用中(不影响常规调用)以便远程调用
    pass

```
### Call 调用远程可执行对象
```python
# 客户端
Call("函数名", ...) # 可以携带任意数量的参数

# 服务端
Call("玩家ID", "函数名", ...)
# 服务端的Call多了一个玩家Id参数 用于指定玩家 其中 "*" 作为保留字段用于全体玩家广播
# 通过Call + AllowCall即可轻易的完成双端通信
```
::: warning 注意事项
远程通信涉及I/O开销，请勿高频调用 (本地环境下Call为本机环回)。
:::

### MultiClientsCall 批量调用 <Badge type="tip" text="^1.2.0" />
```python
# MultiClientsCall为服务端独占功能 用于给多个玩家同时发包调用对立客户端函数
# 该方法相比for循环+Call的批量发包性能更好
MultiClientsCall(["-123", "-456", ...], "函数名", ...)
```

## Entity 实体类
QuMod提供的一个简易的实体封装类 用于快捷的操作实体属性。

```python
from .QuModLibs.Server import *
entityId = "12345" 						# 假定一个实体ID,实际项目开发时可自行获取

print(Entity(entityId).Health.Value) 	# 获取实体生命值
print(Entity(entityId).Health.Max) 		# 获取实体最大生命值

Entity(entityId).Health.Value = 6666 	# 设置当前实体生命值为6666

print(Entity(entityId).Pos) 			# 获取实体Pos位置(x,y,z)
print(Entity(entityId).FootPos) 		# 获取实体脚部位置
Entity(entityId).Pos = (0,0,0) 			# 设置实体位置

print(Entity(entityId).Rot) 			# 获取实体旋转角度
Entity(entityId).Rot = (10,-30) 		# 设置实体旋转角度

print(Entity(entityId).Identifier) 		# 获取实体Identifier

print(Entity(entityId).Dm) 				# 获取实体所在维度

print(Entity(entityId).DirFromRot) 		# 获取实体的面向向量
...

```

## 数据储存管理 <Badge type="warning" text="即将废弃" />
QuMod提供了便捷的数据储存管理，`1.4.0+` 高版本推荐使用 `DataStore` 模块。
```python
from .QuModLibs.Server import *

# 对于客户端还将多一项参数 指定是当前地图还是全局
@QuDataStorage.AutoSave(version = 1)    # version有助于数据结构大型变更时放弃旧版数据
class Data:                             # 当属性新旧类型不同时也将会放弃旧的类型数据
    count = 0   # 记录玩家长按物品使用次数

@Listen(Events.ServerItemTryUseEvent)
def ServerItemTryUseEvent(_={}):
    Data.count += 1     # float / int 将被一致视为number类型
    print(Data.count)
    # QuDataStorage.saveData()  手动调用更新(在重要数据时确保及时储存)
```
::: tip 注意事项
QuDataStorage默认只在游戏关闭时调用储存，如果玩家通过大退出后台/非常规退出，可能丢失保存，因此在一些重要数据保存时开发者应手动调用储存更新。
:::