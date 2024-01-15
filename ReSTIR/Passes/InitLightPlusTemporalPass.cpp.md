```cpp
#include "InitLightPlusTemporalPass.h"
```

```cpp
// Some global vars, used to simplify changing shader location & entry points
namespace {
    // Where is our shader located?
    const char* kFileRayTrace = "Tutorial11\\initLightPlusTemporal.rt.hlsl";

    // What are the entry points in that shader for various ray tracing shaders?
    const char* kEntryPointRayGen = "LambertShadowsRayGen";
    const char* kEntryPointMiss0 = "ShadowMiss";
    const char* kEntryShadowAnyHit = "ShadowAnyHit";
    const char* kEntryShadowClosestHit = "ShadowClosestHit";

    const char* kEntryPointMiss1 = "IndirectMiss";
    const char* kEntryIndirectAnyHit = "IndirectAnyHit";
    const char* kEntryIndirectClosestHit = "IndirectClosestHit";
};
```
- 這段程式碼使用了匿名的命名空間（anonymous namespace）。匿名的命名空間在 C++ 中是一個限定識別符的區域，其中的變數和函式只在當前編譯單元（source file）可見，不會對其他編譯單元造成衝突。
  - 在這個例子中，這個匿名的命名空間包含了全局變數和相應的初始化，這樣做的好處是可以在當前的編譯單元中使用這些變數，而不會對其他編譯單元產生干擾。這樣的設計可以提高程式碼的模組化和可維護性，同時防止全局變數名稱的衝突。
  - 匿名的命名空間通常用於實現在當前編譯單元中共享的私有的變數和函式，而不是將它們暴露給其他編譯單元。
- 這是一個包含全局變數的命名空間，用於存儲在光線追蹤中使用的一些變數和其相應的 HLSL（High-Level Shading Language）檔案的路徑和入口點。
  - 定義了光線追蹤的 HLSL 檔案的路徑。這個路徑是 "Tutorial11\initLightPlusTemporal.rt.hlsl"。
  - 定義了 Ray Generation Shader 入口點的名稱。在這裡，它是 "LambertShadowsRayGen"。
  - 定義了第一個 Miss Shader 入口點的名稱。在這裡，它是 "ShadowMiss"，用於 Shadow Ray 追蹤。
  - 定義了第一個 Any Hit Shader 入口點的名稱。在這裡，它是 "ShadowAnyHit"，用於 Shadow Ray 追蹤。
  - 定義了 Closest Hit Shader 入口點的名稱。在這裡，它是 "ShadowClosestHit"，用於 Shadow Ray 追蹤。
  - 定義了第二個 Miss Shader 入口點的名稱。在這裡，它是 "IndirectMiss"，用於 Indirect Ray 追蹤。
  - 定義了第二個 Any Hit Shader 入口點的名稱。在這裡，它是 "IndirectAnyHit"，用於 Indirect Ray 追蹤。
  - 定義了第二個 Closest Hit Shader 入口點的名稱。在這裡，它是 "IndirectClosestHit"，用於 Indirect Ray 追蹤。
  - 這樣的全局變數和定義通常用於指定射線追蹤程式的位置和相應的入口點，使代碼更易於維護和配置。
---
```cpp
bool InitLightPlusTemporalPass::initialize(RenderContext* pRenderContext, ResourceManager::SharedPtr pResManager)
{
```
- 這是一個 C++ 函式，屬於 InitLightPlusTemporalPass 類別的 initialize 成員函式。這個函式的主要目的是初始化光線追蹤通道，包括設置資源管理器、設定需要的紋理資源、建立光線追蹤的主要設定，如光線追蹤程序、場景等。

```cpp
    // Stash a copy of our resource manager so we can get rendering resources
    mpResManager = pResManager;
```
- 將傳入的 ResourceManager 的指標存儲在 InitLightPlusTemporalPass 類別的成員變數 mpResManager 中，以便後續使用。

```cpp
    mpResManager->requestTextureResources({ "WorldPosition", "WorldNormal", "MaterialDiffuse", "ReservoirPrev",
                                            "ReservoirCurr", "IndirectOutput" });
    mpResManager->requestTextureResource(ResourceManager::kOutputChannel);
    mpResManager->requestTextureResource(ResourceManager::kEnvironmentMap);
```
- 請求特定名稱的紋理資源，這些資源在後續的光線追蹤中可能會用到，如世界位置、世界法向量、材質漫反射等。
- 請求輸出通道的紋理資源。
- 請求環境貼圖的紋理資源。

```cpp
    // mpResManager->updateEnvironmentMap("Data/BackgroundImages/MonValley_G_DirtRoad_3k.hdr");
    mpResManager->setDefaultSceneName("Data/Scenes/forest/forest80.fscene");
```
- 設定默認的場景名稱，這裡是 "Data/Scenes/forest/forest80.fscene"。

```cpp
    // Create our wrapper around a ray tracing pass.  Tell it where our ray generation shader and ray-specific shaders are
    mpRays = RayLaunch::create(kFileRayTrace, kEntryPointRayGen);
```
- 創建一個 RayLaunch 的實例，並指定射線生成（Ray Generation Shader）的 HLSL 檔案和入口點。

```cpp
    // Add ray type #0 (shadow rays)
    mpRays->addMissShader(kFileRayTrace, kEntryPointMiss0);
    mpRays->addHitShader(kFileRayTrace, kEntryShadowClosestHit, kEntryShadowAnyHit);

    // Add ray type #1 (indirect GI rays)
    mpRays->addMissShader(kFileRayTrace, kEntryPointMiss1);
    mpRays->addHitShader(kFileRayTrace, kEntryIndirectClosestHit, kEntryIndirectAnyHit);
```
- 添加 Miss 和 Hit Group Shaders 的 HLSL 檔案和入口點，分別用於 Shadow Rays 和 Indirect GI Rays 的追蹤。

```cpp
    // Now that we've passed all our shaders in, compile and (if available) setup the scene
    mpRays->compileRayProgram();
    if (mpScene) mpRays->setScene(mpScene);
    return true;
```
- 編譯光線追蹤程序。
- 如果存在場景，將場景設定給 mpRays。
- 返回 true，表示初始化成功。

```cpp
}
```
- 總的來說，這個函式的作用是初始化光線追蹤通道，設定必要的資源和參數，以便後續進行光線追蹤渲染。
---
```cpp
void InitLightPlusTemporalPass::renderGui(Gui* pGui)
{
```
- 這是 InitLightPlusTemporalPass 類別的成員函式 renderGui，它用於在 GUI 中顯示一些選項，允許使用者在運行時進行設定。

```cpp
    // Add a toggle to turn on/off shooting of indirect GI rays
    int dirty = 0;
    dirty |= (int)pGui->addCheckBox(mDoDirectShadows ? "Shooting direct shadow rays" : "No direct shadow rays", mDoDirectShadows);
```
- 添加一個複選框到 GUI 中，用於開啟或關閉發射直射陰影射線的選項。如果 mDoDirectShadows 為真，則顯示 "Shooting direct shadow rays"，否則顯示 "No direct shadow rays"。

```cpp
    dirty |= (int)pGui->addCheckBox(mDoIndirectGI ? "Shooting global illumination rays" : "Skipping global illumination",
        mDoIndirectGI);
```
- 添加一個複選框到 GUI 中，用於開啟或關閉發射全局照明射線的選項。如果 mDoIndirectGI 為真，則顯示 "Shooting global illumination rays"，否則顯示 "Skipping global illumination"。

```cpp
    dirty |= (int)pGui->addCheckBox(mDoCosSampling ? "Use cosine sampling" : "Use uniform sampling", mDoCosSampling);
```
- 添加一個複選框到 GUI 中，用於切換使用餘弦採樣或均勻採樣的選項。如果 mDoCosSampling 為真，則顯示 "Use cosine sampling"，否則顯示 "Use uniform sampling"。

```cpp
    dirty |= (int)pGui->addCheckBox(mTemporalReuse ? "Temporal Reuse ON" : "Temporal Reuse OFF", mTemporalReuse);
```
- 添加一個複選框到 GUI 中，用於開啟或關閉時間重用的選項。如果 mTemporalReuse 為真，則顯示 "Temporal Reuse ON"，否則顯示 "Temporal Reuse OFF"。

```cpp
    if (dirty) setRefreshFlag();
```
- 如果有任何選項的狀態發生變化（dirty 不為零），則設置刷新標誌，以通知應用程序需要更新。

```cpp
}
```
- 總的來說，這個函式提供了一個使用者界面，讓使用者可以動態地調整一些影響光線追蹤行為的選項。
---
```cpp
bool InitLightPlusTemporalPass::hasCameraMoved()
{
```
- 這是 InitLightPlusTemporalPass 類別的成員函式 hasCameraMoved。這個函式的目的是檢查場景中的相機是否移動。

```cpp
    // Has our camera moved?
    return mpScene &&                      // No scene?  Then the answer is no
        mpScene->getActiveCamera() &&   // No camera in our scene?  Then the answer is no
        (mpLastCameraMatrix != mpScene->getActiveCamera()->getViewProjMatrix());   // Compare the current matrix with the last one
```
- mpScene &&：檢查是否存在場景。如果場景不存在，則相機無法移動，函式直接返回 false。
- mpScene->getActiveCamera() &&：檢查是否有活動相機。如果沒有活動相機，相機也無法移動，函式直接返回 false。
- (mpLastCameraMatrix != mpScene->getActiveCamera()->getViewProjMatrix());：比較上一次記錄的相機矩陣 mpLastCameraMatrix 與當前活動相機的視圖投影矩陣是否相等。如果不相等，表示相機移動過，函式返回 true；否則，返回 false。

```cpp
}
```
- 總的來說，這個函式用於檢查相機是否發生了移動，以協助在後續的渲染過程中進行相應的處理。
---
```cpp
void InitLightPlusTemporalPass::initScene(RenderContext* pRenderContext, Scene::SharedPtr pScene)
{
```
- 這是 InitLightPlusTemporalPass 類別的成員函式 initScene。這個函式的主要目的是在初始化場景時，將場景的信息存儲起來，包括場景的指針和相機的矩陣。

```cpp
    // Stash a copy of the scene and pass it to our ray tracer (if initialized)
    mpScene = std::dynamic_pointer_cast<RtScene>(pScene);
```
- 將傳入的 pScene 轉換為 RtScene 類別的共享指標，並將其存儲在 mpScene 中。std::dynamic_pointer_cast 用於安全地轉換智能指標類型，確保轉換的正確性。

```cpp
    // Grab a copy of the current scene's camera matrix (if it exists)
    if (mpScene && mpScene->getActiveCamera()) {
        mpLastCameraMatrix = mpScene->getActiveCamera()->getViewProjMatrix();
        mpCurrCameraMatrix = mpScene->getActiveCamera()->getViewProjMatrix();
    }
```
- 如果 mpScene 存在且擁有活動相機，則執行以下兩個步驟：
  - 將活動相機的視圖投影矩陣存儲在 mpLastCameraMatrix 中。
  - 同樣將相同的矩陣存儲在 mpCurrCameraMatrix 中。

```cpp
    if (mpRays) mpRays->setScene(mpScene);
```
- 如果光線追蹤類別 mpRays 已經初始化，則將場景設定給光線追蹤類別。

```cpp
}
```
- 總的來說，這個函式確保在初始化場景時，相應的場景和相機信息被正確地存儲起來，並在需要的情況下設定給其他相關的類別。
---
```cpp
void InitLightPlusTemporalPass::execute(RenderContext* pRenderContext)
{
```
- 這是 InitLightPlusTemporalPass 類別的成員函式 execute。這個函式的主要目的是執行初始化光線樣本和時間重用的光線追蹤過程。

```cpp
    // Get the output buffer we're writing into; clear it to black.
    Texture::SharedPtr pDstTex = mpResManager->getClearedTexture(ResourceManager::kOutputChannel, vec4(0.0f, 0.0f, 0.0f, 0.0f));
```
- 獲取要寫入的輸出緩衝，並將其清除為黑色。這個緩衝將用於存儲光線追蹤的結果。

```cpp
    // Do we have all the resources we need to render?  If not, return
    if (!pDstTex || !mpRays || !mpRays->readyToRender()) return;
```
- 檢查是否擁有渲染所需的所有資源，如果有缺失則直接返回。

```cpp
    // If the camera in our current scene has moved, we want to reset mInitLightPerPixel
    if (hasCameraMoved())
    {
        mpLastCameraMatrix = mpCurrCameraMatrix;
        mpCurrCameraMatrix = mpScene->getActiveCamera()->getViewProjMatrix();
    }
```
- 如果相機發生移動，重置 mInitLightPerPixel。同時更新相機矩陣的記錄。
- **問題**：這裡的代碼並沒有重置 mInitLightPerPixel?

```cpp
    // Set our ray tracing shader variables
    auto rayGenVars = mpRays->getRayGenVars();
    rayGenVars["RayGenCB"]["gMinT"]       = mpResManager->getMinTDist();
    rayGenVars["RayGenCB"]["gFrameCount"] = mFrameCount++;
    // For ReSTIR - update the toggle in the shader
    rayGenVars["RayGenCB"]["gInitLight"]  = mInitLightPerPixel;
    rayGenVars["RayGenCB"]["gTemporalReuse"] = mTemporalReuse;
    rayGenVars["RayGenCB"]["gDoIndirectGI"] = mDoIndirectGI;
    rayGenVars["RayGenCB"]["gCosSampling"] = mDoCosSampling;
    rayGenVars["RayGenCB"]["gDirectShadow"] = mDoDirectShadows;
    rayGenVars["RayGenCB"]["gLastCameraMatrix"] = mpLastCameraMatrix;

    // Pass our G-buffer textures down to the HLSL so we can shade
    rayGenVars["gPos"]         = mpResManager->getTexture("WorldPosition");
    rayGenVars["gNorm"]        = mpResManager->getTexture("WorldNormal");
    rayGenVars["gDiffuseMatl"] = mpResManager->getTexture("MaterialDiffuse");

    // For ReSTIR - update the buffer storing reservoir (weight sum, chosen light index, number of candidates seen)
    rayGenVars["gReservoirPrev"] = mpResManager->getTexture("ReservoirPrev");
    rayGenVars["gReservoirCurr"] = mpResManager->getTexture("ReservoirCurr");
    rayGenVars["gIndirectOutput"] = mpResManager->getTexture("IndirectOutput");

    // Set our environment map texture for indirect rays that miss geometry
    auto missVars = mpRays->getMissVars(1);       // Remember, indirect rays are ray type #1
    missVars["gEnvMap"] = mpResManager->getTexture(ResourceManager::kEnvironmentMap);
```
- 設定光線追蹤的 shader 變數，包括傳遞相關的光線追蹤選項和 G-buffer 相關的紋理。

```cpp
    // Shoot our rays and shade our primary hit points
    mpRays->execute( pRenderContext, mpResManager->getScreenSize() );
```
- 執行光線追蹤。這將發射光線，並在命中點進行陰影和環境光的計算。

```cpp
    // For ReSTIR - toggle to false so we only sample a random candidate for the first frame
    mInitLightPerPixel = false;
```
- 將 mInitLightPerPixel 設置為 false，以確保僅在第一幀中對光源進行隨機抽樣。
- **問題**：為何只在第一幀中對光源進行隨機抽樣? 印象中應該是每一幀的第一步都要做隨機抽樣?

```cpp
}
```
- 總的來說，這個函式主要負責執行初始化光線樣本和時間重用的光線追蹤過程，計算陰影和環境光，並將結果寫入指定的輸出緩衝中。
