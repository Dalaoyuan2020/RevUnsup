"""
Agent 2 (Claude Code) 自动化验收测试
Harness 的一部分，Agent 不能修改此文件。
用法：python tests/test_agent2.py
"""
import sys, json
from pathlib import Path
import numpy as np

results = []
def check(name, ok, detail=""):
    results.append((name, ok))
    s = "✅ PASS" if ok else "❌ FAIL"
    print(f"  {s} {name}" + (f" — {detail}" if detail else ""))
    return ok

def main():
    print("=" * 60)
    print("Agent 2 (Claude Code) 验收测试")
    print("=" * 60)

    # Test 1: 导出文件存在
    print("\n[Test 1] 导出文件")
    d = Path("deploy")
    check("feature_extractor.onnx", (d/"feature_extractor.onnx").exists())
    check("memory_bank.npy", (d/"memory_bank.npy").exists())
    check("deploy_config.json", (d/"deploy_config.json").exists())

    # Test 2: ONNX 可加载
    print("\n[Test 2] ONNX 可加载")
    if (d/"feature_extractor.onnx").exists():
        try:
            import onnxruntime as ort
            sess = ort.InferenceSession(str(d/"feature_extractor.onnx"))
            shape = sess.get_inputs()[0].shape
            check("ONNX 加载成功", True, f"输入shape: {shape}")
        except Exception as e:
            check("ONNX 加载", False, str(e))

    # Test 3: memory_bank shape
    print("\n[Test 3] Memory Bank")
    if (d/"memory_bank.npy").exists():
        mb = np.load(str(d/"memory_bank.npy"))
        check("shape 二维", len(mb.shape)==2, f"shape: {mb.shape}")
        if len(mb.shape)==2:
            check("N > 0", mb.shape[0]>0, f"N={mb.shape[0]}")
            check("C > 0", mb.shape[1]>0, f"C={mb.shape[1]}")

    # Test 4: deploy_config 完整
    print("\n[Test 4] Config")
    if (d/"deploy_config.json").exists():
        cfg = json.loads((d/"deploy_config.json").read_text())
        check("含 image_threshold", "image_threshold" in cfg)
        check("含 pixel_threshold", "pixel_threshold" in cfg)

    # Test 5: Python vs ONNX 一致性（预留，Agent 2 交付后激活）
    print("\n[Test 5] Python vs ONNX 一致性")
    print("  ⏳ 待 Agent 2 完整交付后激活")

    # 总结
    print("\n" + "=" * 60)
    p = sum(1 for _, ok in results if ok)
    print(f"结果: {p}/{len(results)} 通过")
    print("✅ Agent 2 验收通过" if p == len(results) else "❌ Agent 2 验收未通过")
    print("=" * 60)
    return 0 if p == len(results) else 1

if __name__ == "__main__":
    sys.exit(main())
