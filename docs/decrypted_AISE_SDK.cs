// 来源：PaddleSeg/deploy/cpp/build/AISE_SDK.cs（E-SafeNet 加密，由羊爸爸手动解密粘贴）
// 日期：2026-04-09

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace AISEService
{

    public class AISETestSDK
    {
        [DllImport(@"xyc_seg.dll", EntryPoint = "xyc_seg_init", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int xyc_seg_init(string modelpath);

        [DllImport(@"xyc_seg.dll", EntryPoint = "xyc_seg_run", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int xyc_seg_run(IntPtr testImage, IntPtr resultImage, float thresh);


        [DllImport(@"xyc_seg.dll", EntryPoint = "xyc_seg_uninit", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int xyc_seg_uninit();

    }

}
