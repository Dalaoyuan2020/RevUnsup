"""
PatchCore 训练脚本

对齐旧版参数：
- backbone: resnet18
- layers: [layer2, layer3]
- coreset_sampling_ratio: 0.1
- num_neighbors: 1
- image_size: 256x256
- batch_size: 1
"""
# TODO: Agent 1 实现完整训练流程
# 核心步骤：
# 1. 加载 Folder/MVTec 格式数据集
# 2. 初始化 Patchcore(backbone=resnet18, coreset_sampling_ratio=0.1)
# 3. Engine.fit() 训练
# 4. 保存 checkpoint
