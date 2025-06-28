# Pi Renderer 性能优化说明

## 已实施的优化

### 1. 移除调试输出 ⭐⭐⭐⭐⭐
- **影响**: 极大提升性能
- **修改**: 移除了所有 `std::cout` 调试输出，只保留每60帧的性能统计
- **原因**: 控制台输出在每帧都执行会严重拖慢渲染

### 2. Uniform位置缓存 ⭐⭐⭐⭐
- **影响**: 显著减少CPU开销
- **修改**: 在初始化时缓存uniform位置，避免每次渲染都查找
- **变量**: `loc_mvpMatrix`, `loc_modelMatrix`, `loc_color`, `loc_emissive`, `loc_lightDir`, `loc_radianceTex`

### 3. 可选渲染通道 ⭐⭐⭐⭐
- **影响**: 大幅提升帧率
- **修改**: 添加了性能开关控制全局光照和后处理
- **控制变量**:
  - `enableGI`: 控制全局光照计算
  - `enablePostProcessing`: 控制后处理
  - `frameSkip`: 控制GI计算频率（每N帧计算一次）

### 4. 移除帧率限制 ⭐⭐⭐
- **影响**: 允许渲染器达到最大性能
- **修改**: 注释掉 `SDL_Delay` 强制帧率限制

### 5. 平台特定优化 ⭐⭐⭐
- **影响**: 针对树莓派进行特殊优化
- **修改**: 在树莓派上默认使用较保守的设置

## 进一步优化建议

### 立即可实施的优化

1. **降低渲染分辨率** ⭐⭐⭐⭐⭐
   ```cpp
   // 将所有800x600改为更小的分辨率
   // 例如: 640x480 或 512x384
   int renderWidth = 640;
   int renderHeight = 480;
   ```

2. **简化着色器** ⭐⭐⭐⭐
   - 移除法线计算
   - 简化光照计算
   - 使用更低精度的纹理格式

3. **减少几何复杂度** ⭐⭐⭐
   - 使用更简单的网格
   - 减少顶点数量
   - 简化迷宫结构

### 高级优化

1. **实例化渲染** ⭐⭐⭐⭐⭐
   - 使用 `glDrawArraysInstanced` 批量渲染相同的墙块

2. **视锥体裁剪** ⭐⭐⭐⭐
   - 只渲染摄像机可见的物体

3. **LOD系统** ⭐⭐⭐
   - 根据距离使用不同细节级别的模型

## 性能开关使用方法

在 `main.cpp` 中可以调整这些变量：

```cpp
// 最高性能设置（关闭高级效果）
bool enableGI = false;
bool enablePostProcessing = false;
int frameSkip = 0;

// 平衡设置（推荐）
bool enableGI = true;
bool enablePostProcessing = true;
int frameSkip = 2;  // 每3帧计算一次GI

// 最高画质设置
bool enableGI = true;
bool enablePostProcessing = true;
int frameSkip = 0;  // 每帧都计算GI
```

## 预期性能提升

- **调试输出移除**: 2-5倍FPS提升
- **Uniform缓存**: 10-20%提升
- **可选渲染通道**: 2-4倍FPS提升（关闭GI时）
- **移除帧率限制**: 允许达到硬件极限

总体预期：**3-10倍性能提升**（取决于具体设置）
