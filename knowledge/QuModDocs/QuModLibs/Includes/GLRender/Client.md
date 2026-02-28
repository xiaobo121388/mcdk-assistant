# GLRender 客户端静态管理 <Badge type="tip" text="^1.3.5" />
客户端的静态业务管理。

## 注册静态资源
可以通过客户端静态注册减少服务端同步数据包大小。
```python
# Client.py
from .QuModLibs.Client import *
from .QuModLibs.Include.GL_Render.Client import PLAYER_RES_SERVICE, JSONRenderData

# 注册静态资源 自动同步所有玩家(需在玩家渲染完毕之前注册)
PLAYER_RES_SERVICE.access().setPlayerStaticGlobalRes(
    JSONRenderData(
        textures = {
            "xxx": "xxxx"
        },
        geometry = {
            ...
        },
        ...
    )
)

```
::: tip 覆盖优先级
当同时使用静态和动态资源管理时，动态优先覆盖静态资源。
:::

## Query同步监听
通过监听Query同步变化量 可以扩写一些业务功能
```python
# Client.py
from .QuModLibs.Client import *
from .QuModLibs.Modules.Services.Client import BaseService
from .QuModLibs.Include.GL_Render.Client import MOB_QUERYS_UPDATE_EVENTS

@BaseService.Init
class LocalRender(BaseService):
    def __init__(self):
        BaseService.__init__(self)

    @BaseService.ServiceListen(MOB_QUERYS_UPDATE_EVENTS)
    def GLR_EntityQuerysUpdate(self, data=MOB_QUERYS_UPDATE_EVENTS()):
        targetQuery = "query.mod.xxx"
        if targetQuery in data.queryMapRef:
            print("{} 的Query发生变化: {}".format(data.entityId, data.queryMapRef[targetQuery]))
        # queryMapRef是一个引用dict 在此事件下可以通过修改重定向query值
```

## 本地效果监听
`MOB_QUERYS_UPDATE_EVENTS`监听的封装版本，需要指定绑定某个query。

```python
# Client.py
from .QuModLibs.Client import *
from .QuModLibs.Include.GL_Render.Client import MOB_QUERYS_UPDATE_EVENTS, PLAYER_RES_SERVICE

@PLAYER_RES_SERVICE.localPlayerEffectListener("query.mod.xxx")
def STATIC_LISTNER_HANDLER1(entityId, queryValue):
    # 使用该方案处理效果表现 可以避免额外的数据包发送 同时支持有限渲染区域响应(不会触发渲染区块外的变化)
    if queryValue == 1:
        # 播放特效1
        pass
    elif queryValue == 2:
        # 播放特效2
        pass

```