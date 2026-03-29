"""
防线 1：环境检查脚本
所有 Agent 开工前必须跑通此脚本，输出 ALL PASS 才能开始工作。

用法：python scripts/env_check.py
"""
import sys
from pathlib import Path

PASS = "✅ PASS"
FAIL = "❌ FAIL"
results = []


def check(name, condition, detail=""):
    status = PASS if condition else FAIL
    results.append((name, condition))
    print(f"  {status} {name}" + (f" ({detail})" if detail else ""))
    return condition


def main():
    print("=" * 60)
    print("RevUnsup 环境检查")
    print("=" * 60)

    print("\n[1] Python 版本")
    py_ver = sys.version_info
    check("Python >= 3.10", py_ver >= (3, 10), f"当前: {py_ver.major}.{py_ver.minor}")

    print("\n[2] PyTorch")
    try:
        import torch
        check("torch 已安装", True, f"版本: {torch.__version__}")
        check("CUDA 可用", torch.cuda.is_available(),
              f"设备: {torch.cuda.get_device_name(0)}" if torch.cuda.is_available() else "无GPU")
    except ImportError:
        check("torch 已安装", False, "未安装")

    print("\n[3] Anomalib")
    try:
        import anomalib
        check("anomalib 已安装", True, f"版本: {getattr(anomalib, '__version__', 'unknown')}")
    except ImportError:
        check("anomalib 已安装", False, "未安装")

    print("\n[4] OpenCV")
    try:
        import cv2
        check("opencv 已安装", True, f"版本: {cv2.__version__}")
    except ImportError:
        check("opencv 已安装", False, "未安装")

    print("\n[5] ONNX Runtime")
    try:
        import onnxruntime as ort
        check("onnxruntime 已安装", True, f"版本: {ort.__version__}")
        providers = ort.get_available_providers()
        check("CUDA EP 可用", "CUDAExecutionProvider" in providers, f"providers: {providers}")
    except ImportError:
        check("onnxruntime 已安装", False, "未安装")

    print("\n[6] NumPy")
    try:
        import numpy as np
        check("numpy 已安装", True, f"版本: {np.__version__}")
    except ImportError:
        check("numpy 已安装", False, "未安装")

    print("\n[7] 验收数据集")
    acc_dir = Path("data/acceptance")
    has_ok = (acc_dir / "ok").exists() and len(list((acc_dir / "ok").glob("*"))) > 0 if acc_dir.exists() else False
    has_bad = (acc_dir / "bad").exists() and len(list((acc_dir / "bad").glob("*"))) > 0 if acc_dir.exists() else False
    check("data/acceptance/ok/ 有图片", has_ok)
    check("data/acceptance/bad/ 有图片", has_bad)

    print("\n[8] 配置文件")
    check("patchcore_default.yaml", Path("configs/patchcore_default.yaml").exists())
    check("acceptance_criteria.yaml", Path("configs/acceptance_criteria.yaml").exists())

    print("\n" + "=" * 60)
    passed = sum(1 for _, ok in results if ok)
    total = len(results)
    all_pass = passed == total
    print(f"结果: {passed}/{total} 通过")
    if all_pass:
        print("✅ ALL PASS — 可以开始工作")
    else:
        print("❌ 有未通过项 — 请先修复再开工")
        for name, ok in results:
            if not ok:
                print(f"  - {name}")
    print("=" * 60)
    return 0 if all_pass else 1


if __name__ == "__main__":
    sys.exit(main())
