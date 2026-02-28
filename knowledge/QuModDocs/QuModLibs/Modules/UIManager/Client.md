# UI 界面管理器
UI界面管理器用于解决因维度切换/其他因素产生的 不期望的UI销毁问题

```python
# 通常来说使用默认已经创建好的uiManager即可不需要自己构造新的
from .QuModLibs.Modules.UIManager.Client import uiManager, UIManager
```

## UIManager 界面托管
托管下的界面在维度切换后依然会重新建立
```python
from .QuModLibs.UI import *
from .QuModLibs.Modules.UIManager import uiManager

@EasyScreenNodeCls.Binding("myUI.main")
class MyUICls(EasyScreenNodeCls):
	def __init__(self):
		print("界面已创建")

uiManager.creatUI(MyUICls)  # 此处应使用CLASS引用而非实例
uiManager.removeUI(MyUICls) # 删除UI
```
::: warning 注意事项
使用 UIManager 托管创建的UI也应该使用管理器的removeUI方法销毁而不是UI自己的SetRemove
:::