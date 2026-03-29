"""
Agent 3 (OpenCode) 自动化验收测试
Harness 的一部分，Agent 不能修改此文件。
用法：python tests/test_agent3.py
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
    print("Agent 3 (OpenCode) 验收测试")
    print("=" * 60)

    # Test 1: DLL 存在
    print("\n[Test 1] DLL")
    dll_paths = [Path("deploy/dll_wrapper/build/anomalyDet.dll"),
                 Path("deploy/dll_wrapper/anomalyDet.dll"),
                 Path("deploy/anomalyDet.dll")]
    found = next((p for p in dll_paths if p.exists()), None)
    check("anomalyDet.dll 存在", found is not None,
          f"路径: {found}" if found else "未找到")

    # Test 2: C++ vs Python 一致性
    print("\n[Test 2] C++ vs Python 一致性")
    cpp_p = Path("results/cpp_scores.json")
    py_p = Path("results/python_scores.json")
    if check("C++ 分数文件存在", cpp_p.exists()) and check("Python 分数文件存在", py_p.exists()):
        cpp = json.loads(cpp_p.read_text())
        py = json.loads(py_p.read_text())
        max_diff = max(abs(cpp[k]-py[k]) for k in cpp if k in py) if cpp else 999
        check("diff < 0.01", max_diff < 0.01, f"最大差值: {max_diff:.6f}")

    # Test 3: 推理延迟
    print("\n[Test 3] 推理延迟")
    perf_p = Path("results/performance_report.json")
    if check("性能报告存在", perf_p.exists()):
        perf = json.loads(perf_p.read_text())
        avg = perf.get("average_latency_ms", 999)
        check("平均延迟 <= 100ms", avg <= 100, f"{avg:.1f} ms")
        check("特征提取 < 50ms", perf.get("feature_extraction_ms", 999) < 50)
        check("KNN < 40ms", perf.get("knn_search_ms", 999) < 40)
        check("后处理 < 10ms", perf.get("post_processing_ms", 999) < 10)

    # 总结
    print("\n" + "=" * 60)
    p = sum(1 for _, ok in results if ok)
    print(f"结果: {p}/{len(results)} 通过")
    print("✅ Agent 3 验收通过" if p == len(results) else "❌ Agent 3 验收未通过")
    print("=" * 60)
    return 0 if p == len(results) else 1

if __name__ == "__main__":
    sys.exit(main())
