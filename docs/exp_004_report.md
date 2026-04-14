# RU-007 Phase 3A: Zero-cost Forward Acceleration

> Date: 2026-04-14 | GPU: RTX 5060 Ti | PyTorch 2.11.0+cu128 | 6 images x 5 runs

## Config
Base: RU-006 max_pool_4x (FP16 chunked matmul + max_pool kernel=4)
Added flags (3 lines):
1. `torch.backends.cudnn.benchmark = True`
2. `model = model.to(memory_format=torch.channels_last)` + `patch.to(memory_format=torch.channels_last)`
3. `model = torch.compile(model, mode='reduce-overhead')`

## Results

| Segment | RU-006 (est.) | RU-007 (measured) | Change |
|---------|:---:|:---:|:---:|
| **Total** | 71.3ms | **65.5ms** | 1.09x |
| Forward (backbone+pool) | ~41ms* | 51.7ms | see note |
| KNN | 8.85ms | 4.9ms | 1.80x faster |
| Preprocess | ~20ms* | 8.1ms | see note |
| Postprocess | ~1ms | 0.8ms | ~same |

*RU-006 did not have segment timing. The 41ms/20ms split was estimated from total-knn.

## Key Findings

1. **Total speedup: 1.09x (71.3 -> 65.5ms)** — below 10% threshold (Plan E)
2. **torch.compile had zero measurable effect** — same results with/without (tested both)
3. **cudnn.benchmark + channels_last** improved KNN (8.85 -> 4.9ms) and preprocess (est. 20 -> 8.1ms)
4. **Forward was always ~52ms**, not 41ms — the RU-006 estimate was off because preprocess was overestimated
5. **Corrected breakdown**: Forward 52ms (79%) + Preprocess 8ms (12%) + KNN 5ms (8%) + Post 1ms
6. **Target 40ms: NO** (gap: 25.5ms)

## Flag Status
- cudnn.benchmark: ON (small gains in KNN/preprocess)
- channels_last: ON (no clear forward improvement on TorchScript ResNet18)
- torch.compile: ON (no measurable effect — TorchScript + Blackwell may limit gains)

## Implication
Zero-cost flags ceiling is ~65ms. Forward at 52ms is the hard bottleneck (79% of total).
Next lever: Phase 3B (GPU preprocess) or Phase 3C (TensorRT FP16 for backbone).
