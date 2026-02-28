# 混合不同框架的加载器创建项目
一个modMain文件允许同时注册多个MOD加载类。

## 混合注册
```python
# -*- coding: utf-8 -*-
from mod.common.mod import Mod
import mod.server.extraServerApi as serverApi
import mod.client.extraClientApi as clientApi
from .QuModLibs.QuMod import *

# QuModLibs
myQuMod = EasyMod()
myQuMod.Server("...")
myQuMod.Client("...")

# 网易原版
@Mod.Binding(name = "MOD_MAIN", version = "1.0.0")
class MOD_MAIN(object):
    @Mod.InitServer()
    def serverInit(self):
        serverApi.RegisterSystem(...)

    @Mod.InitClient()
    def clientInit(self):
        clientApi.RegisterSystem(...)

    @Mod.DestroyServer()
    def serverDestroy(self):
        pass

    @Mod.DestroyClient()
    def clientDestroy(self):
        pass

# 其他
# ...
```
该方案适用于某些复杂扩展支持的业务需求。