```cpp
#pragma once
#include "../SharedUtils/RenderPass.h"
#include "../SharedUtils/RayLaunch.h"
```

```cpp
class InitLightPlusTemporalPass : public ::RenderPass, inherit_shared_from_this<::RenderPass, InitLightPlusTemporalPass>
{
```
- 定義了一個名為 InitLightPlusTemporalPass 的類別，這是一個繼承自 RenderPass 的渲染通道（render pass）。這個類別負責初始化光照樣本和處理時間上的重用。

```cpp
public:

    bool mTemporalReuse = true;
    bool mInitLightPerPixel = true;
    // Recursive ray tracing can be slow.  Add a toggle to disable, to allow you to manipulate the scene
    bool mDoIndirectGI = true;
    bool mDoCosSampling = true;
    bool mDoDirectShadows = true;
```
- 是否啟用時間上的重用（temporal reuse）。
- 是否在每個像素初始化光照樣本。
- 是否執行間接全局照明（global illumination）。
- 是否使用餘弦取樣（cosine sampling）。
- 是否計算直接陰影。

```cpp
    using SharedPtr = std::shared_ptr<InitLightPlusTemporalPass>;
    using SharedConstPtr = std::shared_ptr<const InitLightPlusTemporalPass>;
```
- 定義一個別名 SharedPtr，它是一個指向 InitLightPlusTemporalPass 物件的可修改的共享指標的型別。
- 定義一個別名 SharedConstPtr，它是一個指向 const InitLightPlusTemporalPass 物件的共享指標的型別。而 const 修飾符表示指標所指向的物件是不可變的。這樣的指標可以被用於唯讀的操作，防止對該物件進行修改。

```cpp
    static SharedPtr create() { return SharedPtr(new InitLightPlusTemporalPass()); }
    virtual ~InitLightPlusTemporalPass() = default;
```
- 提供一個靜態函式 create，用於創建 InitLightPlusTemporalPass 的共享指標。
- 虛擬析構函式，使用默認實現。

```cpp
protected:
    InitLightPlusTemporalPass() : ::RenderPass("Intialize & Temporal Reuse", "Intialize Lights and Temporal Reuse Options") {}
```
- 受保護的構造函式，設置 RenderPass 的名稱和描述。

```cpp
    // Implementation of RenderPass interface
    bool initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager) override;
    void initScene(RenderContext* pRenderContext, Scene::SharedPtr pScene) override;
    void execute(RenderContext* pRenderContext) override;
    void renderGui(Gui* pGui) override;
```
- 實現了 RenderPass 接口的初始化函式，用於初始化渲染通道。
- 實現了 RenderPass 接口的 initScene 函式，用於初始化場景。
- 實現了 RenderPass 接口的 execute 函式，用於執行渲染通道的主要邏輯。
- 實現了 RenderPass 接口的 renderGui 函式，用於渲染通道的 GUI。

```cpp
    // Override some functions that provide information to the RenderPipeline class
    bool requiresScene() override { return true; }
    bool usesRayTracing() override { return true; }
```
- 實現了 RenderPass 接口的 requiresScene 函式，表示此通道需要場景信息。
- 實現了 RenderPass 接口的 usesRayTracing 函式，表示此通道使用光線追蹤。

```cpp
    // A helper utility to determine if the current scene (if any) has had any camera motion
    bool hasCameraMoved();
```
- 一個輔助函式，用於確定當前場景（如果有）的相機是否移動。

```cpp
    // Rendering state
    RayLaunch::SharedPtr                    mpRays;                 ///< Our wrapper around a DX Raytracing pass
    RtScene::SharedPtr                      mpScene;                ///< Our scene file (passed in from app)
    mat4                          mpLastCameraMatrix;
    mat4                          mpCurrCameraMatrix;
```
- 持有一個光線發射器的共享指標，這是一個用於處理光線追蹤的類別。
- 持有一個場景的共享指標，這是一個用於處理場景相關的類別。
- 存儲上一幀和當前幀相機矩陣的成員變數。

```cpp
    // Various internal parameters
    uint32_t                                mFrameCount = 0x1337u;  ///< A frame counter to vary random numbers over time
```
- 一個幀計數器，用於在時間上變化的隨機數。

```cpp
};
```
