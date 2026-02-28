# 新实体组件模块 <Badge type="tip" text="^1.3.0" />
通用的实体组件模块，用于处理实体相关的业务逻辑。
```python
from .QuModLibs.Modules.EntityComps.Server import QBaseEntityComp
# from .QuModLibs.Modules.EntityComps.Client import QBaseEntityComp
```
::: warning 注意事项
所有的组件类内部都是无序管理的，因此不要试图估测多个组件之间的执行顺序。
:::