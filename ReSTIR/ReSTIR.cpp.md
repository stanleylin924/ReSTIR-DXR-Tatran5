```cpp
#include "Falcor.h"
#include <iostream>
#include "Passes/InitLightPlusTemporalPass.h"
#include "Passes/SpatialReusePass.h"
#include "Passes/UpdateReservoirPlusShadePass.h"
#include "../CommonPasses/LightProbeGBufferPass.h"
#include "../CommonPasses/SimpleAccumulationPass.h"
#include "../SharedUtils/RenderingPipeline.h"
```

```cpp
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
```
- 這個 WinMain 函式是 DirectX 視窗應用程式的進入點。

```cpp
    // Toggle
    bool temporalReuse = true;
    bool spatialReuse = true;
    bool globalIllum = true;
```
- 啟用或禁用時間重用（temporal reuse）。
- 啟用或禁用空間重用（spatial reuse）。
- 啟用或禁用全局照明（global illumination）。

```cpp
    // Create our rendering pipeline
    RenderingPipeline *pipeline = new RenderingPipeline();
```
- 創建一個名為 pipeline 的 RenderingPipeline 實例。這是一個用於管理渲染流程和通道的自定義渲染管道。

```cpp
    // Add passes into our pipeline
    pipeline->setPass(0, LightProbeGBufferPass::create());
```
- 將第一個通道設置為 LightProbeGBufferPass，這是用於生成 G-Buffer 的通道。

```cpp
    // Only need scene to load once in first pass among those below (check pass::initialize())
    auto initLightPlusTemporalPass = InitLightPlusTemporalPass::create();
    initLightPlusTemporalPass->mTemporalReuse = temporalReuse;
    initLightPlusTemporalPass->mDoIndirectGI = globalIllum;
    pipeline->setPass(1, initLightPlusTemporalPass);
```
- 創建一個 InitLightPlusTemporalPass 的實例，這是一個用於初始化光照樣本和時間重用的通道。
- 設置 InitLightPlusTemporalPass 的 mTemporalReuse 屬性，啟用或禁用該通道的時間重用。
- 設置 InitLightPlusTemporalPass 的 mDoIndirectGI 屬性，啟用或禁用該通道的全局照明。
- 將第二個通道設置為 InitLightPlusTemporalPass。

```cpp
    auto spatialReusePass = SpatialReusePass::create();
    spatialReusePass->mSpatialReuse = spatialReuse;
    pipeline->setPass(2, spatialReusePass);
```
- 創建一個 SpatialReusePass 的實例，這是一個用於空間重用的通道。
- 設置 SpatialReusePass 的 mSpatialReuse 屬性，啟用或禁用該通道的空間重用。
- 將第三個通道設置為 SpatialReusePass。

```cpp
    pipeline->setPass(3, UpdateReservoirPlusShadePass::create());
```
- 將第四個通道設置為 UpdateReservoirPlusShadePass，這是用於更新蓄水池（reservoir）和陰影的通道。

```cpp
    pipeline->setPass(4, SimpleAccumulationPass::create(ResourceManager::kOutputChannel));
```
- 將第五個通道設置為 SimpleAccumulationPass，這是一個簡單的累積通道。

```cpp
    // Define a set of config / window parameters for our program
    SampleConfig config;
    config.windowDesc.title = "ReSTIR";
    config.windowDesc.resizableWindow = true;
```
- 創建一個 SampleConfig 的實例，包含有關應用程式配置的一些信息，如窗口標題等。
- 設置應用程式窗口的標題為 "ReSTIR"。
- 允許應用程式的窗口大小可調整。

```cpp
    // Start our program!
    RenderingPipeline::run(pipeline, config);
```
- 運行 RenderingPipeline 的 run 函式，開始執行整個應用程式的渲染流程。

```cpp
}
```
- 總體來說，這段程式碼建立了一個渲染管道，配置了不同的通道，並啟動了應用程式的運行。渲染管道中的各個通道可能涉及到不同的渲染階段，如光照計算、蓄水池更新、陰影計算等。
