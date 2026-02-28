# 创建一个QuMod项目
您可以通过Gitee/其他平台下载QuModLibs的源代码并且将其按照特定的项目结构接入Mod项目。
- [Gitee项目地址](https://gitee.com/bili_zero123/qu_mod_libs) 

```
├── 行为包
│   └── 脚本目录
│       └── QuModLibs(这里)
|           ├── __init__.py
|           └── ...
│       ├── __init__.py
│       ├── modMain.py
│       ├── Server.py
│       └── Client.py
```

## MOD初始化
```python
# modMain.py (所在文件 后续均以此描述)
from .QuModLibs.QuMod import * # 导入 QuModLibs.QuMod 的全部功能
myMod = EasyMod()             # 便捷的MOD构建类

# 服务端与客户端注册
myMod.Server("Server")
myMod.Client("Client")
```
```text
QuMod的注册模块需要使用from .xxx import * 语句完整的导入加载。

注册仅用于初始化在对应的端侧线程下加载特定文件 并不是所有文件都一定要在初始化阶段注册
运行时的import同样生效。
```
<!-- ::: tip 注意事项
QuMod的注册模块需要使用from .xxx import * 语句完整的导入加载。

注册仅用于初始化在对应的端侧线程下加载特定文件 并不是所有文件都一定要在初始化阶段注册
运行时的import同样生效。
::: -->

## 动态注册脚本
动态注册可以方便的加载需要的文件在对应系统端
```python
# modMain.py
from .QuModLibs.QuMod import *
myMod = EasyMod()

# 动态注册服务端文件(相对路径)
myMod.Server("Server") # 将加载 脚本目录/Server.py
# myMod.Server("Dir.Test") # 多级目录使用.区分
myMod.Client("Client")  # 客户端同理
```

## 静态初始化逻辑 <Badge type="tip" text="^1.3.0" />
QuMod允许开发者自行定制服务端/客户端的初始化调用逻辑 以增加其灵活性。
```python
# modMain.py
from .QuModLibs.QuMod import *

# 服务端初始化时执行
def SERVER_INIT():
    # 您可以在此处直接加载文件 而非动态注册
    import Server

REG_SERVER_INIT_CALL(SERVER_INIT)

# 客户端初始化时执行
def CLIENT_INIT():
    # 亦或者注册原生系统类 混合项目开发
    clientApi.RegisterSystem(...)

REG_CLIENT_INIT_CALL(CLIENT_INIT)

# 一个特殊的案例 演示了如何通过上下文信息仅对房主注册特定客户端
def SERVER_CLIENT():
    if IN.IsServerUser:
        myMod.Client("Client")

REG_CLIENT_INIT_CALL(SERVER_CLIENT)
```
通过以上的任意方式即可完成端侧系统文件注册。