"""
ROI 模板对齐与裁切模块

功能：
1. 选择模板图 (good sample)
2. 对目标图做模板匹配，估计偏移
3. 图像对齐
4. 应用 ROI mask
5. 输出 256x256 白底 core crop

对应旧版：train.exe 内部的数据预处理阶段
"""
import cv2
import numpy as np
from pathlib import Path
from typing import Tuple, Optional


def load_template(template_path: str) -> np.ndarray:
    """加载模板图像"""
    # TODO: Agent 1 实现
    raise NotImplementedError


def estimate_offset(
    template: np.ndarray,
    target: np.ndarray,
    method: int = cv2.TM_CCOEFF_NORMED
) -> Tuple[int, int]:
    """模板匹配估计偏移量"""
    # TODO: Agent 1 实现
    raise NotImplementedError


def align_image(
    target: np.ndarray,
    offset: Tuple[int, int],
    target_size: Tuple[int, int] = (256, 256)
) -> np.ndarray:
    """根据偏移量对齐图像"""
    # TODO: Agent 1 实现
    raise NotImplementedError


def apply_roi_mask(
    aligned: np.ndarray,
    mask: np.ndarray,
    background: int = 255
) -> np.ndarray:
    """应用 ROI 掩膜，背景填白"""
    # TODO: Agent 1 实现
    raise NotImplementedError


def process_single_image(
    image_path: str,
    template: np.ndarray,
    mask: np.ndarray,
    output_size: Tuple[int, int] = (256, 256)
) -> np.ndarray:
    """单张图像的完整 ROI 裁切流程"""
    # TODO: Agent 1 实现
    raise NotImplementedError
