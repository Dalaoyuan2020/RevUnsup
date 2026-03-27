// TODO: Agent 3 实现
// C# WinForms 测试界面
//
// 功能：
// 1. 选择模型目录
// 2. 选择测试图片
// 3. 调用 anomalyDet.dll 推理
// 4. 显示：原图 | 热力图 | 掩膜 | 异常分数
//
// P/Invoke 声明示例：
// [DllImport("anomalyDet.dll")]
// static extern int AIAD_Init(string modelDir, int gpuId);
//
// [DllImport("anomalyDet.dll")]
// static extern int AIAD_Infer(string imagePath, ref float score, ref int isDefect, float[] heatmap, byte[] mask);
//
// [DllImport("anomalyDet.dll")]
// static extern void AIAD_Release();
