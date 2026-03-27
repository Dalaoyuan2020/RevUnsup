"""
ONNX 导出模块

功能：
1. 加载 PatchCore checkpoint
2. 将 feature extractor 导出为 ONNX
3. 将 memory bank (coreset) 单独保存为 .npy
4. 导出阈值配置为 JSON

注意：PatchCore 的 memory bank 不是标准网络层，
不能直接导出到 ONNX，需要拆分处理：
- ONNX: ResNet18 feature extractor 部分
- NPY: memory bank (coreset embeddings)
- JSON: thresholds + config

C++ 部署时重新组装这三个部分。
"""
import argparse
from pathlib import Path


def export_feature_extractor(checkpoint_path: str, output_path: str, opset: int = 14):
    """导出特征提取器为 ONNX"""
    # TODO: Agent 2 实现
    raise NotImplementedError


def export_memory_bank(checkpoint_path: str, output_path: str):
    """导出 memory bank 为 .npy"""
    # TODO: Agent 2 实现
    raise NotImplementedError


def export_deploy_config(checkpoint_path: str, config_path: str, output_path: str):
    """导出部署配置 JSON（含阈值）"""
    # TODO: Agent 2 实现
    raise NotImplementedError


def export_all(checkpoint_path: str, output_dir: str, opset: int = 14):
    """一键导出全部部署文件"""
    output = Path(output_dir)
    output.mkdir(parents=True, exist_ok=True)

    export_feature_extractor(checkpoint_path, str(output / "feature_extractor.onnx"), opset)
    export_memory_bank(checkpoint_path, str(output / "memory_bank.npy"))
    export_deploy_config(checkpoint_path, "configs/patchcore_default.yaml", str(output / "deploy_config.json"))

    print(f"Exported to {output_dir}:")
    print(f"  - feature_extractor.onnx")
    print(f"  - memory_bank.npy")
    print(f"  - deploy_config.json")


def main():
    parser = argparse.ArgumentParser(description="Export PatchCore to ONNX")
    parser.add_argument("--checkpoint", required=True)
    parser.add_argument("--output", default="deploy/")
    parser.add_argument("--opset", type=int, default=14)
    args = parser.parse_args()
    export_all(args.checkpoint, args.output, args.opset)


if __name__ == "__main__":
    main()
