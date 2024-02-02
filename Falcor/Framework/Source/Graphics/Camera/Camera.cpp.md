```cpp
#include "Framework.h"
#include "Camera.h"
#include "Utils/AABB.h"
#include "Utils/Math/FalcorMath.h"
#include "API/ConstantBuffer.h"

namespace Falcor
{
    // Default dimensions of full frame cameras and 35mm film
    const float Camera::kDefaultFrameHeight = 24.0f;

    Camera::Camera()
    {
    }

    Camera::~Camera() = default;

    Camera::SharedPtr Camera::create()
    {
        Camera* pCamera = new Camera;
        return SharedPtr(pCamera);
    }

    void Camera::beginFrame()
    {
        if (mJitterPattern.pGenerator)
        {
            vec2 jitter = mJitterPattern.pGenerator->next();
            jitter *= mJitterPattern.scale;
            setJitterInternal(jitter.x, jitter.y);
        }

        mData.prevViewProjMat = mViewProjMatNoJitter;
        mData.rightEyePrevViewProjMat = mData.rightEyeViewProjMat;
    }

    void Camera::calculateCameraParameters() const
    {
        if (mDirty)
        {
            // Interpret focal length of 0 as 0 FOV. Technically 0 FOV should be focal length of infinity.
            const float fovY = mData.focalLength == 0.0f ? 0.0f : focalLengthToFovY(mData.focalLength, mData.frameHeight);

            if (mEnablePersistentViewMat)
            {
                mData.viewMat = mPersistentViewMat;
            }
            else
            {
                mData.viewMat = glm::lookAt(mData.posW, mData.target, mData.up);
            }

            // if camera projection is set to be persistent, don't override it.
            if (mEnablePersistentProjMat)
            {
                mData.projMat = mPersistentProjMat;
            }
            else
            {
                if (fovY != 0.f)
                {
                    mData.projMat = glm::perspective(fovY, mData.aspectRatio, mData.nearZ, mData.farZ);
                }
                else
                {
                    // Take the length of look-at vector as half a viewport size
                    const float halfLookAtLength = length(mData.posW - mData.target) * 0.5f;
                    mData.projMat = glm::ortho(-halfLookAtLength, halfLookAtLength, -halfLookAtLength, halfLookAtLength, mData.nearZ, mData.farZ);
                }
            }

            // Build jitter matrix
            // (jitterX and jitterY are expressed as subpixel quantities divided by the screen resolution
            //  for instance to apply an offset of half pixel along the X axis we set jitterX = 0.5f / Width)
            glm::mat4 jitterMat(1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                2.0f * mData.jitterX, 2.0f * mData.jitterY, 0.0f, 1.0f);
            // Apply jitter matrix to the projection matrix
            mViewProjMatNoJitter = mData.projMat * mData.viewMat;
            mData.projMat = jitterMat * mData.projMat;

            mData.viewProjMat = mData.projMat * mData.viewMat;
            mData.invViewProj = glm::inverse(mData.viewProjMat);

            // Extract camera space frustum planes from the VP matrix
            // See: https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
            glm::mat4 tempMat = glm::transpose(mData.viewProjMat);
            for (int i = 0; i < 6; i++)
            {
                glm::vec4 plane = (i & 1) ? tempMat[i >> 1] : -tempMat[i >> 1];
                if(i != 5) // Z range is [0, w]. For the 0 <= z plane we don't need to add w
                {
                    plane += tempMat[3];
                }

                mFrustumPlanes[i].xyz = glm::vec3(plane);
                mFrustumPlanes[i].sign = glm::sign(mFrustumPlanes[i].xyz);
                mFrustumPlanes[i].negW = -plane.w;
            }

            // Ray tracing related vectors
            mData.cameraW = glm::normalize(mData.target - mData.posW) * mData.focalDistance;
            mData.cameraU = glm::normalize(glm::cross(mData.cameraW, mData.up));
            mData.cameraV = glm::normalize(glm::cross(mData.cameraU, mData.cameraW));
            const float ulen = mData.focalDistance * tanf(fovY * 0.5f) * mData.aspectRatio;
            mData.cameraU *= ulen;
            const float vlen = mData.focalDistance * tanf(fovY * 0.5f);
            mData.cameraV *= vlen;

            mDirty = false;
        }
    }

    const glm::mat4& Camera::getViewMatrix() const
    {
        calculateCameraParameters();
        return mData.viewMat;
    }

    const glm::mat4& Camera::getProjMatrix() const
    {
        calculateCameraParameters();
        return mData.projMat;
    }

    const glm::mat4& Camera::getViewProjMatrix() const
    {
        calculateCameraParameters();
        return mData.viewProjMat;
    }
```
- 在所提供的相機類別（Camera）中，getViewMatrix()、getProjMatrix()、getViewProjMatrix() 這三個函數用於獲取不同的變換矩陣：
    - getViewMatrix(): 返回相機的視圖矩陣（View Matrix）。視圖矩陣是將場景從世界坐標系轉換到相機的局部坐標系，描述相機的位置、朝向和上向量。
    - getProjMatrix(): 返回相機的投影矩陣（Projection Matrix）。投影矩陣用於將視景體中的三維坐標映射到裁剪空間，通常包含透視或正交投影的相關參數。
    - getViewProjMatrix(): 返回相機的視圖投影矩陣（View-Projection Matrix）。這是將視圖矩陣和投影矩陣相乘的結果，表示將場景從世界坐標系轉換到裁剪空間。
- 簡而言之，getViewMatrix() 提供了相機的視圖矩陣，getProjMatrix() 提供了相機的投影矩陣，而 getViewProjMatrix() 提供了將視圖和投影兩者合併的視圖投影矩陣。在渲染中，通常會使用 getViewProjMatrix() 將場景中的物體變換到裁剪空間，以進行後續的光栅化和顯示。
```cpp
    const glm::mat4& Camera::getInvViewProjMatrix() const
    {
        calculateCameraParameters();
        return mData.invViewProj;
    }

    void Camera::setProjectionMatrix(const glm::mat4& proj)
    {
        mDirty = true;
        mPersistentProjMat = proj;
        togglePersistentProjectionMatrix(true);
    }

    void Camera::setViewMatrix(const glm::mat4& view)
    {
        mDirty = true;
        mPersistentViewMat = view;
        togglePersistentViewMatrix(true);
    }

    void Camera::togglePersistentProjectionMatrix(bool persistent)
    {
        mEnablePersistentProjMat = persistent;
    }

    void Camera::togglePersistentViewMatrix(bool persistent)
    {
        mEnablePersistentViewMat = persistent;
    }

    bool Camera::isObjectCulled(const BoundingBox& box) const
    {
        calculateCameraParameters();

        bool isInside = true;
        // AABB vs. frustum test
        // See method 4b: https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/
        for (int plane = 0; plane < 6; plane++)
        {
            glm::vec3 signedExtent = box.extent * mFrustumPlanes[plane].sign;
            float dr = glm::dot(box.center + signedExtent, mFrustumPlanes[plane].xyz);
            isInside = isInside && (dr > mFrustumPlanes[plane].negW);
        }

        return !isInside;
    }

    void Camera::setRightEyeMatrices(const glm::mat4& view, const glm::mat4& proj)
    {
        mData.rightEyeViewMat = view;
        mData.rightEyeProjMat = proj;
        mData.rightEyeViewProjMat = proj * view;
    }

    void Camera::setIntoConstantBuffer(ConstantBuffer* pCB, const std::string& varName) const
    {
        size_t offset = pCB->getVariableOffset(varName + ".viewMat");

        if (offset == ConstantBuffer::kInvalidOffset)
        {
            logWarning("Camera::setIntoConstantBuffer() - variable \"" + varName + "\"not found in constant buffer\n");
            return;
        }

        setIntoConstantBuffer(pCB, offset);
    }

    void Camera::setIntoConstantBuffer(ConstantBuffer* pBuffer, const std::size_t& offset) const
    {
        calculateCameraParameters();
        assert(offset + getShaderDataSize() <= pBuffer->getSize());

        pBuffer->setBlob(&mData, offset, getShaderDataSize());
    }

    void Camera::move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    {
        setPosition(position);
        setTarget(target);
        setUpVector(up);
    }

    void Camera::setPatternGenerator(const PatternGenerator::SharedPtr& pGenerator, const vec2& scale)
    {
        mJitterPattern.pGenerator = pGenerator;
        mJitterPattern.scale = scale;
        if (!pGenerator)
        {
            setJitterInternal(0, 0);
        }
    }

    void Camera::setJitter(float jitterX, float jitterY)
    {
        if (mJitterPattern.pGenerator)
        {
            logWarning("Camera::setJitter() called when a pattern-generator object was attached to the camera. Detaching the pattern-generator");
            mJitterPattern.pGenerator = nullptr;
        }
        setJitterInternal(jitterX, jitterY);
    }

    void Camera::setJitterInternal(float jitterX, float jitterY)
    { 
        mData.jitterX = jitterX; 
        mData.jitterY = jitterY; 
        mDirty = true; 
    }
}
```