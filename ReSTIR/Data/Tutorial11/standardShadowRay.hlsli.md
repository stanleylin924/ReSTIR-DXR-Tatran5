### ShadowRayPayload
```cpp
// Payload for our shadow rays.
struct ShadowRayPayload
{
    float visFactor;  // Will be 1.0 for fully lit, 0.0 for fully shadowed
};
```
---
### shadowRayVisibility()
```cpp
// A utility function to trace a shadow ray and return 1 if no shadow and 0 if shadowed.
//    -> Note:  This assumes the shadow hit programs and miss programs are index 0!
float shadowRayVisibility(float3 origin, float3 direction, float minT, float maxT)
{
```
- 這段程式碼是一個用於確定可見性的函數。它使用光線追蹤（ray tracing）的概念，透過追蹤從某個點（origin）沿著某個方向（direction）的陰影射線（shadow ray），來確定光線是否被其他物體所阻擋。
- 回傳值：1 表示可見（未被遮擋），0 表示不可見（被遮擋）。
```cpp
    // Setup our shadow ray
    RayDesc ray;
    ray.Origin = origin;        // Where does it start?
    ray.Direction = direction;  // What direction do we shoot it?
    ray.TMin = minT;            // The closest distance we'll count as a hit
    ray.TMax = maxT;            // The farthest distance we'll count as a hit
```
- 創建一個射線（RayDesc），指定起始點、方向以及追蹤的最小和最大距離。
```cpp
    // Our shadow rays are *assumed* to hit geometry; this miss shader changes this to 1.0 for "visible"
    ShadowRayPayload payload = { 0.0f };
```
- 設定陰影射線的追蹤結果，使用 ShadowRayPayload 來存儲。
```cpp
    // Query if anything is between the current point and the light
    TraceRay(gRtScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        0xFF, 0, hitProgramCount, 0, ray, payload);
```
- 使用 TraceRay 函數追蹤光線。RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH 表示一旦找到第一個擊中點，就結束搜尋。RAY_FLAG_SKIP_CLOSEST_HIT_SHADER 表示不執行 closest hit shader。
```cpp
    // Return our ray payload (which is 1 for visible, 0 for occluded)
    return payload.visFactor;
```
- 從 payload.visFactor 中獲取陰影射線的可見性，其中 1 表示可見，0 表示被遮擋。
```cpp
}
```
---
### ShadowMiss()
```cpp
// What code is executed when our ray misses all geometry?
[shader("miss")]
void ShadowMiss(inout ShadowRayPayload rayData)
{
    // If we miss all geometry, then the light is visibile
    rayData.visFactor = 1.0f;
}
```
- ShadowMiss 函數在射線未與場景中的任何物體相交時被調用，並且它設置 rayData.visFactor 為 1.0f，表示對於擊中點而言光源是可見的（未被遮擋），因此擊中點不在陰影中。
---
### ShadowAnyHit()
```cpp
// What code is executed when our ray hits a potentially transparent surface?
[shader("anyhit")]
void ShadowAnyHit(inout ShadowRayPayload rayData, BuiltInTriangleIntersectionAttributes attribs)
{
    // Is this a transparent part of the surface?  If so, ignore this hit
    if (alphaTestFails(attribs))
        IgnoreHit();
}
```
- ShadowAnyHit 函數在射線與場景中的物體相交時被調用，並且它檢查該相交處是否是表面的透明部分。如果是透明的，則使用 IgnoreHit() 函數忽略這次相交。
---
### ShadowClosestHit()
```cpp
// What code is executed when we have a new closest hitpoint?
[shader("closesthit")]
void ShadowClosestHit(inout ShadowRayPayload rayData, BuiltInTriangleIntersectionAttributes attribs)
{
}
```
- 在這裡沒有用到 closest hit shader，因此不執行任何操作。