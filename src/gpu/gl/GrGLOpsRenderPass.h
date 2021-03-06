/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrGLOpsRenderPass_DEFINED
#define GrGLOpsRenderPass_DEFINED

#include "src/gpu/GrOpsRenderPass.h"

#include "src/gpu/GrOpFlushState.h"
#include "src/gpu/gl/GrGLGpu.h"
#include "src/gpu/gl/GrGLRenderTarget.h"

class GrGLGpu;
class GrGLRenderTarget;

class GrGLOpsRenderPass : public GrOpsRenderPass {
/**
 * We do not actually buffer up draws or do any work in the this class for GL. Instead commands
 * are immediately sent to the gpu to execute. Thus all the commands in this class are simply
 * pass through functions to corresponding calls in the GrGLGpu class.
 */
public:
    GrGLOpsRenderPass(GrGLGpu* gpu) : fGpu(gpu) {}

    void begin() override {
        fGpu->beginCommandBuffer(fRenderTarget, fContentBounds, fOrigin, fColorLoadAndStoreInfo,
                                 fStencilLoadAndStoreInfo);
    }

    void end() override {
        fGpu->endCommandBuffer(fRenderTarget, fColorLoadAndStoreInfo, fStencilLoadAndStoreInfo);
    }

    void inlineUpload(GrOpFlushState* state, GrDeferredTextureUploadFn& upload) override {
        state->doUpload(upload);
    }

    void set(GrRenderTarget*, const SkIRect& contentBounds, GrSurfaceOrigin,
             const LoadAndStoreInfo&, const StencilLoadAndStoreInfo&);

    void reset() {
        fRenderTarget = nullptr;
    }

private:
    GrGpu* gpu() override { return fGpu; }

    void setupGeometry(const GrBuffer* vertexBuffer, int baseVertex, const GrBuffer* instanceBuffer,
                       int baseInstance);

    bool onBindPipeline(const GrProgramInfo& programInfo, const SkRect& drawBounds) override;
    void onSetScissorRect(const SkIRect& scissor) override;
    bool onBindTextures(const GrPrimitiveProcessor& primProc, const GrPipeline& pipeline,
                        const GrSurfaceProxy* const primProcTextures[]) override;
    void onBindBuffers(const GrBuffer* indexBuffer, const GrBuffer* instanceBuffer,
                       const GrBuffer* vertexBuffer, GrPrimitiveRestart) override;
    void onDraw(int vertexCount, int baseVertex) override;
    void onDrawIndexed(int indexCount, int baseIndex, uint16_t minIndexValue,
                       uint16_t maxIndexValue, int baseVertex) override;
    void onDrawInstanced(int instanceCount, int baseInstance, int vertexCount,
                         int baseVertex) override;
    void onDrawIndexedInstanced(int indexCount, int baseIndex, int instanceCount, int baseInstance,
                                int baseVertex) override;
    void onClear(const GrFixedClip& clip, const SkPMColor4f& color) override;
    void onClearStencilClip(const GrFixedClip& clip, bool insideStencilMask) override;

    GrGLGpu* fGpu;
    SkIRect fContentBounds;
    LoadAndStoreInfo fColorLoadAndStoreInfo;
    StencilLoadAndStoreInfo fStencilLoadAndStoreInfo;

    // Per-pipeline state.
    GrPrimitiveType fPrimitiveType;
    GrGLAttribArrayState* fAttribArrayState = nullptr;

    // If using an index buffer, this gets set during onBindBuffers. It is either the CPU address of
    // the indices, or nullptr if they reside physically in GPU memory.
    const uint16_t* fIndexPointer;

    // We may defer binding of instance and vertex buffers because GL does not always support a base
    // instance and/or vertex.
    sk_sp<const GrBuffer> fDeferredInstanceBuffer;
    sk_sp<const GrBuffer> fDeferredVertexBuffer;

    typedef GrOpsRenderPass INHERITED;
};

#endif

