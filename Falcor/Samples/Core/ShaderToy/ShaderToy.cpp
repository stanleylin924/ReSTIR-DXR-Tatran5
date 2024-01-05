/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "ShaderToy.h"

ShaderToy::~ShaderToy()
{
}

void ShaderToy::onLoad(SampleCallbacks* pSample, const RenderContext::SharedPtr& pRenderContext)
{
    // create rasterizer state
    RasterizerState::Desc rsDesc;
    mpNoCullRastState = RasterizerState::create(rsDesc);

    // Depth test
    DepthStencilState::Desc dsDesc;
	dsDesc.setDepthTest(false);
	mpNoDepthDS = DepthStencilState::create(dsDesc);

    // Blend state
    BlendState::Desc blendDesc;
    mpOpaqueBS = BlendState::create(blendDesc);

    // Texture sampler
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear).setMaxAnisotropy(8);
    mpLinearSampler = Sampler::create(samplerDesc);

    // Load shaders
    mpMainPass = FullScreenPass::create("toyContainer.hlsl");

    // Create Constant buffer
    mpToyVars = GraphicsVars::create(mpMainPass->getProgram()->getReflector());

    // Get buffer finding
    mToyCBBinding = mpMainPass->getProgram()->getReflector()->getDefaultParameterBlock()->getResourceBinding("ToyCB");
}

void ShaderToy::onFrameRender(SampleCallbacks* pSample, const RenderContext::SharedPtr& pRenderContext, const Fbo::SharedPtr& pTargetFbo)
{
    // iResolution
    float width = (float)pTargetFbo->getWidth();
    float height = (float)pTargetFbo->getHeight();
    ParameterBlock* pDefaultBlock = mpToyVars->getDefaultBlock().get();
    pDefaultBlock->getConstantBuffer(mToyCBBinding, 0)["iResolution"] = glm::vec2(width, height);;

    // iGlobalTime
    float iGlobalTime = (float)pSample->getCurrentTime();  
    pDefaultBlock->getConstantBuffer(mToyCBBinding, 0)["iGlobalTime"] = iGlobalTime;

    // run final pass
    pRenderContext->setGraphicsVars(mpToyVars);
    mpMainPass->execute(pRenderContext.get());
}

void ShaderToy::onShutdown(SampleCallbacks* pSample)
{
}

bool ShaderToy::onKeyEvent(SampleCallbacks* pSample, const KeyboardEvent& keyEvent)
{
    bool bHandled = false;
    {
        if(keyEvent.type == KeyboardEvent::Type::KeyPressed)
        {
            //switch(keyEvent.key)
            //{
            //default:
            //    bHandled = false;
            //}
        }
    }
    return bHandled;
}

bool ShaderToy::onMouseEvent(SampleCallbacks* pSample, const MouseEvent& mouseEvent)
{
    bool bHandled = false;
    return bHandled;
}

void ShaderToy::onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height)
{
    mAspectRatio = (float(width) / float(height));
}

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
#else
int main(int argc, char** argv)
#endif
{
    ShaderToy::UniquePtr pRenderer = std::make_unique<ShaderToy>();
    SampleConfig config;
    config.windowDesc.width = 1280;
    config.windowDesc.height = 720;
    config.deviceDesc.enableVsync = true;
    config.windowDesc.resizableWindow = true;
    config.windowDesc.title = "Falcor Shader Toy";
#ifdef _WIN32
    Sample::run(config, pRenderer);
#else
    config.argc = (uint32_t)argc;
    config.argv = argv;
    Sample::run(config, pRenderer);
#endif
    return 0;
}