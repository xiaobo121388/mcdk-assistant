# 简易移动光源教程

> By 先知

移动光源是一个经典的模组，可以使生物或玩家处于发光状态。

目前的最优解是使用着色器来完成，但这对开发技术要求较高，并且只适用于玩家移动光源，无法支持多个生物同时发光。因此，我们选择让**生物放置光源方块**的方式来实现该功能。

关于生物功能的管理，可以使用 **QuMod 的实体组件类**。

------

## 第一步：定义实体组件类
实体组件类提供了高效组织管理的实体逻辑解决方案。
```python
# 标记该组件类：无论实体是否处于渲染区域，组件都会被加载，不会因为不可见而被回收。
@QBaseEntityComp.setFlags(QEntityCompFlags.IGNORE_RENDERING_STATUS)
class EntityLight(QBaseEntityComp):
    """ 实体光源 """
    _map = {}
    lightBlockName = "minecraft:light_block"
    def onBind(self):
        QBaseEntityComp.onBind(self)
        self.addTimer(self.Timer(self.placeLightBlock, time = 0.1, loop = True))
        EntityLight._map[self.entityId] = self
    
    def onUnBind(self):
        if self.entityId in EntityLight._map:
            del EntityLight._map[self.entityId]
        
    def placeLightBlock(self):
        x, y, z = Entity(self.entityId).Pos
        pos = floor(x), floor(y), floor(z)
        dm = Entity(self.entityId).Dm

        comp = factory.CreateBlockInfo(self.entityId)
        blockDict = comp.GetBlockNew(pos, dm)
        if blockDict["name"] == "minecraft:air":
            comp.SetBlockNew(pos, {"name":EntityLight.lightBlockName, "aux":15}, 0, dm, updateNeighbors = False)
```
- 通过 `self.addTimer` 添加了一个定时器，设置为每隔 0.1 秒执行一次，用于持续循环放置光源方块。

- 语句 `pos = floor(x), floor(y), floor(z)` 的作用是将实体的浮点坐标取整，转换为对应的方块格子坐标，确保放置光源时精确定位到当前所在的方块位置。


## 第二步，回收上次放置的光源方块
通过定义变量来记录上次放置的位置与维度，随后每次放置前都先清除上次放置的光源方块。

```python
    def onBind(self):
        ...
        self.lastLightPos = None
        self.lastLightDm = -1
        
    def placeLightBlock(self):
        ...
        if comp.SetBlockNew(pos, {"name":EntityLight.lightBlockName, "aux":15}, 0, dm, updateNeighbors = False): # 当成功清掉光源方块后,才更新
            self.lastLightPos = pos
            self.lastLightDm = dm
                
    def unPlaceLightBlock(self):
        """ 回收光源方块 """
        if self.lastLightPos and self.lastLightDm != -1:
            comp = factory.CreateBlockInfo(self.entityId)
            blockDict = comp.GetBlockNew(self.lastLightPos, self.lastLightDm)
            if blockDict["name"] == EntityLight.lightBlockName:
                if comp.SetBlockNew(self.lastLightPos, {"name":"minecraft:air", "aux":0}, 0, self.lastLightDm, updateNeighbors = False): # 成功回收
                    self.lastLightPos = None
                    self.lastLightDm = -1   
```



## 第三步，处理边界情况
当遇到实体所在区块被卸载（死亡，换维度）的情况时，依然要保证光源方块的回收。于是我们监听区块卸载事件进行处理。并在实体组件类的onUnBind方法中也回收一次光源方块。

```python
    def onUnBind(self):
        ...
        self.unPlaceLightBlock()

    @staticmethod
    def getAllComp():
        # type: () -> dict[str, EntityLight]
        return EntityLight._map
    
    @staticmethod
    @Listen(Events.ChunkAcquireDiscardedServerEvent)
    def EntityLightChunkAcquireDiscardedServerEvent(args):
        """ 区块卸载事件,用于回收一些异常方块 """
        data = Events.ChunkAcquireDiscardedServerEvent(args)

        for k, v in EntityLight.getAllComp().items():
            if v.lastLightDm != data.dimension:
                continue
            if v.lastLightPos:
                x, y, z = v.lastLightPos
                if data.chunkPosX * 16 <= x <= data.chunkPosX * 16 + 15 and data.chunkPosZ * 16 <= z <= data.chunkPosZ * 16 + 15: # 判断方块是否在被卸载的区块内部
                    v.unPlaceLightBlock()

```



## 第四步，优化方块的放置

由于放置光源方块容易被草丛，藤蔓挡住，于是我们可以定义一个集合，存储允许被替换的方块：

```python
@QBaseEntityComp.setFlags(QEntityCompFlags.IGNORE_RENDERING_STATUS)
class EntityLight(QBaseEntityComp):
    """ 实体光源 """
    ...
    canBreakBlock = {
        "minecraft:air",
        "minecraft:tallgrass",
        "minecraft:shortgrass",
        "minecraft:vine"
    }
    
    def placeLightBlock(self):
		...
        # if blockDict["name"] == "minecraft:air:
        #    |
        #    |
        #    V
        if blockDict["name"] in EntityLight.canBreakBlock: # 将原先判断是否为空气的代码,改为判断是否在集合内
		...
```

> [!TIP]
>
> 关于为何使用集合而不是列表。由于列表是线性查询，效率是O（n），而集合是哈希查询，效率O（1），因此集合在查找元素方面会比列表快一些。即使在小数量级下并不明显。



如果一切正常，你的代码应该类似这样：

```python
@QBaseEntityComp.setFlags(QEntityCompFlags.IGNORE_RENDERING_STATUS)
class EntityLight(QBaseEntityComp):
    """ 实体光源 """
    _map = {}
    lightBlockName = "minecraft:light_block"
    canBreakBlock = {
        "minecraft:air",
        "minecraft:tallgrass",
        "minecraft:shortgrass",
        "minecraft:vine"
    }

    def onBind(self):
        QBaseEntityComp.onBind(self)
        self.lastLightPos = None
        self.lastLightDm = -1
        self.addTimer(self.Timer(self.placeLightBlock, time = 0.1, loop = True))
        EntityLight._map[self.entityId] = self
    
    def placeLightBlock(self):
        x, y, z = Entity(self.entityId).Pos
        pos = floor(x), floor(y), floor(z)
        dm = Entity(self.entityId).Dm
        if pos != self.lastLightPos or dm != self.lastLightDm:
            self.unPlaceLightBlock()
        comp = factory.CreateBlockInfo(self.entityId)
        blockDict = comp.GetBlockNew(pos, dm)
        if blockDict["name"] in EntityLight.canBreakBlock:
            if comp.SetBlockNew(pos, {"name":EntityLight.lightBlockName, "aux":15}, 0, dm, updateNeighbors = False):
                self.lastLightPos = pos
                self.lastLightDm = dm
    
    def unPlaceLightBlock(self):
        """ 回收光源方块 """
        if self.lastLightPos and self.lastLightDm != -1:
            comp = factory.CreateBlockInfo(self.entityId)
            blockDict = comp.GetBlockNew(self.lastLightPos, self.lastLightDm)
            if blockDict["name"] == EntityLight.lightBlockName:
                if comp.SetBlockNew(self.lastLightPos, {"name":"minecraft:air", "aux":0}, 0, self.lastLightDm, updateNeighbors = False): # 成功回收
                    self.lastLightPos = None
                    self.lastLightDm = -1        

    def onUnBind(self):
        self.unPlaceLightBlock()
        if self.entityId in EntityLight._map:
            del EntityLight._map[self.entityId]

    @staticmethod
    def getAllComp():
        # type: () -> dict[str, EntityLight]
        return EntityLight._map
    
    @staticmethod
    @Listen(Events.ChunkAcquireDiscardedServerEvent)
    def EntityLightChunkAcquireDiscardedServerEvent(args):
        """ 区块卸载事件,用于回收一些异常方块 """
        data = Events.ChunkAcquireDiscardedServerEvent(args)

        for k, v in EntityLight.getAllComp().items():
            if v.lastLightDm != data.dimension:
                continue
            if v.lastLightPos:
                x, y, z = v.lastLightPos
                if data.chunkPosX * 16 <= x <= data.chunkPosX * 16 + 15 and data.chunkPosZ * 16 <= z <= data.chunkPosZ * 16 + 15:
                    v.unPlaceLightBlock()       
```

## 第五步，使用

我们可以在需要实体发光的时候为实体添加该组件。不需要了就移除该组件

```python
class EntityLight(QBaseEntityComp):
    ...

EntityLight.create(entityId) # 添加

comp = EntityLight.getComp(entityId) # 销毁
if comp:
    comp.unBind()
```

如此，便完成了一个简易移动光源。


## 小结

- 本次使用实体组件类完成了移动光源的功能。如你所见，实体组件类是一个面向对象的，易于管理的功能制作方式，可以极大增强项目的维护性。
- 关于放置光源方块的移动光源，即使我们已经处理了很多边界情况，依然会出现各种问题。有能力的开发者可以尝试用着色器制作，将带来更好的效果
