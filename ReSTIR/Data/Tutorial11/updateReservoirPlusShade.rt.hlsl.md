
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
}
```
---
### Texture2D
```cpp
// Input and out textures that need to be set by the C++ code
Texture2D<float4>   gPos;           // G-buffer world-space position
Texture2D<float4>   gNorm;          // G-buffer world-space normal
Texture2D<float4>   gDiffuseMatl;   // G-buffer diffuse material (RGB) and opacity (A)

RWTexture2D<float4> gReservoirPrev;         // For ReSTIR - need to be read-write because it is also updated in the shader as well
Texture2D<float4>   gReservoirSpatial;

RWTexture2D<float4> gIndirectOutput; //For output from indirect illumination

RWTexture2D<float4> gOutput;        // Output to store shaded result
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
    float4 reservoir = gReservoirSpatial[launchIndex];
    gReservoirPrev[launchIndex] = reservoir; // Update reservoir value to be used for next pass
```
- 從空間儲存庫 (gReservoirSpatial) 中讀取當前像素位置 (launchIndex) 對應的儲存庫值 (reservoir)。
- 把當前的儲存庫值 (reservoir) 更新到先前儲存庫 (gReservoirPrev) 中，以便在下一次迭代中使用。
```cpp
    // Our camera sees the background if worldPos.w is 0, only do diffuse shading elsewhere
    if (worldPos.w != 0.0f)
    {
```
- 如果擊中了幾何體（即 worldPos.w != 0.0f），則進行以下操作：
```cpp
        int lightToSample;

        // We need to query our scene to find info about the current light
        float distToLight;      // How far away is it?
        float3 lightIntensity;  // What color is it?
        float3 toLight;         // What direction is it from our current pixel?
        float LdotN;            // Lambert term
        float shadowMult; // Visibility term

        lightToSample = reservoir.y;
        getLightData(lightToSample, worldPos.xyz, toLight, lightIntensity, distToLight);
        LdotN = saturate(dot(worldNorm.xyz, toLight));
        shadowMult = float(gLightsCount) * shadowRayVisibility(worldPos.xyz, toLight, gMinT, distToLight);
        shadeColor = shadowMult * reservoir.w * LdotN * lightIntensity * difMatlColor.rgb / M_PI;
```
- 獲取當前光源索引 (lightToSample)，可能是之前空間重用過程中選擇的光源。
- 獲取與光源相關的資訊，包括距離 (distToLight)、光強度 (lightIntensity)、光線方向 (toLight) 以及 Lambert term (LdotN)。
- **計算陰影多重性 (shadowMult)，這可能是根據場景中光源數目 (gLightsCount) 和陰影射線的可見性來計算的。**
- 計算最終的陰影顏色，考慮 Lambert term、光強度、漫反射材料顏色以及陰影多重性。
```cpp
    }

    // Save out our final shaded
    //gOutput[launchIndex] = float4(shadeColor, 1.f);
    gOutput[launchIndex] = float4(shadeColor, 1.f) + gIndirectOutput[launchIndex];
```
- 最終的陰影顏色與間接光輸出 (gIndirectOutput) 相加，並將結果寫入輸出緩衝 (gOutput)。
```cpp
}
```
- 總的來說，這段程式碼實現了基於 Lambertian 反射模型的陰影計算，同時使用了一些先前的空間重用信息。
