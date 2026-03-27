"""通用工具函数"""
import yaml
import json
from pathlib import Path
from typing import Dict, Any


def load_config(config_path: str) -> Dict[str, Any]:
    """加载 YAML 配置"""
    with open(config_path, "r") as f:
        return yaml.safe_load(f)


def save_json(data: Dict, output_path: str):
    """保存 JSON"""
    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)


def load_json(path: str) -> Dict:
    """加载 JSON"""
    with open(path, "r") as f:
        return json.load(f)
