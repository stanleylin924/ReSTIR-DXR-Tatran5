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
```cpp
// A constant buffer we'll populate from our C++ code
cbuffer RayGenCB
{
    float gMinT;        // Min distance to start a ray to avoid self-occlusion
    uint  gFrameCount;  // Frame counter, used to perturb random seed each frame
    bool  gInitLight;       // For ReSTIR - to choose an arbitrary light for this pixel after choosing 32 random light candidates
    bool  gTemporalReuse;

    //For GI
    bool  gDoIndirectGI;   // A boolean determining if we should shoot indirect GI rays
    bool  gCosSampling;    // Use cosine sampling (true) or uniform sampling (false)
    bool  gDirectShadow;   // Should we shoot shadow rays from our first hit point?

    matrix <float, 4, 4> gLastCameraMatrix;
}


// The payload used for our indirect global illumination rays
struct IndirectRayPayload
{
    float3 color;    // The (returned) color in the ray's direction
    uint   rndSeed;  // Our random seed, so we pick uncorrelated RNGs along our ray
};


// Input and out textures that need to be set by the C++ code
Texture2D<float4>   gPos;           // G-buffer world-space position
Texture2D<float4>   gNorm;          // G-buffer world-space normal
Texture2D<float4>   gDiffuseMatl;   // G-buffer diffuse material (RGB) and opacity (A)
RWTexture2D<float4> gReservoirPrev;     // For ReSTIR - need to be read-write because it is also updated in the shader as well
RWTexture2D<float4> gReservoirCurr;     // For ReSTIR - need to be read-write because it is also updated in the shader as wellRWTexture2D<float4> gOutput;        // Output to store shaded result
RWTexture2D<float4> gIndirectOutput; //For output from indirect illumination

// Our environment map, used for the miss shader for indirect rays
Texture2D<float4> gEnvMap;
```
---
```cpp
// What code is executed when our ray misses all geometry?
[shader("miss")]
void IndirectMiss(inout IndirectRayPayload rayData)
{
    // Load some information about our lightprobe texture
    float2 dims;
    gEnvMap.GetDimensions(dims.x, dims.y);

    // Convert our ray direction to a (u,v) coordinate
    float2 uv = wsVectorToLatLong(WorldRayDirection());

    // Load our background color, then store it into our ray payload
    rayData.color = gEnvMap[uint2(uv * dims)].rgb;
}
```
---
```cpp
// What code is executed when our ray hits a potentially transparent surface?
[shader("anyhit")]
void IndirectAnyHit(inout IndirectRayPayload rayData, BuiltInTriangleIntersectionAttributes attribs)
{
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (alphaTestFails(attribs))
        IgnoreHit();
}
```
---
```cpp
// What code is executed when we have a new closest hitpoint?   Well, pick a random light,
//    shoot a shadow ray to that light, and shade using diffuse shading.
[shader("closesthit")]
void IndirectClosestHit(inout IndirectRayPayload rayData, BuiltInTriangleIntersectionAttributes attribs)
{
    // Run a helper functions to extract Falcor scene data for shading
    ShadingData shadeData = getHitShadingData(attribs);

    // Pick a random light from our scene to shoot a shadow ray towards
    int lightToSample = min(int(nextRand(rayData.rndSeed) * gLightsCount), gLightsCount - 1);

    // Query the scene to find info about the randomly selected light
    float distToLight;
    float3 lightIntensity;
    float3 toLight;
    getLightData(lightToSample, shadeData.posW, toLight, lightIntensity, distToLight);

    // Compute our lambertion term (L dot N)
    float LdotN = saturate(dot(shadeData.N, toLight));

    // Shoot our shadow ray to our randomly selected light
    float shadowMult = float(gLightsCount) * shadowRayVisibility(shadeData.posW, toLight, RayTMin(), distToLight);

    // Return the Lambertian shading color using the physically based Lambertian term (albedo / pi)
    rayData.color = shadowMult * LdotN * lightIntensity * shadeData.diffuse / M_PI;
}
```
---
```cpp
// A utility function to trace an idirect ray and return the color it sees.
//    -> Note:  This assumes the indirect hit programs and miss programs are index 1!
float3 shootIndirectRay(float3 rayOrigin, float3 rayDir, float minT, uint seed)
{
    // Setup shadow ray
    RayDesc rayColor;
    rayColor.Origin = rayOrigin;  // Where does it start?
    rayColor.Direction = rayDir;  // What direction do we shoot it?
    rayColor.TMin = minT;         // The closest distance we'll count as a hit
    rayColor.TMax = 1.0e38f;      // The farthest distance we'll count as a hit

    // Initialize the ray's payload data with black return color and the current rng seed
    IndirectRayPayload payload;
    payload.color = float3(0, 0, 0);
    payload.rndSeed = seed;

    // Trace our ray to get a color in the indirect direction.  Use hit group #1 and miss shader #1
    TraceRay(gRtScene, 0, 0xFF, 1, hitProgramCount, 1, rayColor, payload);

    // Return the color we got from our ray
    return payload.color;
}
```
---
```cpp
// How do we shade our g-buffer and generate shadow rays?
[shader("raygeneration")]
void LambertShadowsRayGen()
{
```
- 這是一個 HLSL 光線追蹤的 Ray Generation Shader，用於對 G-buffer 進行著色並生成陰影光線。

```cpp
    // Get our pixel's position on the screen
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
```
- 獲取像素在屏幕上的位置以及發射光線的維度（螢幕的大小）。

```cpp
    // Load g-buffer data:  world-space position, normal, and diffuse color
    float4 worldPos = gPos[launchIndex];
    float4 worldNorm = gNorm[launchIndex];
    float4 difMatlColor = gDiffuseMatl[launchIndex];
```
- 從 G-buffer 中讀取（當前像素所對應擊中點的）世界空間位置、法線和漫反射材質顏色。

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
    // Our camera sees the background if worldPos.w is 0, only do diffuse shading elsewhere
    if (worldPos.w != 0.0f)
    {
```
- 如果擊中了幾何體（即 worldPos.w != 0.0f），執行以下步驟：

```cpp
        // Pick a random light from our scene to sample
        int lightToSample;

        // We need to query our scene to find info about the current light
        float distToLight;      // How far away is it?
        float3 lightIntensity;  // What color is it?
        float3 toLight;         // What direction is it from our current pixel?
        float LdotN;            // Lambert term

        float4 prev_reservoir = float4(0.f); // initialize previous reservoir

        // if not first time fill with previous frame reservoir
        if (!gInitLight) {
            float4 screen_space = mul(worldPos, gLastCameraMatrix);
            screen_space /= screen_space.w;
            uint2 prevIndex = launchIndex;
            prevIndex.x = ((screen_space.x + 1.f) / 2.f) * (float)launchDim.x;
            prevIndex.y = ((1.f - screen_space.y) / 2.f) * (float)launchDim.y;

            if (prevIndex.x >= 0 && prevIndex.x < launchDim.x && prevIndex.y >= 0 && prevIndex.y < launchDim.y) {
                prev_reservoir = gReservoirPrev[prevIndex];
            }
        }

        float4 reservoir = float4(0.f);
        float p_hat;
```
- 創建了一個名為 reservoir 的四維浮點數向量，並將其所有元素初始化為 0.0。reservoir 被用來儲存光源的相關信息，其四個欄位分別表示：
  - x：累計權重和，即 $r.w_{sum}$。
  - y：當前 reservoir 中的樣本，即當前選中的光源。
  - z：累計樣本數或光源數量，即 $r.M$。
  - w：表示 visibility，即選中的光源對於擊中點是否可見。1 表示可見，0 表示不可見。
```cpp
        // initialize previous reservoir if this is the first iteraation
        if (gInitLight) { prev_reservoir = float4(0.f); }
```
```cpp
        // ----------------------------------------------------------------------------------------------
        // -----------------------------Initial candidates generation BEGIN -----------------------------
        // ----------------------------------------------------------------------------------------------
```
- 生成初始候選光源，遵循 ReSTIR 論文中的 Algorithm 3。這涉及對場景中的隨機光源進行抽樣，計算 Lambertian term 和 p_hat 值，並更新候選光源的儲備。
```cpp
        // Generate Initial Candidates - Algorithm 3 of ReSTIR paper
        for (int i = 0; i < min(gLightsCount, 32); i++) {
```
- 限制迭代的次數，確保不會超過光源的總數（若光源數量少於 32）或是 32（若光源數量超過 32）。
- 從最多 32 個初始候選光源中，最終挑出一個選中的光源儲存到 reservoir 中。
```cpp
            lightToSample = min(int(nextRand(randSeed) * gLightsCount), gLightsCount - 1);
```
- 使用 nextRand(randSeed) 函數生成一個在 [0, 1) 之間的隨機數，並將其乘以光源數量 gLightsCount。將其擴展為在 [0, gLightsCount) 之間的隨機數，gLightsCount 為場景中的光源數量。
- 通過 int 函數將所得的浮點數轉換為整數，min 函數確保其不超過光源數量的上限。
- 最後得到一個隨機選中（均勻取樣）的光源索引。
- 可以看到 32 個初始候選光源是透過均勻取樣選出的，每個光源被選中的機率均等。
```cpp
            getLightData(lightToSample, worldPos.xyz, toLight, lightIntensity, distToLight);
```
- 使用 getLightData 函數獲取所選光源的信息，包括光源方向 toLight、光源強度 lightIntensity 以及擊中點和光源之間的距離 distToLight。
```cpp
            LdotN = saturate(dot(worldNorm.xyz, toLight)); // lambertian term
```
- 計算 Lambertian 反射的項目 LdotN，即入射光線和法線的點積，並進行飽和處理。
- 這個公式計算了光線的入射角的餘弦值（dot product）並通過 saturate 函數將結果夾緊在 [0, 1] 的範圍內。
  - worldNorm.xyz 是表面法線的向量。
  - toLight 是光線的方向向量。
  - dot(worldNorm.xyz, toLight) 計算了法線向量和光線方向向量的內積，結果是兩個向量之間夾角的餘弦值。在這個上下文中，這個值通常被稱為 Lambertian 反射的 Lambert term。
  - 然而，內積的結果範圍是負無窮到正無窮，而 Lambert term 的合法範圍是 [0, 1]。為了確保值在這個合法範圍內，使用 saturate 函數。它將所有小於 0 的值置為 0，所有大於 1 的值置為 1，因此確保 Lambert term 被夾在合理的範圍內。
- 總的來說，LdotN 用來衡量光線的入射角度對漫反射光線的影響，這是 Lambertian 反射模型的一個基本元素。
```cpp
            // p_hat of the light is f * Le * G / pdf
            p_hat = length(difMatlColor.xyz / M_PI * lightIntensity * LdotN / (distToLight * distToLight)); // technically p_hat is divided by pdf, but point light pdf is 1
```
- 分配每個候選光源的權重 $w(x) = \hat{p}(x)/p(x)$，由於每個光源被選中的機率均等，因此 $p(x)$ 可以消掉，得到 $w(x) = \hat{p}(x)$，即 p_hat。
- $\hat{p}(x)$ 可以視為光源在 Rendering Equation 中的貢獻，$\hat{p}(x)$ = 材質的漫反射顏色（物體顏色） / $\pi$ * 光源的強度（顏色）* 光線方向和表面法線的夾角的 cos 值 / 光源到擊中點的距離平方。
  - difMatlColor.xyz / M_PI：這部分是表面漫反射材質的顏色，並將其除以 π。這裡的 π 是用來簡化計算，以確保漫反射的能量在整個半球上是均勻分布的。這是 Lambertian 反射的一個重要特性。
  - lightIntensity：這是光源的強度，表示光的顏色和亮度。
  - LdotN：這是入射光線與表面法線的點積，即光線方向和表面法線的夾角的餘弦值。這表示光線的入射角度對於表面的影響，Lambertian 反射假設光在所有方向上均勻分散，所以這是漫反射模型中的一部分。
  - distToLight * distToLight：這是入射光線的距離的平方。在 Lambertian 反射中，通常會將光線的強度除以距離的平方，以模擬光在空間中的衰減。這是一種物理現象，即光通量隨著距離的增加而減少。
  - length()：這是為了計算上述所有項目的總體強度。length() 函數計算一個向量的歐幾里德範數（或稱為長度），在這裡用於計算漫反射項的強度。
- 總體來說，這個公式是為了計算 Lambertian 反射模型下，從光源到表面某點的漫反射光線的強度。這考慮了材質的顏色，光源的強度，光線的入射角，以及光線的衰減效應。
```cpp
            reservoir = updateReservoir(reservoir, lightToSample, p_hat, randSeed);
```
- 將計算得到的 p_hat（作為光源的權重）和相應的光源索引等信息傳遞給 updateReservoir 函數，以更新 reservoir 的值。
```cpp
        }
```
- 總體來說，這個過程是在所有光源中隨機選擇一個光源，獲取其相應的信息，計算對擊中點的影響，然後更新儲存器，以用於後續的全局光照計算。
```cpp
        // ----------------------------------------------------------------------------------------------
        // -----------------------------Initial candidates generation END -------------------------------
        // ----------------------------------------------------------------------------------------------
```
- 下面這一段代碼是用來評估初始化候選光源的可見性，並設置 reservoir 向量的第四個元素 w 的值。
```cpp
        // Evaluate visibility for initial candidate and set r.W value
        lightToSample = reservoir.y;
        getLightData(lightToSample, worldPos.xyz, toLight, lightIntensity, distToLight);
```
- 使用當前選中的光源，獲取該光源在當前擊中點的信息，包括光線方向 toLight、光線強度 lightIntensity、擊中點到光源的距離 distToLight 等。
```cpp
        LdotN = saturate(dot(worldNorm.xyz, toLight));
```
- 計算 Lambertian 反射項 LdotN，這是擊中點法向量和光線方向的內積，取值範圍在 0 到 1 之間。（參考前面說明）
```cpp
        p_hat = length(difMatlColor.xyz / M_PI * lightIntensity * LdotN / (distToLight * distToLight));
```
- 計算光源的 p_hat 值（作為該光源的權重），這是一個確定性概率，表示在擊中點上選中該光源的概率。計算公式中考慮了材質的顏色，光源的強度，光線的入射角，以及光線的衰減效應。（參考前面說明）
```cpp
        reservoir.w = (1.f / max(p_hat, 0.0001f)) * (reservoir.x / max(reservoir.z, 0.0001f));
```
- 計算 reservoir.w，即 r.W，這是 ReSTIR 演算法中用來採樣的權重。它的計算包括了 p_hat、reservoir.x（之前累積的所有 p_hat 值的和）和 reservoir.z（之前累積的光源數量）等參數。
```cpp
        if (shadowRayVisibility(worldPos.xyz, toLight, gMinT, distToLight) < 0.001f) {
            reservoir.w = 0.f;
        }
```
- 如果通過 shadowRayVisibility 函數判斷光源和撞擊點之間的陰影可見性低於一個閾值，則將 reservoir.w 設置為0，表示該光源將被視為不可見。
```cpp
        // ----------------------------------------------------------------------------------------------
        // ----------------------------------- Temporal reuse BEGIN -------------------------------------
        // ----------------------------------------------------------------------------------------------
        if (gTemporalReuse) {
            float4 temporal_reservoir = float4(0.f);

            // combine current reservoir
            temporal_reservoir = updateReservoir(temporal_reservoir, reservoir.y, p_hat * reservoir.w * reservoir.z, randSeed);

            // combine previous reservoir
            getLightData(prev_reservoir.y, worldPos.xyz, toLight, lightIntensity, distToLight);
            LdotN = saturate(dot(worldNorm.xyz, toLight));
            p_hat = length(difMatlColor.xyz / M_PI * lightIntensity * LdotN / (distToLight * distToLight));
            prev_reservoir.z = min(20.f * reservoir.z, prev_reservoir.z);
            temporal_reservoir = updateReservoir(temporal_reservoir, prev_reservoir.y, p_hat * prev_reservoir.w * prev_reservoir.z, randSeed);

            // set M value
            temporal_reservoir.z = reservoir.z + prev_reservoir.z;

            // set W value
            getLightData(temporal_reservoir.y, worldPos.xyz, toLight, lightIntensity, distToLight);
            LdotN = saturate(dot(worldNorm.xyz, toLight));
            p_hat = length(difMatlColor.xyz / M_PI * lightIntensity * LdotN / (distToLight * distToLight));
            temporal_reservoir.w = (1.f / max(p_hat, 0.0001f)) * (temporal_reservoir.x / max(temporal_reservoir.z, 0.0001f));

            // set current reservoir to the combined temporal reservoir
            reservoir = temporal_reservoir;
        }
```
- 如果啟用了 gTemporalReuse，則執行時間重用的相關計算。這包括結合當前儲備和前一幀的儲備，以及更新 M 和 W 的值。

```cpp

        // ----------------------------------------------------------------------------------------------
        // ----------------------------------- Temporal reuse END ---------------------------------------
        // ----------------------------------------------------------------------------------------------

        // ----------------------------------------------------------------------------------------------
        //----------------------------------- Global Illumination BEGIN----------------------------------
        // ----------------------------------------------------------------------------------------------

        //For Indirect Illumination
        float3 bounceColor;
        float ID_NdotL;
        float sampleProb;

        // Indirect illumination
        if (gDoIndirectGI)
        {
            // Select a random direction for our diffuse interreflection ray.
            float3 bounceDir;
            if (gCosSampling)
                bounceDir = getCosHemisphereSample(randSeed, worldNorm.xyz);      // Use cosine sampling
            else
                bounceDir = getUniformHemisphereSample(randSeed, worldNorm.xyz);  // Use uniform random samples

            // Get NdotL for our selected ray direction
            ID_NdotL = saturate(dot(worldNorm.xyz, bounceDir));

            // Shoot our indirect global illumination ray
            bounceColor = shootIndirectRay(worldPos.xyz, bounceDir, gMinT, randSeed);

            //bounceColor = (ID_NdotL > 0.50f) ? float3(0, 0, 0) : bounceColor;

            // Probability of selecting this ray ( cos/pi for cosine sampling, 1/2pi for uniform sampling )
            sampleProb = gCosSampling ? (ID_NdotL / M_PI) : (1.0f / (2.0f * M_PI));
        }
```
- 如果啟用了全局光照 gDoIndirectGI，則選擇一個隨機方向生成漫反射射線，計算 NdotL 和發射間接全局光線，並計算選擇該射線的概率。

```cpp

        // ----------------------------------------------------------------------------------------------
        // ---------------------------------- Global Illumination END------------------------------------
        // ----------------------------------------------------------------------------------------------

        // Save the computed reserrvoir back into the buffer
        gReservoirCurr[launchIndex] = reservoir;
        gIndirectOutput[launchIndex] = float4(0.f); //Intialize to 0
        if (gDoIndirectGI)
        {
            gIndirectOutput[launchIndex] = float4((ID_NdotL * bounceColor* difMatlColor.rgb / M_PI / sampleProb), 1.0);
        }
```
- 將計算的儲備值保存回緩衝 gReservoirCurr 和計算的間接光輸出保存回緩衝 gIndirectOutput。

```cpp
    }

    // Save out our final shaded
    //gOutput[launchIndex] = float4(shadeColor, 1.0f);

}
```
- 總的來說，這個 Ray Generation Shader 主要負責計算場景中光源的影響，包括漫反射陰影和全局間接光照。 ReSTIR 算法的一些步驟被用來生成和更新候選光源的 Reservoir，以實現光線追蹤效果。
