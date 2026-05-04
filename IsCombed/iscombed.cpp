/*
    Copyright (C) 2026 HolyWu

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <cmath>

#include <memory>
#include <string>

#include <VapourSynth4.h>
#include <VSHelper4.h>

using namespace std::string_literals;

struct IsCombedData final {
    VSNode* node;
    const VSVideoInfo* vi;
    VSVideoFormat format;
    int cthresh, blockx, blocky, mi, metric;
    bool chroma;
    int arraySize, cthresh6, cthreshsq, heighta, widtha, xBlocks4, xHalf, xShift, yHalf, yShift;
    int(*filter)(const VSFrame* src, VSFrame* cmask, const IsCombedData* VS_RESTRICT d, const VSAPI* vsapi) noexcept;
};

static inline bool isPowerOf2(const int i) noexcept {
    return i && !(i & (i - 1));
}

template<typename pixel_t>
static int filter(const VSFrame* src, VSFrame* cmask, const IsCombedData* VS_RESTRICT d, const VSAPI* vsapi) noexcept {
    for (int plane = 0; plane < (d->chroma ? 3 : 1); plane++) {
        const int width = vsapi->getFrameWidth(src, plane);
        const int height = vsapi->getFrameHeight(src, plane);
        const ptrdiff_t srcStride = vsapi->getStride(src, plane) / sizeof(pixel_t);
        const ptrdiff_t cmkStride = vsapi->getStride(cmask, plane);
        const pixel_t* srcp = reinterpret_cast<const pixel_t*>(vsapi->getReadPtr(src, plane));
        uint8_t* VS_RESTRICT cmkp = vsapi->getWritePtr(cmask, plane);

        const pixel_t* srcppp = srcp - srcStride * 2;
        const pixel_t* srcpp = srcp - srcStride;
        const pixel_t* srcpn = srcp + srcStride;
        const pixel_t* srcpnn = srcp + srcStride * 2;

        memset(cmkp, 0, cmkStride * height);

        if (d->metric == 0) {
            for (int x = 0; x < width; x++) {
                const int sFirst = srcp[x] - srcpn[x];
                if ((sFirst > d->cthresh || sFirst < -d->cthresh) && std::abs(srcpnn[x] + 4 * srcp[x] + srcpnn[x] - 3 * (srcpn[x] + srcpn[x])) > d->cthresh6)
                    cmkp[x] = 0xFF;
            }
            srcppp += srcStride;
            srcpp += srcStride;
            srcp += srcStride;
            srcpn += srcStride;
            srcpnn += srcStride;
            cmkp += cmkStride;

            for (int x = 0; x < width; x++) {
                const int sFirst = srcp[x] - srcpp[x];
                const int sSecond = srcp[x] - srcpn[x];
                if (((sFirst > d->cthresh && sSecond > d->cthresh) || (sFirst < -d->cthresh && sSecond < -d->cthresh)) &&
                    std::abs(srcpnn[x] + 4 * srcp[x] + srcpnn[x] - 3 * (srcpp[x] + srcpn[x])) > d->cthresh6)
                    cmkp[x] = 0xFF;
            }
            srcppp += srcStride;
            srcpp += srcStride;
            srcp += srcStride;
            srcpn += srcStride;
            srcpnn += srcStride;
            cmkp += cmkStride;

            for (int y = 2; y < height - 2; y++) {
                for (int x = 0; x < width; x++) {
                    const int sFirst = srcp[x] - srcpp[x];
                    const int sSecond = srcp[x] - srcpn[x];
                    if (((sFirst > d->cthresh && sSecond > d->cthresh) || (sFirst < -d->cthresh && sSecond < -d->cthresh)) &&
                        std::abs(srcppp[x] + 4 * srcp[x] + srcpnn[x] - 3 * (srcpp[x] + srcpn[x])) > d->cthresh6)
                        cmkp[x] = 0xFF;
                }
                srcppp += srcStride;
                srcpp += srcStride;
                srcp += srcStride;
                srcpn += srcStride;
                srcpnn += srcStride;
                cmkp += cmkStride;
            }

            for (int x = 0; x < width; x++) {
                const int sFirst = srcp[x] - srcpp[x];
                const int sSecond = srcp[x] - srcpn[x];
                if (((sFirst > d->cthresh && sSecond > d->cthresh) || (sFirst < -d->cthresh && sSecond < -d->cthresh)) &&
                    std::abs(srcppp[x] + 4 * srcp[x] + srcppp[x] - 3 * (srcpp[x] + srcpn[x])) > d->cthresh6)
                    cmkp[x] = 0xFF;
            }
            srcppp += srcStride;
            srcpp += srcStride;
            srcp += srcStride;
            srcpn += srcStride;
            srcpnn += srcStride;
            cmkp += cmkStride;

            for (int x = 0; x < width; x++) {
                const int sFirst = srcp[x] - srcpp[x];
                if ((sFirst > d->cthresh || sFirst < -d->cthresh) && std::abs(srcppp[x] + 4 * srcp[x] + srcppp[x] - 3 * (srcpp[x] + srcpp[x])) > d->cthresh6)
                    cmkp[x] = 0xFF;
            }
        } else {
            for (int x = 0; x < width; x++) {
                if ((srcp[x] - srcpn[x]) * (srcp[x] - srcpn[x]) > d->cthreshsq)
                    cmkp[x] = 0xFF;
            }
            srcpp += srcStride;
            srcp += srcStride;
            srcpn += srcStride;
            cmkp += cmkStride;

            for (int y = 1; y < height - 1; y++) {
                for (int x = 0; x < width; x++) {
                    if ((srcp[x] - srcpp[x]) * (srcp[x] - srcpn[x]) > d->cthreshsq)
                        cmkp[x] = 0xFF;
                }
                srcpp += srcStride;
                srcp += srcStride;
                srcpn += srcStride;
                cmkp += cmkStride;
            }

            for (int x = 0; x < width; x++) {
                if ((srcp[x] - srcpp[x]) * (srcp[x] - srcpp[x]) > d->cthreshsq)
                    cmkp[x] = 0xFF;
            }
        }
    }

    if (d->chroma) {
        const int width = vsapi->getFrameWidth(cmask, 2);
        const int height = vsapi->getFrameHeight(cmask, 2);
        const ptrdiff_t stride = vsapi->getStride(cmask, 0);
        const ptrdiff_t strideY = stride << d->vi->format.subSamplingH;
        const ptrdiff_t strideUV = vsapi->getStride(cmask, 2);
        uint8_t* VS_RESTRICT cmkp = vsapi->getWritePtr(cmask, 0);
        const uint8_t* cmkpU = vsapi->getReadPtr(cmask, 1);
        const uint8_t* cmkpV = vsapi->getReadPtr(cmask, 2);

        uint8_t* VS_RESTRICT cmkpp = cmkp - stride;
        uint8_t* VS_RESTRICT cmkpn = cmkp + stride;
        uint8_t* VS_RESTRICT cmkpnn = cmkp + stride * 2;
        const uint8_t* cmkppU = cmkpU - strideUV;
        const uint8_t* cmkpnU = cmkpU + strideUV;
        const uint8_t* cmkppV = cmkpV - strideUV;
        const uint8_t* cmkpnV = cmkpV + strideUV;

        for (int y = 1; y < height - 1; y++) {
            cmkpp += strideY;
            cmkp += strideY;
            cmkpn += strideY;
            cmkpnn += strideY;
            cmkppU += strideUV;
            cmkpU += strideUV;
            cmkpnU += strideUV;
            cmkppV += strideUV;
            cmkpV += strideUV;
            cmkpnV += strideUV;

            for (int x = 1; x < width - 1; x++) {
                if ((cmkpU[x] && (cmkpU[x - 1] || cmkpU[x + 1] || cmkppU[x - 1] || cmkppU[x] || cmkppU[x + 1] || cmkpnU[x - 1] || cmkpnU[x] || cmkpnU[x + 1])) ||
                    (cmkpV[x] && (cmkpV[x - 1] || cmkpV[x + 1] || cmkppV[x - 1] || cmkppV[x] || cmkppV[x + 1] || cmkpnV[x - 1] || cmkpnV[x] || cmkpnV[x + 1]))) {
                    if (d->vi->format.subSamplingW == 0) {
                        cmkp[x] = 0xFF;

                        if (d->vi->format.subSamplingH > 0) {
                            cmkpn[x] = 0xFF;
                            (y & 1 ? cmkpp : cmkpnn)[x] = 0xFF;
                        }
                    } else if (d->vi->format.subSamplingW == 1) {
                        reinterpret_cast<uint16_t*>(cmkp)[x] = 0xFFFF;

                        if (d->vi->format.subSamplingH > 0) {
                            reinterpret_cast<uint16_t*>(cmkpn)[x] = 0xFFFF;
                            reinterpret_cast<uint16_t*>(y & 1 ? cmkpp : cmkpnn)[x] = 0xFFFF;
                        }
                    } else {
                        reinterpret_cast<uint32_t*>(cmkp)[x] = 0xFFFFFFFF;

                        if (d->vi->format.subSamplingH > 0) {
                            reinterpret_cast<uint32_t*>(cmkpn)[x] = 0xFFFFFFFF;
                            reinterpret_cast<uint32_t*>(y & 1 ? cmkpp : cmkpnn)[x] = 0xFFFFFFFF;
                        }
                    }
                }
            }
        }
    }

    const int width = vsapi->getFrameWidth(cmask, 0);
    const int height = vsapi->getFrameHeight(cmask, 0);
    const ptrdiff_t stride = vsapi->getStride(cmask, 0);
    const uint8_t* cmkp = vsapi->getReadPtr(cmask, 0) + stride;

    const uint8_t* cmkpp = cmkp - stride;
    const uint8_t* cmkpn = cmkp + stride;

    auto cArray = std::make_unique<int[]>(d->arraySize);

    for (int y = 1; y < d->yHalf; y++) {
        const int temp1 = (y >> d->yShift) * d->xBlocks4;
        const int temp2 = ((y + d->yHalf) >> d->yShift) * d->xBlocks4;

        for (int x = 0; x < width; x++) {
            if (cmkpp[x] && cmkp[x] && cmkpn[x]) {
                const int box1 = (x >> d->xShift) * 4;
                const int box2 = ((x + d->xHalf) >> d->xShift) * 4;
                ++cArray[temp1 + box1 + 0];
                ++cArray[temp1 + box2 + 1];
                ++cArray[temp2 + box1 + 2];
                ++cArray[temp2 + box2 + 3];
            }
        }

        cmkpp += stride;
        cmkp += stride;
        cmkpn += stride;
    }

    for (int y = d->yHalf; y < d->heighta; y += d->yHalf) {
        const int temp1 = (y >> d->yShift) * d->xBlocks4;
        const int temp2 = ((y + d->yHalf) >> d->yShift) * d->xBlocks4;

        for (int x = 0; x < d->widtha; x += d->xHalf) {
            const uint8_t* cmkppT = cmkpp;
            const uint8_t* cmkpT = cmkp;
            const uint8_t* cmkpnT = cmkpn;
            int sum = 0;

            for (int u = 0; u < d->yHalf; u++) {
                for (int v = 0; v < d->xHalf; v++) {
                    if (cmkppT[x + v] && cmkpT[x + v] && cmkpnT[x + v])
                        sum++;
                }
                cmkppT += stride;
                cmkpT += stride;
                cmkpnT += stride;
            }

            if (sum) {
                const int box1 = (x >> d->xShift) * 4;
                const int box2 = ((x + d->xHalf) >> d->xShift) * 4;
                cArray[temp1 + box1 + 0] += sum;
                cArray[temp1 + box2 + 1] += sum;
                cArray[temp2 + box1 + 2] += sum;
                cArray[temp2 + box2 + 3] += sum;
            }
        }

        for (int x = d->widtha; x < width; x++) {
            const uint8_t* cmkppT = cmkpp;
            const uint8_t* cmkpT = cmkp;
            const uint8_t* cmkpnT = cmkpn;
            int sum = 0;

            for (int u = 0; u < d->yHalf; u++) {
                if (cmkppT[x] && cmkpT[x] && cmkpnT[x])
                    sum++;
                cmkppT += stride;
                cmkpT += stride;
                cmkpnT += stride;
            }

            if (sum) {
                const int box1 = (x >> d->xShift) * 4;
                const int box2 = ((x + d->xHalf) >> d->xShift) * 4;
                cArray[temp1 + box1 + 0] += sum;
                cArray[temp1 + box2 + 1] += sum;
                cArray[temp2 + box1 + 2] += sum;
                cArray[temp2 + box2 + 3] += sum;
            }
        }

        cmkpp += stride * d->yHalf;
        cmkp += stride * d->yHalf;
        cmkpn += stride * d->yHalf;
    }

    for (int y = d->heighta; y < height - 1; y++) {
        const int temp1 = (y >> d->yShift) * d->xBlocks4;
        const int temp2 = ((y + d->yHalf) >> d->yShift) * d->xBlocks4;

        for (int x = 0; x < width; x++) {
            if (cmkpp[x] && cmkp[x] && cmkpn[x]) {
                const int box1 = (x >> d->xShift) * 4;
                const int box2 = ((x + d->xHalf) >> d->xShift) * 4;
                ++cArray[temp1 + box1 + 0];
                ++cArray[temp1 + box2 + 1];
                ++cArray[temp2 + box1 + 2];
                ++cArray[temp2 + box2 + 3];
            }
        }

        cmkpp += stride;
        cmkp += stride;
        cmkpn += stride;
    }

    int mic = 0;
    for (int x = 0; x < d->arraySize; x++) {
        if (cArray[x] > mic)
            mic = cArray[x];
    }
    return mic > d->mi;
}

static const VSFrame* VS_CC isCombedGetFrame(int n, int activationReason, void* instanceData, [[maybe_unused]] void** frameData, VSFrameContext* frameCtx,
                                             VSCore* core, const VSAPI* vsapi) {
    auto d = static_cast<const IsCombedData*>(instanceData);

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
    } else if (activationReason == arAllFramesReady) {
        const VSFrame* src = vsapi->getFrameFilter(n, d->node, frameCtx);
        VSFrame* cmask = vsapi->newVideoFrame(&d->format, d->vi->width, d->vi->height, nullptr, core);
        VSFrame* dst = vsapi->copyFrame(src, core);

        vsapi->mapSetInt(vsapi->getFramePropertiesRW(dst), "_Combed", d->filter(src, cmask, d, vsapi), maReplace);

        vsapi->freeFrame(src);
        vsapi->freeFrame(cmask);
        return dst;
    }

    return nullptr;
}

static void VS_CC isCombedFree(void* instanceData, [[maybe_unused]] VSCore* core, const VSAPI* vsapi) {
    auto d = static_cast<IsCombedData*>(instanceData);
    vsapi->freeNode(d->node);
    delete d;
}

static void VS_CC isCombedCreate(const VSMap* in, VSMap* out, [[maybe_unused]] void* userData, VSCore* core, const VSAPI* vsapi) {
    auto d = std::make_unique<IsCombedData>();

    try {
        d->node = vsapi->mapGetNode(in, "clip", 0, nullptr);
        d->vi = vsapi->getVideoInfo(d->node);
        int err;

        if (!vsh::isConstantVideoFormat(d->vi) || d->vi->format.sampleType != stInteger || d->vi->format.bitsPerSample > 16)
            throw "only constant format 8-16 bit integer input supported"s;

        if (d->vi->format.subSamplingW > 2)
            throw "only horizontal chroma subsampling 1x-4x supported"s;

        vsapi->queryVideoFormat(&d->format, d->vi->format.colorFamily, stInteger, 8, d->vi->format.subSamplingW, d->vi->format.subSamplingH, core);

        d->cthresh = vsapi->mapGetIntSaturated(in, "cthresh", 0, &err);
        if (err)
            d->cthresh = 6;

        d->blockx = vsapi->mapGetIntSaturated(in, "blockx", 0, &err);
        if (err)
            d->blockx = 16;

        d->blocky = vsapi->mapGetIntSaturated(in, "blocky", 0, &err);
        if (err)
            d->blocky = 16;

        d->chroma = !!vsapi->mapGetInt(in, "chroma", 0, &err);

        d->mi = vsapi->mapGetIntSaturated(in, "mi", 0, &err);
        if (err)
            d->mi = 64;

        d->metric = vsapi->mapGetIntSaturated(in, "metric", 0, &err);

        if (d->cthresh < 0 || d->cthresh > 255)
            throw "cthresh must be between 0 and 255 (inclusive)"s;

        if (!isPowerOf2(d->blockx) || d->blockx < 4 || d->blockx > 2048)
            throw "illegal blockx size"s;

        if (!isPowerOf2(d->blocky) || d->blocky < 4 || d->blocky > 2048)
            throw "illegal blocky size"s;

        if (d->chroma && d->vi->format.colorFamily == cfGray)
            throw "chroma can not be true for Gray color family"s;

        if (d->mi < 0)
            throw "mi must be greater than or equal to 0"s;

        if (d->metric < 0 || d->metric > 1)
            throw "metric must be 0 or 1"s;

        d->filter = (d->vi->format.bytesPerSample == 1) ? filter<uint8_t> : filter<uint16_t>;

        d->cthresh = d->cthresh << (d->vi->format.bitsPerSample - 8);
        d->cthresh6 = d->cthresh * 6;
        d->cthreshsq = d->cthresh * d->cthresh;

        d->xHalf = d->blockx / 2;
        d->yHalf = d->blocky / 2;
        d->xShift = static_cast<int>(std::log2(d->blockx));
        d->yShift = static_cast<int>(std::log2(d->blocky));

        const int xBlocks = ((d->vi->width + d->xHalf) >> d->xShift) + 1;
        const int yBlocks = ((d->vi->height + d->yHalf) >> d->yShift) + 1;
        d->arraySize = xBlocks * yBlocks * 4;
        d->xBlocks4 = xBlocks * 4;

        d->widtha = (d->vi->width >> (d->xShift - 1)) << (d->xShift - 1);
        d->heighta = (d->vi->height >> (d->yShift - 1)) << (d->yShift - 1);
        if (d->heighta == d->vi->height)
            d->heighta = d->vi->height - d->yHalf;
    } catch (const std::string& error) {
        vsapi->mapSetError(out, ("IsCombed: " + error).c_str());
        vsapi->freeNode(d->node);
        return;
    }

    VSFilterDependency deps[] = { {d->node, rpStrictSpatial} };
    vsapi->createVideoFilter(out, "IsCombed", d->vi, isCombedGetFrame, isCombedFree, fmParallel, deps, 1, d.get(), core);
    d.release();
}

//////////////////////////////////////////
// Init

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin* plugin, const VSPLUGINAPI* vspapi) {
    vspapi->configPlugin("com.holywu.iscombed",
                         "iscombed",
                         "Check whether or not a frame is combed",
                         VS_MAKE_VERSION(1, 0),
                         VAPOURSYNTH_API_VERSION,
                         0,
                         plugin);

    vspapi->registerFunction("IsCombed",
                             "clip:vnode;cthresh:int:opt;blockx:int:opt;blocky:int:opt;chroma:int:opt;mi:int:opt;metric:int:opt;",
                             "clip:vnode;",
                             isCombedCreate,
                             nullptr,
                             plugin);
}
