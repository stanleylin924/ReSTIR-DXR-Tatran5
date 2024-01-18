
### Contents
[TOC]

---
```cpp
// Some shared Falcor stuff for talking between CPU and GPU code
#include "HostDeviceSharedMacros.h"
#include "HostDeviceData.h"

// Include and import common Falcor utilities and data structures
import Raytracing;                   // Shared ray tracing specific functions & data
import ShaderCommon;                 // Shared shading data structures
import Shading;                      // Shading functions, etc
import Lights;                       // Light structures for our current scene

// A separate file with some simple utility functions: getPerpendicularVector(), initRand(), nextRand()
#include "restirUtils.hlsli"

// Include shader entries, data structures, and utility function to spawn shadow rays
#include "standardShadowRay.hlsli"
```
---
### RayGenCB
```cpp
// A constant buffer we'll populate from our C++ code
cbuffer RayGenCB
{
    float gMinT;        // Min distance to start a ray to avoid self-occlusion
    uint  gFrameCount;  // Frame counter, used to perturb random seed each frame
    bool  gSpatialReuse;
}
```
---
### Texture2D
```cpp
// Input and out textures that need to be set by the C++ code
Texture2D<float4>   gPos;           // G-buffer world-space position
Texture2D<float4>   gNorm;          // G-buffer world-space normal
Texture2D<float4>   gDiffuseMatl;   // G-buffer diffuse material (RGB) and opacity (A)

RWTexture2D<float4> gReservoirCurr;         // For ReSTIR - need to be read-write because it is also updated in the shader as well
RWTexture2D<float4> gReservoirSpatial;      // For ReSTIR - need to be read-write because it is also updated in the shader as well
```
---
### LambertShadowsRayGen()
```cpp
// How do we shade our g-buffer and generate shadow rays?
[shader("raygeneration")]
void LambertShadowsRayGen()
{
    // Get our pixel's position on the screen
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
```
- 獲取當前像素在屏幕上的位置 (launchIndex) 以及螢幕的大小 (launchDim)。
```cpp
    // Load g-buffer data:  world-space position, normal, and diffuse color
    float4 worldPos = gPos[launchIndex];
    float4 worldNorm = gNorm[launchIndex];
    float4 difMatlColor = gDiffuseMatl[launchIndex];
```
- 載入世界空間中的位置 (worldPos)、法線 (worldNorm) 以及漫反射材料顏色 (difMatlColor)。
```cpp
    // If we don't hit any geometry, our difuse material contains our background color.
    float3 shadeColor = difMatlColor.rgb;
```
- 如果 worldPos.w = 0.0f，即未擊中任何幾何體，將 shadeColor 設置為背景色（已儲存在漫反射材質中）。在此先將 shadeColor 初始化為漫反射材質顏色。
```cpp
    // Initialize our random number generator
    uint randSeed = initRand(launchIndex.x + launchIndex.y * launchDim.x, gFrameCount, 16);
```
- 初始化隨機數生成器，取得隨機數種子。
```cpp
    float4 reservoirNew = float4(0.f);
```
- 創建了一個名為 reservoirNew 的四維浮點數向量，並將其所有元素初始化為 0.0。reservoirNew 被用來儲存光源的相關信息，其四個欄位分別表示：
  - x：累計權重和，即 $r.w_{sum}$。
  - y：當前 reservoir 中的樣本，即當前選中的光源。
  - z：累計樣本數或光源數量，即 $r.M$。
  - w：表示 visibility，即選中的光源對於擊中點是否可見。1 表示可見，0 表示不可見。
```cpp
    // Our camera sees the background if worldPos.w is 0, only do diffuse shading elsewhere
    if (worldPos.w != 0.0f && gSpatialReuse)
    {
```
- 如果擊中了幾何體（即 worldPos.w != 0.0f），並且啟用了空間重用（gSpatialReuse 為 true），則進行以下操作：
```cpp
        // We need to query our scene to find info about the current light
        float distToLight;      // How far away is it?
        float3 lightIntensity;  // What color is it?
        float3 toLight;         // What direction is it from our current pixel?
        float LdotN;                        // Lambert term

        // Additional variables for ReSTIR
        float p_hat;

        // ----------------------------------------------------------------------------------------------
        // ----------------------------------- Algorithm 5 - Spatial reuse BEGIN ------------------------
        // ----------------------------------------------------------------------------------------------
        uint2 neighborOffset;
        uint2   neighborIndex;
        float4 neighborReservoir;

        int neighborsCount = 15;
        int neighborsRange = 5; // Want to sample neighbors within [-neighborsRange, neighborsRange] offset
```
```cpp
        // Combine with reservoir at current pixel -------------------------------------------------------
        float4 reservoir = gReservoirCurr[launchIndex];
        getLightData(reservoir.y, worldPos.xyz, toLight, lightIntensity, distToLight);
        LdotN = saturate(dot(worldNorm.xyz, toLight)); // lambertian term
        p_hat = length(difMatlColor.xyz / M_PI * lightIntensity * LdotN / (distToLight * distToLight));
```
- 獲取當前像素的光線相關數據，包括距離、光強度、光線方向以及 Lambert term (LdotN)。
- 根據 Lambert term 計算重要性 (p_hat)。
```cpp
        reservoirNew = updateReservoir(reservoirNew, reservoir.y, p_hat * reservoir.w * reservoir.z, randSeed);
```
- 使用算法 3 中的公式更新新的儲存庫（reservoirNew）。
```cpp
        float lightSamplesCount = reservoir.z;
        // Combined logic of picking random neighbor and combine reservoirs
        for (int i = 0; i < neighborsCount; i++) {
            // Reservoir reminder:
            // .x: weight sum
            // .y: chosen light for the pixel
            // .z: the number of samples seen for this current light
            // .w: the final adjusted weight for the current pixel following the formula in algorithm 3 (r.W)

            // Generate a random number from range [0, 2 * neighborsRange] then offset in negative direction
            // by spatialNeighborCount to get range [-neighborsRange, neighborsRange].
            // Need to take care of out of bound case hence the max and min
            neighborOffset.x = int(nextRand(randSeed) * neighborsRange * 2.f) - neighborsRange;
            neighborOffset.y = int(nextRand(randSeed) * neighborsRange * 2.f) - neighborsRange;

            neighborIndex.x = max(0, min(launchDim.x - 1, launchIndex.x + neighborOffset.x));
            neighborIndex.y = max(0, min(launchDim.y - 1, launchIndex.y + neighborOffset.y));

            neighborReservoir = gReservoirCurr[neighborIndex];
```
- 進行鄰居的迭代選擇和更新。鄰居的選擇是通過在 [-neighborsRange, neighborsRange] 範圍內生成隨機偏移量，然後應用到當前像素索引上來實現的。
```cpp
            getLightData(neighborReservoir.y, worldPos.xyz, toLight, lightIntensity, distToLight);
            LdotN = saturate(dot(worldNorm.xyz, toLight)); // lambertian term
            p_hat = length(difMatlColor.xyz / M_PI * lightIntensity * LdotN / (distToLight * distToLight));

            reservoirNew = updateReservoir(reservoirNew, neighborReservoir.y, p_hat * neighborReservoir.w * neighborReservoir.z, randSeed);

            lightSamplesCount += neighborReservoir.z;
```
- 累計鄰居的光線數量，以及使用算法 3 中的公式更新新的儲存庫。
```cpp
        }

        // Update the correct number of candidates considered for this pixel
        reservoirNew.z = lightSamplesCount;

        // Update the adjusted final weight of the current reservoir ------------------------------------
        getLightData(reservoirNew.y, worldPos.xyz, toLight, lightIntensity, distToLight);
        LdotN = saturate(dot(worldNorm.xyz, toLight)); // lambertian term
        p_hat = length(difMatlColor.xyz / M_PI * lightIntensity * LdotN / (distToLight * distToLight));

        reservoirNew.w = (1.f / max(p_hat, 0.0001f)) * (reservoirNew.x / max(reservoirNew.z, 0.0001f));
```
- 更新當前像素的儲存庫數量（reservoirNew.z）以及相應的最終調整權重（reservoirNew.w）。
```cpp
    }

    gReservoirSpatial[launchIndex] = reservoirNew;
```
- 更新當前像素的 Spatial Reservoir（gReservoirSpatial）。
```cpp
}
```
- 這段程式碼的主要目的是在光線追踪中實現一種空間重用（Spatial Reuse）技術，以提高效率並獲得更精確的結果。
