```cpp
// Define pi
#define M_1_PI  0.318309886183790671538
```
- 定義常數 M_1_PI，其值為 pi 的倒數。
---
### updateReservoir()
```cpp
float4 updateReservoir(float4 reservoir, int lightToSample, float weight, uint randSeed) {
```
- 這段程式碼實現的是 ReSTIR 算法中的 reservoir 更新的過程，具體來說，這是 ReSTIR 論文中的 Algorithm 2。
```cpp
    // Algorithm 2 of ReSTIR paper
    reservoir.x = reservoir.x + weight; // r.w_sum
```
- 將儲備中的 r.w_sum（權重和）增加 weight，這是儲備中用於權重和的項。
```cpp
    reservoir.z = reservoir.z + 1.0f; // r.M
```
- 將儲備中的 r.M（候選數目）增加 1，這是儲備中用於跟踪候選數目的項。
```cpp
    if (nextRand(randSeed) < weight / reservoir.x) {
```
- 使用隨機數生成器 nextRand 生成一個在 [0, 1) 之間的隨機數，如果這個隨機數小於 weight / reservoir.x，則執行下一步；否則，跳過下一步。
```cpp
        reservoir.y = lightToSample; // r.y
```
- 如果滿足上述條件，將儲備中的 r.y（選擇的光源索引）設置為 lightToSample，即當前選擇的光源索引。
```cpp
    }

    return reservoir;
```
- 返回更新後的儲備。
```cpp
}
```
- 總的來說，這個函數的目的是根據新計算得到的概率密度函數比例（weight / reservoir.x），更新儲備的權重和候選數目，同時根據一定的概率選擇當前的光源索引。這個更新的過程是 ReSTIR 算法中重要的一部分，用於有效地選擇和追蹤光源。
---
### getLightData()
```cpp
// A helper to extract important light data from internal Falcor data structures.  What's going on isn't particularly
//     important -- any framework you use will expose internal scene data in some way.  Use your framework's utilities.
void getLightData(in int index, in float3 hitPos, out float3 toLight, out float3 lightIntensity, out float distToLight)
{
```
- 這段程式碼是一個用來從內部 Falcor 資料結構中提取重要光源數據的輔助函數。具體來說，它通過使用內建的 Falcor 函數和資料結構來填充一個 LightSample 數據結構。這個函數接受光源的索引（index）和擊中點的位置（hitPos），然後返回一組簡化的光源數據，包括光源方向（toLight）、光源強度（lightIntensity）和擊中點到光源的距離（distToLight）。
```cpp
    // Use built-in Falcor functions and data structures to fill in a LightSample data structure
    //   -> See "Lights.slang" for it's definition
    LightSample ls;
```
- 創建一個名為ls的LightSample結構的實例，這個結構包含了有關光源的信息。
```cpp
    // Is it a directional light?
    if (gLights[index].type == LightDirectional)
        ls = evalDirectionalLight(gLights[index], hitPos);
```
- 檢查光源的類型是否是方向光。
- 如果是方向光，使用 evalDirectionalLight 函數計算光線樣本。
```cpp
    // No?  Must be a point light.
    else
        ls = evalPointLight(gLights[index], hitPos);
```
- 如果不是方向光，則假定是點光源，並使用 evalPointLight 函數計算光線樣本。
```cpp
    // Convert the LightSample structure into simpler data
    toLight = normalize(ls.L);
    lightIntensity = ls.diffuse;
    distToLight = length(ls.posW - hitPos);
```
- 計算光源的標準化（normalized）方向。
- 將光源的漫射強度設定給 lightIntensity。
- 計算擊中點到光源的距離。
```cpp
}
```
- 函數的主要邏輯是檢查指定索引的光源類型，如果是方向光（LightDirectional），則使用 evalDirectionalLight 函數評估該方向光在擊中點的效果；否則，假定為點光源（evalPointLight 函數）。最後，從 LightSample 結構中提取的數據被轉換成更簡單的形式，以供後續使用。
---
### getPerpendicularVector()
```cpp
// Utility function to get a vector perpendicular to an input vector
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 getPerpendicularVector(float3 u)
{
```
- 這是一個函數，用於計算與給定向量 u 垂直的向量。
- 接收一個三維向量 u 並返回一個垂直的三維向量。
```cpp
    float3 a = abs(u);
```
- 創建一個新的向量 a，其元素是 u 中對應元素的絕對值。
```cpp
    uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
```
- 根據 a 的元素值，計算 xm，它是一個條件表達式，如果 (a.x - a.y)<0 和 (a.x - a.z)<0 均為真，則 xm 被設置為 1，否則為 0。
```cpp
    uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
```
- 根據 a 的元素值，計算 ym，如果 (a.y - a.z)<0 為真，則 ym 被設置為 1 ^ xm，否則為 0。這裡的 ^ 是按位 XOR 運算符。
```cpp
    uint zm = 1 ^ (xm | ym);
```
- 計算 zm，它是 1 ^ (xm | ym) 的結果，其中 | 是按位 OR 運算符。
```cpp
    return cross(u, float3(xm, ym, zm));
```
- 使用 cross 函數計算原始向量 u 和新創建的向量 float3(xm, ym, zm) 的叉積，這樣就得到了一個與原始向量垂直的向量。返回這個新的垂直向量。
```cpp
}
```
---
### initRand()
```cpp
// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff = 16)
{
```
- 這是一個用於初始化隨機數生成器的函數，基於簡單的偽隨機數生成算法。
- 接收兩個 32 位整數 val0 和 val1，以及一個可選的參數 backoff（默認為16）。該函數返回一個 32 位無號整數。
- backoff 是一個可選的參數，它表示初始化隨機數生成器時從初始狀態開始進行的迭代次數。具體來說，它在一個迴圈中使用 [unroll] 來增加隨機性，使得生成的初始數據更加混亂和不可預測。通常，增加 backoff 的值可以改善伪隨機數生成器的質量，使其更難預測。但同時，它也會增加初始化所需的計算時間。
```cpp
    uint v0 = val0, v1 = val1, s0 = 0;
```
- 初始化三個 32 位無號整數變數 v0、v1 和 s0。
```cpp
    [unroll]
```
- [unroll] 是 HLSL（High-Level Shading Language）中的一個指示符，用於提示編譯器展開指定的循環。在上下文中，它通常與 for 循環一起使用。
- 當編譯器遇到 [unroll] 標記時，它可能會嘗試展開循環的迭代，這意味著將循環內的代碼複製多次，以減少循環的開銷。這對於某些循環，特別是在迭代次數相對較小且已知的情況下，可能有助於提高程序的性能。
- 在給定的代碼片段中，[unroll] 被用於一個 for 循環，這可能會提示編譯器嘗試展開這個循環，以加速初始化隨機數生成器的操作。展開循環的效果是在編譯時生成多個循環迭代的副本，這樣可能會提高程序的運行速度，但同時也可能增加生成的代碼的大小。
```cpp
    for (uint n = 0; n < backoff; n++)
    {
```
- 使用 HLSL 的 [unroll] 關鍵字，展開一個迴圈，迴圈的次數為 backoff。
```cpp
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
```
- 在每次循環中，增加 s0 的值。這是一個常量，用於偽隨機性。
- 更新 v0 的值，根據一系列的位元運算和 XOR 運算。
- 更新 v1 的值，同樣根據一系列的位元運算和 XOR 運算。
```cpp
    }
    return v0;
```
- 返回計算結果中的 v0。這個值可以作為初始化後的隨機數生成器的初始值。
```cpp
}
```
---
### nextRand()
```cpp
// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
```
- 這段代碼是一個簡單的線性同餘隨機數生成器（Linear Congruential Generator，LCG）。這種生成器通常用來產生偽隨機數序列。
- s 是一個 uint 類型的變數，用於保存生成器的內部狀態。在每次調用函數時，該狀態會更新。
```cpp
    s = (1664525u * s + 1013904223u);
```
- 這是線性同餘公式，用於計算下一個隨機數。1664525 和 1013904223 是經過選擇的常數，它們的選擇影響著生成器的性能和統計特性。
```cpp
    return float(s & 0x00FFFFFF) / float(0x01000000);
```
- 用於將 32 位無號整數轉換為浮點數。它通過將 s 與 0x00FFFFFF 進行按位 AND 運算，截斷高 8 位，然後將結果除以 0x01000000（2^24），得到一個範圍在 [0, 1) 之間的浮點數。
```cpp
}
```
- 總的來說，nextRand 函數返回一個在區間 [0, 1) 內的偽隨機浮點數，同時更新內部狀態 s 以供下一次調用使用。請注意，這是一個簡單的偽隨機數生成器，不適用於加密或需要高質量隨機數的應用。
---
### getCosHemisphereSample()
```cpp
// Get a cosine-weighted random vector centered around a specified normal direction.
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
```
- 這段代碼實現了在半球上採樣的功能，並使用了餘弦加權（Cosine Weighted）的方法，確保更多的樣本集中在表面法線方向。
- 返回一個 float3 類型的向量，表示在半球上根據餘弦加權進行採樣的結果。這個向量包含三個分量，分別表示 x、y、z 軸方向的分量。
```cpp
    // Get 2 random numbers to select our sample with
    float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));
```
- 生成兩個隨機數，用於選擇半球上的樣本。
```cpp
    // Cosine weighted hemisphere sample from RNG
    float3 bitangent = getPerpendicularVector(hitNorm);
```
- 使用 getPerpendicularVector 函數計算表面法線的垂直向量，作為半球上的一個樣本點。
```cpp
    float3 tangent = cross(bitangent, hitNorm);
```
- 使用表面法線和垂直向量的外積計算第二個垂直向量。
```cpp
    float r = sqrt(randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;
```
- 將第一個隨機數開平方根，用於在半球上選擇半徑。
- 將第二個隨機數轉換為角度 phi。
```cpp
    // Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
```
- 根據半球座標系統中的坐標，計算半球上的樣本點。使用三個向量（tangent、bitangent 和 hitNorm）的線性組合。這確保樣本點集中在表面法線的方向，由於餘弦加權，樣本在表面法線方向上的分佈更多。
- tangent * (r * cos(phi).x): 這部分計算在局部坐標系統中，相對於表面法線的切線方向的分量。tangent 是表面法線的切線向量，而 (r * cos(phi).x) 部分是對應於切線向量的長度及方向。
- bitangent * (r * sin(phi)): 這部分計算在局部坐標系統中，相對於表面法線的副切線方向的分量。bitangent 是表面法線的副切線向量，而 (r * sin(phi)) 部分是對應於副切線向量的長度及方向。
- hitNorm.xyz * sqrt(1 - randVal.x): 這部分計算垂直於表面法線的分量，以確保向量的長度不會超過 1。hitNorm.xyz 是表面法線，而 sqrt(1 - randVal.x) 部分是確保該分量的長度不會超過 1。
```cpp
}
```
- 總的來說，這個函數用於生成在半球上進行餘弦加權（Cosine Weighted）的樣本點，通常用於計算漫反射光線的方向。
---
### alphaTestFails()
```cpp
// This function tests if the alpha test fails, given the attributes of the current hit.
//   -> Can legally be called in a DXR any-hit shader or a DXR closest-hit shader, and
//      accesses Falcor helpers and data structures to extract and perform the alpha test.
bool alphaTestFails(BuiltInTriangleIntersectionAttributes attribs)
{
```
- 這段代碼是用於執行 alpha 測試（alpha test）的函式。
- 返回值是一個布林值，表示是否通過 alpha 測試。如果 alpha 測試失敗（即基礎顏色的 alpha 分量小於材質的 alpha 閾值），則返回 true，否則返回 false。所以，如果返回 true，表示命中點的 alpha 值不滿足測試條件，而返回 false 則表示 alpha 測試通過。
- Alpha 值越高表示越不透明，越低表示越透明。Alpha 值通常在範圍 [0, 1] 之間，其中 0 表示完全透明，1 表示完全不透明。因此，alpha 值越接近 0，物體就越透明。
- 當 alpha test 失敗時，渲染管線可能會選擇丟棄（或者不繪製）該像素。這樣可以實現一些優化，尤其是對於需要進行透明物體的渲染。通過在 alpha test 階段剔除不符合條件的像素，可以減少需要進行透明混合計算的像素數量，提高渲染性能。
```cpp
    // Run a Falcor helper to extract the current hit point's geometric data
    VertexOut  vsOut = getVertexAttributes(PrimitiveIndex(), attribs);
```
- 使用 Falcor 提供的函式 getVertexAttributes 從預建的三角形交點屬性（BuiltInTriangleIntersectionAttributes）中提取當前擊中點的幾何數據。這些數據包括頂點的位置、法線、切線、副切線和紋理坐標等。
```cpp
    // Extracts the diffuse color from the material (the alpha component is opacity)
    ExplicitLodTextureSampler lodSampler = { 0 };  // Specify the tex lod/mip to use here
```
- 創建一個用於顯式紋理層級（mip level）的紋理取樣器。這可能用於提供更多細節的紋理信息。
```cpp
    float4 baseColor = sampleTexture(gMaterial.resources.baseColor, gMaterial.resources.samplerState,
        vsOut.texC, gMaterial.baseColor, EXTRACT_DIFFUSE_TYPE(gMaterial.flags), lodSampler);
```
- 通過使用 sampleTexture 函式從基礎顏色紋理中獲取顏色數據，其中包括 alpha 分量。這裡 gMaterial.resources.baseColor 是紋理資源，gMaterial.resources.samplerState 是取樣器狀態，vsOut.texC 是紋理坐標，gMaterial.baseColor 是材質的基礎顏色，EXTRACT_DIFFUSE_TYPE(gMaterial.flags) 和 lodSampler 則是用於進一步指定紋理的屬性。
```cpp
    // Test if this hit point fails a standard alpha test.
    return (baseColor.a < gMaterial.alphaThreshold);
```
- 比較基礎顏色的 alpha 分量與材質的 alpha 閾值。如果基礎顏色的 alpha 小於閾值，alphaTestFails 返回 true，表示 alpha 測試失敗，否則返回 false，表示 alpha 測試通過。
```cpp
}
```
- 總的來說，這段代碼用於確定擊中點是否通過 alpha 測試。
---
### getHitShadingData()
```cpp
//-------- For GI -------------

// Encapsulates a bunch of Falcor stuff into one simpler function.
//    -> This can only be called within a closest hit or any hit shader
ShadingData getHitShadingData(BuiltInTriangleIntersectionAttributes attribs)
{
```
- 這段程式碼是用於獲取擊中點的著色數據（Shading Data）。具體而言，它調用了 Falcor 渲染引擎提供的一些輔助函數來計算在當前擊中點的重要數據。
```cpp
    // Run a pair of Falcor helper functions to compute important data at the current hit point
    VertexOut  vsOut = getVertexAttributes(PrimitiveIndex(), attribs);
```
- 從頂點著色器的輸出（vsOut）中提取當前擊中點的幾何屬性，例如頂點位置、法線、紋理坐標等。
```cpp
    return prepareShadingData(vsOut, gMaterial, gCamera.posW, 0);
```
- 接受頂點著色器的輸出、材質信息、相機位置和其他可能的參數，並生成用於光照計算的著色數據。
- 最終，函數返回了 ShadingData 結構，其中包含在擊中點進行光照計算所需的信息，例如法線、紋理坐標、材質屬性等。這些數據可以在後續的著色階段中使用，以確定最終像素的顏色。
```cpp
}
```
---
### atan2_WAR()
```cpp
// A work-around function because some DXR drivers seem to have broken atan2() implementations
float atan2_WAR(float y, float x)
{
```
- 這是一個修正版的反正切函數 atan2，該函數計算給定 y 和 x 座標的反正切值。這裡的函數被稱為 atan2_WAR，其中 "WAR" 是指 "Work-Around"，意味著這是一種針對某些特定情況的 work-around（解決方法）。
```cpp
    if (x > 0.f)
        return atan(y / x);
    else if (x < 0.f && y >= 0.f)
        return atan(y / x) + M_PI;
    else if (x < 0.f && y < 0.f)
        return atan(y / x) - M_PI;
    else if (x == 0.f && y > 0.f)
        return M_PI / 2.f;
    else if (x == 0.f && y < 0.f)
        return -M_PI / 2.f;
    return 0.f; // x==0 && y==0 (undefined)
}
```
- 這段代碼考慮了 x 和 y 的不同符號情況，以確保計算結果的正確性。在一些情況下，原生的 atan2 函數可能會產生不正確的結果，這可能是由於對特定情境的處理不足引起的。因此，這段代碼通過不同的情況來避免可能的問題，確保返回正確的角度值。
- 總的來說，這是為了處理某些 atan2 可能出現的邊界情況而進行的調整。
---
### wsVectorToLatLong()
```cpp
// Convert our world space direction to a (u,v) coord in a latitude-longitude spherical map
float2 wsVectorToLatLong(float3 dir)
{
```
- 這是一個將世界空間中的三維向量轉換成緯度（Latitude）和經度（Longitude）坐標的函數。通常，這種轉換被用於將三維方向轉換為二維坐標，以便在紋理映射等應用中使用。
```cpp
    float3 p = normalize(dir);
```
- 使用 normalize 函數將給定的方向向量 dir 正規化。
```cpp
    // atan2_WAR is a work-around due to an apparent compiler bug in atan2
    float u = (1.f + atan2_WAR(p.x, -p.z) * M_1_PI) * 0.5f;
    float v = acos(p.y) * M_1_PI;
```
- 使用 atan2_WAR 函數（一種修正版的反正切函數，可能是為了處理某些邊界情況）計算經度 u 和緯度 v。
- 經度 u 被計算為 (1.f + atan2_WAR(p.x, -p.z) * M_1_PI) * 0.5f。這裡 atan2_WAR(p.x, -p.z) 計算了經度的角度值，並且通過一些調整確保它在合理的範圍內。然後，這個值被映射到範圍 [0, 1]，最終得到經度 u。
- 緯度 v 被計算為 acos(p.y) * M_1_PI。這裡 acos(p.y) 計算了緯度的角度值，然後通過將其映射到範圍 [0, 1] 得到緯度 v。
```cpp
    return float2(u, v);
```
- 最終，函數返回一個包含經度和緯度的 float2 向量。這樣的轉換通常在天空盒、球形映射等場景中使用，以方便紋理映射的應用。
```cpp
}
```
---
### getUniformHemisphereSample()
```cpp
// Get a uniform weighted random vector centered around a specified normal direction.
float3 getUniformHemisphereSample(inout uint randSeed, float3 hitNorm)
{
```
- 這是一個生成均勻分布在半球面上的樣本的函數，通常用於在基於物理的渲染（PBR）中進行光線追蹤，例如計算漫反射或環境光照。
- 這樣的樣本生成通常用於計算漫反射光線的方向，以模擬材質表面的漫反射反射。這有助於實現更真實的光線追蹤效果。
```cpp
    // Get 2 random numbers to select our sample with
    float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));
```
- 使用 nextRand 函數獲取兩個隨機數 randVal.x 和 randVal.y，這兩個值將用於選擇樣本。
```cpp
    // Cosine weighted hemisphere sample from RNG
    float3 bitangent = getPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(max(0.0f, 1.0f - randVal.x*randVal.x));
    float phi = 2.0f * 3.14159265f * randVal.y;
```
- 計算半球面上的樣本方向。為了實現均勻分布，這裡使用了在半球面上的坐標系統。
- 首先，計算垂直於法線方向的兩個向量：bitangent 和 tangent。
- 然後計算在這個局部坐標系統中的半球面樣本的極坐標 r 和 phi。
```cpp
    // Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * randVal.x;
```
- 最後，使用三角函數和法線方向，計算出均勻分布在半球面上的樣本方向。
```cpp
}
```
