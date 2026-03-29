"""
Agent 1 (Codex) 自动化验收测试
Harness 的一部分，Agent 不能修改此文件。
用法：python tests/test_agent1.py
"""
import sys, json
from pathlib import Path

results = []
def check(name, ok, detail=""):
    results.append((name, ok))
    s = "✅ PASS" if ok else "❌ FAIL"
    print(f"  {s} {name}" + (f" — {detail}" if detail else ""))
    return ok

def main():
    print("=" * 60)
    print("Agent 1 (Codex) 验收测试")
    print("=" * 60)

    # Test 1: checkpoint 存在且可加载
    print("\n[Test 1] Checkpoint")
    ckpts = list(Path("checkpoints").glob("**/*.ckpt"))
    if check("checkpoint 存在", len(ckpts) > 0):
        try:
            import torch
            torch.load(ckpts[0], map_location="cpu")
            check("checkpoint 可加载", True)
        except Exception as e:
            check("checkpoint 可加载", False, str(e))

    # Test 2: thresholds.json
    print("\n[Test 2] Thresholds")
    tp = Path("checkpoints/thresholds.json")
    if not tp.exists(): tp = Path("models/thresholds.json")
    if check("thresholds.json 存在", tp.exists()):
        d = json.loads(tp.read_text())
        check("image_threshold > 0", d.get("image_threshold", 0) > 0, f"值:{d.get('image_threshold')}")
        check("pixel_threshold > 0", d.get("pixel_threshold", 0) > 0, f"值:{d.get('pixel_threshold')}")

    # Test 3: 验收集精度
    print("\n[Test 3] 验收集精度")
    acc = Path("data/acceptance")
    ok_imgs = list(acc.glob("ok/*")) if acc.exists() else []
    bad_imgs = list(acc.glob("bad/*")) if acc.exists() else []
    if check("验收集有图", len(ok_imgs) > 0 and len(bad_imgs) > 0):
        try:
            from src.infer_service import PatchCoreInference
            from sklearn.metrics import roc_auc_score
            svc = PatchCoreInference(checkpoint_path=str(ckpts[0]))
            svc.load_model()
            scores, labels = [], []
            for img in ok_imgs:
                scores.append(svc.predict(str(img))["anomaly_score"]); labels.append(0)
            for img in bad_imgs:
                scores.append(svc.predict(str(img))["anomaly_score"]); labels.append(1)
            auroc = roc_auc_score(labels, scores)
            check("AUROC >= 0.90", auroc >= 0.90, f"AUROC={auroc:.4f}")
        except Exception as e:
            check("推理测试", False, str(e))

    # 总结
    print("\n" + "=" * 60)
    p = sum(1 for _, ok in results if ok)
    print(f"结果: {p}/{len(results)} 通过")
    print("✅ Agent 1 验收通过" if p == len(results) else "❌ Agent 1 验收未通过")
    print("=" * 60)
    return 0 if p == len(results) else 1

if __name__ == "__main__":
    sys.exit(main())
