"""
全链路集成门禁测试 — 哪步 FAIL 定位到哪个 Agent
Harness 最终防线。
用法：python tests/integration_test.py
"""
import sys, json
from pathlib import Path

results = []
def check(name, ok, detail="", responsible=""):
    results.append((name, ok, responsible))
    s = "✅ PASS" if ok else "❌ FAIL"
    r = f" [→{responsible}]" if responsible and not ok else ""
    print(f"  {s} {name}" + (f" — {detail}" if detail else "") + r)
    return ok

def main():
    print("=" * 60)
    print("RevUnsup 全链路集成测试")
    print("=" * 60)

    # Step 1: Agent 1 产物
    print("\n[Step 1] Agent 1 产物")
    ckpts = list(Path("checkpoints").glob("**/*.ckpt"))
    if not check("checkpoint 存在", len(ckpts)>0, responsible="Agent 1 (Codex)"):
        return 1
    try:
        import torch; torch.load(ckpts[0], map_location="cpu")
        check("checkpoint 可加载", True, responsible="Agent 1")
    except Exception as e:
        check("checkpoint 可加载", False, str(e), "Agent 1 (Codex)"); return 1

    # Step 2: Python 推理精度
    print("\n[Step 2] Python 推理精度")
    acc = Path("data/acceptance")
    ok_imgs = list(acc.glob("ok/*")) if acc.exists() else []
    bad_imgs = list(acc.glob("bad/*")) if acc.exists() else []
    if check("验收集完整", len(ok_imgs)>0 and len(bad_imgs)>0):
        try:
            from src.infer_service import PatchCoreInference
            from sklearn.metrics import roc_auc_score
            svc = PatchCoreInference(checkpoint_path=str(ckpts[0])); svc.load_model()
            py_scores, scores, labels = {}, [], []
            for img in ok_imgs:
                r = svc.predict(str(img)); py_scores[img.name]=r["anomaly_score"]
                scores.append(r["anomaly_score"]); labels.append(0)
            for img in bad_imgs:
                r = svc.predict(str(img)); py_scores[img.name]=r["anomaly_score"]
                scores.append(r["anomaly_score"]); labels.append(1)
            auroc = roc_auc_score(labels, scores)
            check("AUROC >= 0.90", auroc>=0.90, f"AUROC={auroc:.4f}", "Agent 1 (Codex)")
            Path("results").mkdir(exist_ok=True)
            Path("results/python_scores.json").write_text(json.dumps(py_scores, indent=2))
        except Exception as e:
            check("Python 推理", False, str(e), "Agent 1+2")

    # Step 3: ONNX 一致性
    print("\n[Step 3] ONNX 一致性")
    check("ONNX 存在", Path("deploy/feature_extractor.onnx").exists(), responsible="Agent 2 (Claude Code)")

    # Step 4: C++ 一致性
    print("\n[Step 4] C++ vs Python")
    cpp_p = Path("results/cpp_scores.json")
    if check("C++ 分数文件存在", cpp_p.exists(), responsible="Agent 3 (OpenCode)"):
        cpp = json.loads(cpp_p.read_text())
        max_diff = max((abs(py_scores.get(k,0)-cpp[k]) for k in cpp), default=999)
        check("diff < 0.01", max_diff<0.01, f"最大差值:{max_diff:.6f}", "Agent 3 (OpenCode)")

    # Step 5: 延迟
    print("\n[Step 5] C++ 延迟")
    perf_p = Path("results/performance_report.json")
    if check("性能报告存在", perf_p.exists(), responsible="Agent 3 (OpenCode)"):
        perf = json.loads(perf_p.read_text())
        check("延迟 <= 100ms", perf.get("average_latency_ms",999)<=100,
              f"{perf.get('average_latency_ms',999):.1f}ms", "Agent 3 (OpenCode)")

    # 总结
    print("\n" + "=" * 60)
    p = sum(1 for _,ok,_ in results if ok)
    print(f"结果: {p}/{len(results)} 通过")
    if p == len(results):
        print("\n✅ ALL PASS — 可以切换到真实数据")
    else:
        print("\n❌ 失败项:")
        for name, ok, resp in results:
            if not ok: print(f"  ❌ {name} → {resp}")
    print("=" * 60)
    return 0 if p == len(results) else 1

if __name__ == "__main__":
    sys.exit(main())
