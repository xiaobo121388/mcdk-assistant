# 权重工具
用于基于weight值产生随机结果，weight越高被选取的可能性越大。
```python
from .QuModLibs.Modules.Utils.WeightUtil import QWeightSelector, QWeightPool
```

## QWeightSelector 简易选择器
基础的权重选择器。
```python
from .QuModLibs.Modules.Utils.WeightUtil import QWeightSelector

selector = QWeightSelector()
selector.add("苹果", weight=1) # 20%概率被选中
selector.add("香蕉", weight=4) # 80%概率被选中 

print(selector.randomSelection())   # 随机抽取
```

## QWeightPool 分表合并选择器
适用于大量重复weight的权重池。
```python
from .QuModLibs.Modules.Utils.WeightUtil import QWeightPool

selector = QWeightPool()
selector.add("苹果", weight=1)
selector.add("香蕉", weight=4)

print(selector.randomSelection())   # 随机抽取
```