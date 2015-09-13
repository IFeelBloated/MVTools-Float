// Make a motion compensate temporal denoiser
// Copyright(c)2006 A.G.Balakhnin aka Fizick
// See legal notice in Copying.txt for more information

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

#include "VapourSynth.h"
#include "VSHelper.h"

#include "MVClip.h"
#include "MVDegrains.h"
#include "MVFilter.h"
#include "MVFrame.h"
#include "MVInterface.h"
#include "Overlap.h"


typedef struct {
    VSNodeRef *node;
    const VSVideoInfo *vi;

    VSNodeRef *super;
    VSNodeRef *vectors[12];

    float thSAD[3];
    int YUVplanes;
    float nLimit[3];
    float nSCD1;
    float nSCD2;

    MVClipDicks *mvClips[12];

    MVFilter *bleh;

    int nSuperHPad;
    int nSuperVPad;
    int nSuperPel;
    int nSuperModeYUV;
    int nSuperLevels;

    int dstTempPitch;

    OverlapsFunction OVERS[3];
    DenoiseFunction DEGRAIN[3];
    LimitFunction LimitChanges;
    ToPixelsFunction ToPixels;

    bool process[3];

    int xSubUV;
    int ySubUV;

    int nWidth[3];
    int nHeight[3];
    int nOverlapX[3];
    int nOverlapY[3];
    int nBlkSizeX[3];
    int nBlkSizeY[3];
    int nWidth_B[3];
    int nHeight_B[3];

    OverlapWindows *OverWins[3];
} MVDegrainData;


static void VS_CC mvdegrainInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
    MVDegrainData *d = (MVDegrainData *) * instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}


template <int radius>
static const VSFrameRef *VS_CC mvdegrainGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) {
    MVDegrainData *d = (MVDegrainData *) * instanceData;

    if (activationReason == arInitial) {
		if (radius > 5)
			vsapi->requestFrameFilter(n, d->vectors[Forward6], frameCtx);
		if (radius > 4)
			vsapi->requestFrameFilter(n, d->vectors[Forward5], frameCtx);
		if (radius > 3)
			vsapi->requestFrameFilter(n, d->vectors[Forward4], frameCtx);
        if (radius > 2)
            vsapi->requestFrameFilter(n, d->vectors[Forward3], frameCtx);
        if (radius > 1)
            vsapi->requestFrameFilter(n, d->vectors[Forward2], frameCtx);
        vsapi->requestFrameFilter(n, d->vectors[Forward1], frameCtx);
        vsapi->requestFrameFilter(n, d->vectors[Backward1], frameCtx);
        if (radius > 1)
            vsapi->requestFrameFilter(n, d->vectors[Backward2], frameCtx);
        if (radius > 2)
            vsapi->requestFrameFilter(n, d->vectors[Backward3], frameCtx);
		if (radius > 3)
			vsapi->requestFrameFilter(n, d->vectors[Backward4], frameCtx);
		if (radius > 4)
			vsapi->requestFrameFilter(n, d->vectors[Backward5], frameCtx);
		if (radius > 5)
			vsapi->requestFrameFilter(n, d->vectors[Backward6], frameCtx);

		if (radius > 5) {
			int offF6 = -1 * d->mvClips[Forward6]->GetDeltaFrame();
			if (n + offF6 >= 0)
				vsapi->requestFrameFilter(n + offF6, d->super, frameCtx);
		}

		if (radius > 4) {
			int offF5 = -1 * d->mvClips[Forward5]->GetDeltaFrame();
			if (n + offF5 >= 0)
				vsapi->requestFrameFilter(n + offF5, d->super, frameCtx);
		}

		if (radius > 3) {
			int offF4 = -1 * d->mvClips[Forward4]->GetDeltaFrame();
			if (n + offF4 >= 0)
				vsapi->requestFrameFilter(n + offF4, d->super, frameCtx);
		}

        if (radius > 2) {
            int offF3 = -1 * d->mvClips[Forward3]->GetDeltaFrame();
            if (n + offF3 >= 0)
                vsapi->requestFrameFilter(n + offF3, d->super, frameCtx);
        }

        if (radius > 1) {
            int offF2 = -1 * d->mvClips[Forward2]->GetDeltaFrame();
            if (n + offF2 >= 0)
                vsapi->requestFrameFilter(n + offF2, d->super, frameCtx);
        }

        int offF = -1 * d->mvClips[Forward1]->GetDeltaFrame();
        if (n + offF >= 0)
            vsapi->requestFrameFilter(n + offF, d->super, frameCtx);

        int offB = d->mvClips[Backward1]->GetDeltaFrame();
        if (n + offB < d->vi->numFrames || !d->vi->numFrames)
            vsapi->requestFrameFilter(n + offB, d->super, frameCtx);

        if (radius > 1) {
            int offB2 = d->mvClips[Backward2]->GetDeltaFrame();
            if (n + offB2 < d->vi->numFrames || !d->vi->numFrames)
                vsapi->requestFrameFilter(n + offB2, d->super, frameCtx);
        }

        if (radius > 2) {
            int offB3 = d->mvClips[Backward3]->GetDeltaFrame();
            if (n + offB3 < d->vi->numFrames || !d->vi->numFrames)
                vsapi->requestFrameFilter(n + offB3, d->super, frameCtx);
        }

		if (radius > 3) {
			int offB4 = d->mvClips[Backward4]->GetDeltaFrame();
			if (n + offB4 < d->vi->numFrames || !d->vi->numFrames)
				vsapi->requestFrameFilter(n + offB4, d->super, frameCtx);
		}

		if (radius > 4) {
			int offB5 = d->mvClips[Backward5]->GetDeltaFrame();
			if (n + offB5 < d->vi->numFrames || !d->vi->numFrames)
				vsapi->requestFrameFilter(n + offB5, d->super, frameCtx);
		}

		if (radius > 5) {
			int offB6 = d->mvClips[Backward6]->GetDeltaFrame();
			if (n + offB6 < d->vi->numFrames || !d->vi->numFrames)
				vsapi->requestFrameFilter(n + offB6, d->super, frameCtx);
		}

        vsapi->requestFrameFilter(n, d->node, frameCtx);
    } else if (activationReason == arAllFramesReady) {
        const VSFrameRef *src = vsapi->getFrameFilter(n, d->node, frameCtx);
        VSFrameRef *dst = vsapi->newVideoFrame(d->vi->format, d->vi->width, d->vi->height, src, core);

        uint8_t *pDst[3], *pDstCur[3];
        const uint8_t *pSrcCur[3];
        const uint8_t *pSrc[3];
        const uint8_t *pRefs[3][radius*2];
        int nDstPitches[3], nSrcPitches[3];
        int nRefPitches[3][radius*2];
        bool isUsable[radius*2];
        int nLogPel = (d->bleh->nPel == 4) ? 2 : (d->bleh->nPel == 2) ? 1 : 0;

        MVClipBalls *balls[radius*2];
        const VSFrameRef *refFrames[radius*2] = { 0 };

        for (int r = 0; r < radius*2; r++) {
            const VSFrameRef *frame = vsapi->getFrameFilter(n, d->vectors[r], frameCtx);
            balls[r] = new MVClipBalls(d->mvClips[r], vsapi);
            balls[r]->Update(frame);
            isUsable[r] = balls[r]->IsUsable();
            vsapi->freeFrame(frame);

            if (isUsable[r]) {
                int offset = d->mvClips[r]->GetDeltaFrame() * (d->mvClips[r]->IsBackward() ? 1 : -1);
                refFrames[r] = vsapi->getFrameFilter(n + offset, d->super, frameCtx);
            }
        }


        for (int i = 0; i < d->vi->format->numPlanes; i++) {
            pDst[i] = vsapi->getWritePtr(dst, i);
            nDstPitches[i] = vsapi->getStride(dst, i);
            pSrc[i] = vsapi->getReadPtr(src, i);
            nSrcPitches[i] = vsapi->getStride(src, i);

            for (int r = 0; r < radius*2; r++)
                if (isUsable[r]) {
                    pRefs[i][r] = vsapi->getReadPtr(refFrames[r], i);
                    nRefPitches[i][r] = vsapi->getStride(refFrames[r], i);
                }
        }

        const int xSubUV = d->xSubUV;
        const int ySubUV = d->ySubUV;
        const int xRatioUV = d->bleh->xRatioUV;
        const int yRatioUV = d->bleh->yRatioUV;
        const int nBlkX = d->bleh->nBlkX;
        const int nBlkY = d->bleh->nBlkY;
        const int YUVplanes = d->YUVplanes;
        const int dstTempPitch = d->dstTempPitch;
        const int *nWidth = d->nWidth;
        const int *nHeight = d->nHeight;
        const int *nOverlapX = d->nOverlapX;
        const int *nOverlapY = d->nOverlapY;
        const int *nBlkSizeX = d->nBlkSizeX;
        const int *nBlkSizeY = d->nBlkSizeY;
        const int *nWidth_B = d->nWidth_B;
        const int *nHeight_B = d->nHeight_B;
        const float *thSAD = d->thSAD;
        const float *nLimit = d->nLimit;


        MVGroupOfFrames *pRefGOF[radius*2];
        for (int r = 0; r < radius*2; r++)
            pRefGOF[r] = new MVGroupOfFrames(d->nSuperLevels, nWidth[0], nHeight[0], d->nSuperPel, d->nSuperHPad, d->nSuperVPad, d->nSuperModeYUV, xRatioUV, yRatioUV);


        OverlapWindows *OverWins[3] = { d->OverWins[0], d->OverWins[1], d->OverWins[2] };
        uint8_t *DstTemp = NULL;
        int tmpBlockPitch = nBlkSizeX[0] * 4;
        uint8_t *tmpBlock = NULL;
        if (nOverlapX[0] > 0 || nOverlapY[0] > 0) {
            DstTemp = new uint8_t[dstTempPitch * nHeight[0]];
            tmpBlock = new uint8_t[tmpBlockPitch * nBlkSizeY[0]];
        }

        MVPlane *pPlanes[3][radius*2] = { };

        MVPlaneSet planes[3] = { YPLANE, UPLANE, VPLANE };

        for (int r = 0; r < radius*2; r++)
            if (isUsable[r]) {
                pRefGOF[r]->Update(YUVplanes, (uint8_t*)pRefs[0][r], nRefPitches[0][r], (uint8_t*)pRefs[1][r], nRefPitches[1][r], (uint8_t*)pRefs[2][r], nRefPitches[2][r]);
                for (int plane = 0; plane < d->vi->format->numPlanes; plane++)
                    if (YUVplanes & planes[plane])
                        pPlanes[plane][r] = pRefGOF[r]->GetFrame(0)->GetPlane(planes[plane]);
            }


        pDstCur[0] = pDst[0];
        pDstCur[1] = pDst[1];
        pDstCur[2] = pDst[2];
        pSrcCur[0] = pSrc[0];
        pSrcCur[1] = pSrc[1];
        pSrcCur[2] = pSrc[2];
        // -----------------------------------------------------------------------------

        for (int plane = 0; plane < d->vi->format->numPlanes; plane++) {
            if (!d->process[plane]) {
                memcpy(pDstCur[plane], pSrcCur[plane], nSrcPitches[plane] * nHeight[plane]);
                continue;
            }

            if (nOverlapX[0] == 0 && nOverlapY[0] == 0) {
                for (int by = 0; by < nBlkY; by++) {
                    int xx = 0;
                    for (int bx = 0; bx < nBlkX; bx++) {
                        int i = by * nBlkX + bx;

                        const uint8_t *pointers[radius*2]; // Moved by the degrain function.
                        int strides[radius*2];

                        float WSrc, WRefs[radius*2];

                        for (int r = 0; r < radius*2; r++)
                            useBlock(pointers[r], strides[r], WRefs[r], isUsable[r], balls[r], i, pPlanes[plane][r], pSrcCur, xx, nSrcPitches, nLogPel, plane, xSubUV, ySubUV, thSAD);

                        normalizeWeights<radius>(WSrc, WRefs);

                        d->DEGRAIN[plane](pDstCur[plane] + xx, nDstPitches[plane], pSrcCur[plane] + xx, nSrcPitches[plane],
                                pointers, strides,
                                WSrc, WRefs);

                        xx += nBlkSizeX[plane] * 4;

                        if (bx == nBlkX - 1 && nWidth_B[0] < nWidth[0]) // right non-covered region
                            vs_bitblt(pDstCur[plane] + nWidth_B[plane] * 4, nDstPitches[plane],
                                    pSrcCur[plane] + nWidth_B[plane] * 4, nSrcPitches[plane],
                                    (nWidth[plane] - nWidth_B[plane]) * 4, nBlkSizeY[plane]);
                    }
                    pDstCur[plane] += nBlkSizeY[plane] * (nDstPitches[plane]);
                    pSrcCur[plane] += nBlkSizeY[plane] * (nSrcPitches[plane]);

                    if (by == nBlkY - 1 && nHeight_B[0] < nHeight[0]) // bottom uncovered region
                        vs_bitblt(pDstCur[plane], nDstPitches[plane],
                                pSrcCur[plane], nSrcPitches[plane],
                                nWidth[plane] * 4, nHeight[plane] - nHeight_B[plane]);
                }
            } else {// overlap
                uint8_t *pDstTemp = DstTemp;
                memset(pDstTemp, 0, dstTempPitch * nHeight_B[0]);

                for (int by = 0; by < nBlkY; by++) {
                    int wby = ((by + nBlkY - 3) / (nBlkY - 2)) * 3;
                    int xx = 0;
                    for (int bx = 0; bx < nBlkX; bx++) {
                        // select window
                        int wbx = (bx + nBlkX - 3) / (nBlkX - 2);
                        short *winOver = OverWins[plane]->GetWindow(wby + wbx);

                        int i = by * nBlkX + bx;

                        const uint8_t *pointers[radius*2]; // Moved by the degrain function.
                        int strides[radius*2];

                        float WSrc, WRefs[radius*2];

                        for (int r = 0; r < radius*2; r++)
                            useBlock(pointers[r], strides[r], WRefs[r], isUsable[r], balls[r], i, pPlanes[plane][r], pSrcCur, xx, nSrcPitches, nLogPel, plane, xSubUV, ySubUV, thSAD);

                        normalizeWeights<radius>(WSrc, WRefs);

                        d->DEGRAIN[plane](tmpBlock, tmpBlockPitch, pSrcCur[plane] + xx, nSrcPitches[plane],
                                pointers, strides,
                                WSrc, WRefs);
                        d->OVERS[plane](pDstTemp + xx*2, dstTempPitch, tmpBlock, tmpBlockPitch, winOver, nBlkSizeX[plane]);

                        xx += (nBlkSizeX[plane] - nOverlapX[plane]) * 4;

                    }
                    pSrcCur[plane] += (nBlkSizeY[plane] - nOverlapY[plane]) * nSrcPitches[plane];
                    pDstTemp += (nBlkSizeY[plane] - nOverlapY[plane]) * dstTempPitch;
                }

                d->ToPixels(pDst[plane], nDstPitches[plane], DstTemp, dstTempPitch, nWidth_B[plane], nHeight_B[plane]);

                if (nWidth_B[0] < nWidth[0])
                    vs_bitblt(pDst[plane] + nWidth_B[plane] * 4, nDstPitches[plane],
                            pSrc[plane] + nWidth_B[plane] * 4, nSrcPitches[plane],
                            (nWidth[plane] - nWidth_B[plane]) * 4, nHeight_B[plane]);

                if (nHeight_B[0] < nHeight[0]) // bottom noncovered region
                    vs_bitblt(pDst[plane] + nDstPitches[plane] * nHeight_B[plane], nDstPitches[plane],
                            pSrc[plane] + nSrcPitches[plane] * nHeight_B[plane], nSrcPitches[plane],
                            nWidth[plane] * 4, nHeight[plane] - nHeight_B[plane]);
            }

            if (nLimit[plane] < 1.f)
                d->LimitChanges(pDst[plane], nDstPitches[plane],
                                pSrc[plane], nSrcPitches[plane],
                                nWidth[plane], nHeight[plane], nLimit[plane]);
        }


        if (tmpBlock)
            delete[] tmpBlock;

        if (DstTemp)
            delete[] DstTemp;

        for (int r = 0; r < radius*2; r++) {
            delete pRefGOF[r];

            if (refFrames[r])
                vsapi->freeFrame(refFrames[r]);

            delete balls[r];
        }

        vsapi->freeFrame(src);

        return dst;
    }

    return 0;
}


template <int radius>
static void VS_CC mvdegrainFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
    MVDegrainData *d = (MVDegrainData *)instanceData;

    if (d->nOverlapX[0] || d->nOverlapY[0]) {
        delete d->OverWins[0];
        if (d->vi->format->colorFamily != cmGray)
            delete d->OverWins[1];
    }
    for (int r = 0; r < radius*2; r++) {
        delete d->mvClips[r];
        vsapi->freeNode(d->vectors[r]);
    }
    vsapi->freeNode(d->super);
    vsapi->freeNode(d->node);

    delete d->bleh;

    free(d);
}


template <int radius>
static void selectFunctions(MVDegrainData *d) {
    const int xRatioUV = d->bleh->xRatioUV;
    const int yRatioUV = d->bleh->yRatioUV;
    const int nBlkSizeX = d->bleh->nBlkSizeX;
    const int nBlkSizeY = d->bleh->nBlkSizeY;

    OverlapsFunction overs[33][33];
    DenoiseFunction degs[33][33];

        overs[2][2] = Overlaps_C<2,2, double, float>;
        degs[2][2] = Degrain_C<radius, 2,2, float>;

        overs[2][4] = Overlaps_C<2,4, double, float>;
        degs[2][4] = Degrain_C<radius, 2,4, float>;

        overs[4][2] = Overlaps_C<4,2, double, float>;
        degs[4][2] = Degrain_C<radius, 4,2, float>;

        overs[4][4] = Overlaps_C<4,4, double, float>;
        degs[4][4] = Degrain_C<radius, 4,4, float>;

        overs[4][8] = Overlaps_C<4,8, double, float>;
        degs[4][8] = Degrain_C<radius, 4,8, float>;

        overs[8][1] = Overlaps_C<8,1, double, float>;
        degs[8][1] = Degrain_C<radius, 8,1, float>;

        overs[8][2] = Overlaps_C<8,2, double, float>;
        degs[8][2] = Degrain_C<radius, 8,2, float>;

        overs[8][4] = Overlaps_C<8,4, double, float>;
        degs[8][4] = Degrain_C<radius, 8,4, float>;

        overs[8][8] = Overlaps_C<8,8, double, float>;
        degs[8][8] = Degrain_C<radius, 8,8, float>;

        overs[8][16] = Overlaps_C<8,16, double, float>;
        degs[8][16] = Degrain_C<radius, 8,16, float>;

        overs[16][1] = Overlaps_C<16,1, double, float>;
        degs[16][1] = Degrain_C<radius, 16,1, float>;

        overs[16][2] = Overlaps_C<16,2, double, float>;
        degs[16][2] = Degrain_C<radius, 16,2, float>;

        overs[16][4] = Overlaps_C<16,4, double, float>;
        degs[16][4] = Degrain_C<radius, 16,4, float>;

        overs[16][8] = Overlaps_C<16,8, double, float>;
        degs[16][8] = Degrain_C<radius, 16,8, float>;

        overs[16][16] = Overlaps_C<16,16, double, float>;
        degs[16][16] = Degrain_C<radius, 16,16, float>;

        overs[16][32] = Overlaps_C<16,32, double, float>;
        degs[16][32] = Degrain_C<radius, 16,32, float>;

        overs[32][8] = Overlaps_C<32,8, double, float>;
        degs[32][8] = Degrain_C<radius, 32,8, float>;

        overs[32][16] = Overlaps_C<32,16, double, float>;
        degs[32][16] = Degrain_C<radius, 32,16, float>;

        overs[32][32] = Overlaps_C<32,32, double, float>;
        degs[32][32] = Degrain_C<radius, 32,32, float>;

        d->LimitChanges = LimitChanges_C<float>;

        d->ToPixels = ToPixels<double, float>;
    

    d->OVERS[0] = overs[nBlkSizeX][nBlkSizeY];
    d->DEGRAIN[0] = degs[nBlkSizeX][nBlkSizeY];

    d->OVERS[1] = d->OVERS[2] = overs[nBlkSizeX / xRatioUV][nBlkSizeY / yRatioUV];
    d->DEGRAIN[1] = d->DEGRAIN[2] = degs[nBlkSizeX / xRatioUV][nBlkSizeY / yRatioUV];
}


template <int radius>
static void VS_CC mvdegrainCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
    std::string filter = "Degrain";
    filter.append(std::to_string(radius));

    MVDegrainData d;
    MVDegrainData *data;

    int err;

    d.thSAD[0] = static_cast<float>(vsapi->propGetFloat(in, "thsad", 0, &err));
    if (err)
        d.thSAD[0] = 400.f;

    d.thSAD[1] = d.thSAD[2] = static_cast<float>(vsapi->propGetFloat(in, "thsadc", 0, &err));
    if (err)
        d.thSAD[1] = d.thSAD[2] = d.thSAD[0];

    int plane = int64ToIntS(vsapi->propGetInt(in, "plane", 0, &err));
    if (err)
        plane = 4;

    d.nSCD1 = static_cast<float>(vsapi->propGetFloat(in, "thscd1", 0, &err));
    if (err)
        d.nSCD1 = MV_DEFAULT_SCD1;

    d.nSCD2 = static_cast<float>(vsapi->propGetFloat(in, "thscd2", 0, &err));
    if (err)
        d.nSCD2 = MV_DEFAULT_SCD2;

    if (plane < 0 || plane > 4) {
        vsapi->setError(out, (filter + ": plane must be between 0 and 4 (inclusive).").c_str());
        return;
    }

    int planes[5] = { YPLANE, UPLANE, VPLANE, UVPLANES, YUVPLANES };
    d.YUVplanes = planes[plane];


    d.super = vsapi->propGetNode(in, "super", 0, NULL);

    char errorMsg[1024];
    const VSFrameRef *evil = vsapi->getFrame(0, d.super, errorMsg, 1024);
    if (!evil) {
        vsapi->setError(out, (filter + ": failed to retrieve first frame from super clip. Error message: " + errorMsg).c_str());
        vsapi->freeNode(d.super);
        return;
    }
    const VSMap *props = vsapi->getFramePropsRO(evil);
    int evil_err[6];
    int nHeightS = int64ToIntS(vsapi->propGetInt(props, "Super_height", 0, &evil_err[0]));
    d.nSuperHPad = int64ToIntS(vsapi->propGetInt(props, "Super_hpad", 0, &evil_err[1]));
    d.nSuperVPad = int64ToIntS(vsapi->propGetInt(props, "Super_vpad", 0, &evil_err[2]));
    d.nSuperPel = int64ToIntS(vsapi->propGetInt(props, "Super_pel", 0, &evil_err[3]));
    d.nSuperModeYUV = int64ToIntS(vsapi->propGetInt(props, "Super_modeyuv", 0, &evil_err[4]));
    d.nSuperLevels = int64ToIntS(vsapi->propGetInt(props, "Super_levels", 0, &evil_err[5]));
    vsapi->freeFrame(evil);

    for (int i = 0; i < 6; i++)
        if (evil_err[i]) {
            vsapi->setError(out, (filter + ": required properties not found in first frame of super clip. Maybe clip didn't come from mv.Super? Was the first frame trimmed away?").c_str());
            vsapi->freeNode(d.super);
            return;
        }


    d.vectors[Backward1] = vsapi->propGetNode(in, "mvbw", 0, NULL);
    d.vectors[Forward1] = vsapi->propGetNode(in, "mvfw", 0, NULL);
    d.vectors[Backward2] = vsapi->propGetNode(in, "mvbw2", 0, &err);
    d.vectors[Forward2] = vsapi->propGetNode(in, "mvfw2", 0, &err);
    d.vectors[Backward3] = vsapi->propGetNode(in, "mvbw3", 0, &err);
    d.vectors[Forward3] = vsapi->propGetNode(in, "mvfw3", 0, &err);
	d.vectors[Backward4] = vsapi->propGetNode(in, "mvbw4", 0, &err);
	d.vectors[Forward4] = vsapi->propGetNode(in, "mvfw4", 0, &err);
	d.vectors[Backward5] = vsapi->propGetNode(in, "mvbw5", 0, &err);
	d.vectors[Forward5] = vsapi->propGetNode(in, "mvfw5", 0, &err);
	d.vectors[Backward6] = vsapi->propGetNode(in, "mvbw6", 0, &err);
	d.vectors[Forward6] = vsapi->propGetNode(in, "mvfw6", 0, &err);

    // XXX Yoda had the right idea.

    // Loops are nice.
    for (int r = 0; r < radius*2; r++) {
        try {
            d.mvClips[r] = new MVClipDicks(d.vectors[r], d.nSCD1, d.nSCD2, vsapi);
        } catch (MVException &e) {
            vsapi->setError(out, (filter + ": " + e.what()).c_str());

            vsapi->freeNode(d.super);

            for (int rr = 0; rr < radius*2; rr++)
                vsapi->freeNode(d.vectors[rr]);

            for (int rr = 0; rr < r; rr++)
                delete d.mvClips[rr];

            return;
        }
    }


    try {
        // Make sure the motion vector clips are correct.
        if (!d.mvClips[Backward1]->IsBackward())
            throw MVException("mvbw must be generated with isb=True.");
        if (d.mvClips[Forward1]->IsBackward())
            throw MVException("mvfw must be generated with isb=False.");
        if (radius > 1) {
            if (!d.mvClips[Backward2]->IsBackward())
                throw MVException("mvbw2 must be generated with isb=True.");
            if (d.mvClips[Forward2]->IsBackward())
                throw MVException("mvfw2 must be generated with isb=False.");

            if (d.mvClips[Backward2]->GetDeltaFrame() <= d.mvClips[Backward1]->GetDeltaFrame())
                throw MVException("mvbw2 must have greater delta than mvbw.");
            if (d.mvClips[Forward2]->GetDeltaFrame() <= d.mvClips[Forward1]->GetDeltaFrame())
                throw MVException("mvfw2 must have greater delta than mvfw.");
        }
        if (radius > 2) {
            if (!d.mvClips[Backward3]->IsBackward())
                throw MVException("mvbw3 must be generated with isb=True.");
            if (d.mvClips[Forward3]->IsBackward())
                throw MVException("mvfw3 must be generated with isb=False.");

            if (d.mvClips[Backward3]->GetDeltaFrame() <= d.mvClips[Backward2]->GetDeltaFrame())
                throw MVException("mvbw3 must have greater delta than mvbw2.");
            if (d.mvClips[Forward3]->GetDeltaFrame() <= d.mvClips[Forward2]->GetDeltaFrame())
                throw MVException("mvfw3 must have greater delta than mvfw2.");
        }

		if (radius > 3) {
			if (!d.mvClips[Backward4]->IsBackward())
				throw MVException("mvbw4 must be generated with isb=True.");
			if (d.mvClips[Forward4]->IsBackward())
				throw MVException("mvfw4 must be generated with isb=False.");

			if (d.mvClips[Backward4]->GetDeltaFrame() <= d.mvClips[Backward3]->GetDeltaFrame())
				throw MVException("mvbw4 must have greater delta than mvbw3.");
			if (d.mvClips[Forward4]->GetDeltaFrame() <= d.mvClips[Forward3]->GetDeltaFrame())
				throw MVException("mvfw4 must have greater delta than mvfw3.");
		}

		if (radius > 4) {
			if (!d.mvClips[Backward5]->IsBackward())
				throw MVException("mvbw5 must be generated with isb=True.");
			if (d.mvClips[Forward5]->IsBackward())
				throw MVException("mvfw5 must be generated with isb=False.");

			if (d.mvClips[Backward5]->GetDeltaFrame() <= d.mvClips[Backward4]->GetDeltaFrame())
				throw MVException("mvbw5 must have greater delta than mvbw4.");
			if (d.mvClips[Forward5]->GetDeltaFrame() <= d.mvClips[Forward4]->GetDeltaFrame())
				throw MVException("mvfw5 must have greater delta than mvfw4.");
		}

		if (radius > 5) {
			if (!d.mvClips[Backward6]->IsBackward())
				throw MVException("mvbw6 must be generated with isb=True.");
			if (d.mvClips[Forward6]->IsBackward())
				throw MVException("mvfw6 must be generated with isb=False.");

			if (d.mvClips[Backward6]->GetDeltaFrame() <= d.mvClips[Backward5]->GetDeltaFrame())
				throw MVException("mvbw6 must have greater delta than mvbw5.");
			if (d.mvClips[Forward6]->GetDeltaFrame() <= d.mvClips[Forward5]->GetDeltaFrame())
				throw MVException("mvfw6 must have greater delta than mvfw5.");
		}

        d.bleh = new MVFilter(d.vectors[Forward1], filter.c_str(), vsapi);
    } catch (MVException &e) {
        vsapi->setError(out, (filter + ": " + e.what()).c_str());
        vsapi->freeNode(d.super);

        for (int r = 0; r < radius*2; r++) {
            vsapi->freeNode(d.vectors[r]);
            delete d.mvClips[r];
        }
        return;
    }


    try {
		const char *vectorNames[12] = { "mvbw", "mvfw", "mvbw2", "mvfw2", "mvbw3", "mvfw3", "mvbw4", "mvfw4", "mvbw5", "mvfw5", "mvbw6", "mvfw6" };

        for (int r = 0; r < radius*2; r++)
            d.bleh->CheckSimilarity(d.mvClips[r], vectorNames[r]);
    } catch (MVException &e) {
        vsapi->setError(out, (filter + ": " + e.what()).c_str());

        vsapi->freeNode(d.super);

        for (int r = 0; r < radius*2; r++) {
            vsapi->freeNode(d.vectors[r]);
            delete d.mvClips[r];
        }

        delete d.bleh;
        return;
    }

    d.thSAD[0] = d.thSAD[0] * d.mvClips[Backward1]->GetThSCD1() / d.nSCD1; // normalize to block SAD
    d.thSAD[1] = d.thSAD[2] = d.thSAD[1] * d.mvClips[Backward1]->GetThSCD1() / d.nSCD1; // chroma threshold, normalized to block SAD



    d.node = vsapi->propGetNode(in, "clip", 0, 0);
    d.vi = vsapi->getVideoInfo(d.node);

    const VSVideoInfo *supervi = vsapi->getVideoInfo(d.super);
    int nSuperWidth = supervi->width;

    if (d.bleh->nHeight != nHeightS || d.bleh->nHeight != d.vi->height || d.bleh->nWidth != nSuperWidth - d.nSuperHPad * 2 || d.bleh->nWidth != d.vi->width) {
        vsapi->setError(out, (filter + ": wrong source or super clip frame size.").c_str());
        vsapi->freeNode(d.super);
        vsapi->freeNode(d.node);

        for (int r = 0; r < radius*2; r++) {
            vsapi->freeNode(d.vectors[r]);
            delete d.mvClips[r];
        }

        delete d.bleh;
        return;
    }


    float pixelMax = 1.f;

    d.nLimit[0] = static_cast<float>(vsapi->propGetFloat(in, "limit", 0, &err));
    if (err)
        d.nLimit[0] = pixelMax;

    d.nLimit[1] = d.nLimit[2] = static_cast<float>(vsapi->propGetFloat(in, "limitc", 0, &err));
    if (err)
        d.nLimit[1] = d.nLimit[2] = d.nLimit[0];

    if (d.nLimit[0] < 0 || d.nLimit[0] > pixelMax) {
        vsapi->setError(out, (filter + ": limit must be between 0 and " + std::to_string(pixelMax) + " (inclusive).").c_str());

        vsapi->freeNode(d.super);
        vsapi->freeNode(d.node);

        for (int r = 0; r < radius*2; r++) {
            vsapi->freeNode(d.vectors[r]);
            delete d.mvClips[r];
        }

        delete d.bleh;

        return;
    }

    if (d.nLimit[1] < 0 || d.nLimit[1] > pixelMax) {
        vsapi->setError(out, (filter + ": limitc must be between 0 and " + std::to_string(pixelMax) + " (inclusive).").c_str());

        vsapi->freeNode(d.super);
        vsapi->freeNode(d.node);

        for (int r = 0; r < radius*2; r++) {
            vsapi->freeNode(d.vectors[r]);
            delete d.mvClips[r];
        }

        delete d.bleh;

        return;
    }


    d.dstTempPitch = ((d.bleh->nWidth + 15)/16)*16 * 4 * 2;

    d.process[0] = !!(d.YUVplanes & YPLANE);
    d.process[1] = !!(d.YUVplanes & UPLANE & d.nSuperModeYUV);
    d.process[2] = !!(d.YUVplanes & VPLANE & d.nSuperModeYUV);

    d.xSubUV = d.vi->format->subSamplingW;
    d.ySubUV = d.vi->format->subSamplingH;

    d.nWidth[0] = d.bleh->nWidth;
    d.nWidth[1] = d.nWidth[2] = d.nWidth[0] >> d.xSubUV;

    d.nHeight[0] = d.bleh->nHeight;
    d.nHeight[1] = d.nHeight[2] = d.nHeight[0] >> d.ySubUV;

    d.nOverlapX[0] = d.bleh->nOverlapX;
    d.nOverlapX[1] = d.nOverlapX[2] = d.nOverlapX[0] >> d.xSubUV;

    d.nOverlapY[0] = d.bleh->nOverlapY;
    d.nOverlapY[1] = d.nOverlapY[2] = d.nOverlapY[0] >> d.ySubUV;

    d.nBlkSizeX[0] = d.bleh->nBlkSizeX;
    d.nBlkSizeX[1] = d.nBlkSizeX[2] = d.nBlkSizeX[0] >> d.xSubUV;

    d.nBlkSizeY[0] = d.bleh->nBlkSizeY;
    d.nBlkSizeY[1] = d.nBlkSizeY[2] = d.nBlkSizeY[0] >> d.ySubUV;

    d.nWidth_B[0] = d.bleh->nBlkX * (d.nBlkSizeX[0] - d.nOverlapX[0]) + d.nOverlapX[0];
    d.nWidth_B[1] = d.nWidth_B[2] = d.nWidth_B[0] >> d.xSubUV;

    d.nHeight_B[0] = d.bleh->nBlkY * (d.nBlkSizeY[0] - d.nOverlapY[0]) + d.nOverlapY[0];
    d.nHeight_B[1] = d.nHeight_B[2] = d.nHeight_B[0] >> d.ySubUV;

    if (d.nOverlapX[0] || d.nOverlapY[0]) {
        d.OverWins[0] = new OverlapWindows(d.nBlkSizeX[0], d.nBlkSizeY[0], d.nOverlapX[0], d.nOverlapY[0]);
        if (d.vi->format->colorFamily != cmGray) {
            d.OverWins[1] = new OverlapWindows(d.nBlkSizeX[1], d.nBlkSizeY[1], d.nOverlapX[1], d.nOverlapY[1]);
            d.OverWins[2] = d.OverWins[1];
        }
    }

    selectFunctions<radius>(&d);


    data = (MVDegrainData *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in, out, filter.c_str(), mvdegrainInit, mvdegrainGetFrame<radius>, mvdegrainFree<radius>, fmParallel, 0, data, core);
}


void mvdegrainsRegister(VSRegisterFunction registerFunc, VSPlugin *plugin) {
    registerFunc("Degrain1",
            "clip:clip;"
            "super:clip;"
            "mvbw:clip;"
            "mvfw:clip;"
            "thsad:float:opt;"
            "thsadc:float:opt;"
            "plane:int:opt;"
            "limit:float:opt;"
            "limitc:float:opt;"
            "thscd1:float:opt;"
            "thscd2:float:opt;"
            , mvdegrainCreate<1>, 0, plugin);
    registerFunc("Degrain2",
            "clip:clip;"
            "super:clip;"
            "mvbw:clip;"
            "mvfw:clip;"
            "mvbw2:clip;"
            "mvfw2:clip;"
            "thsad:float:opt;"
            "thsadc:float:opt;"
            "plane:int:opt;"
            "limit:float:opt;"
            "limitc:float:opt;"
            "thscd1:float:opt;"
            "thscd2:float:opt;"
            , mvdegrainCreate<2>, 0, plugin);
    registerFunc("Degrain3",
            "clip:clip;"
            "super:clip;"
            "mvbw:clip;"
            "mvfw:clip;"
            "mvbw2:clip;"
            "mvfw2:clip;"
            "mvbw3:clip;"
            "mvfw3:clip;"
            "thsad:float:opt;"
            "thsadc:float:opt;"
            "plane:int:opt;"
            "limit:float:opt;"
            "limitc:float:opt;"
            "thscd1:float:opt;"
            "thscd2:float:opt;"
            , mvdegrainCreate<3>, 0, plugin);
	registerFunc("Degrain4",
		"clip:clip;"
		"super:clip;"
		"mvbw:clip;"
		"mvfw:clip;"
		"mvbw2:clip;"
		"mvfw2:clip;"
		"mvbw3:clip;"
		"mvfw3:clip;"
		"mvbw4:clip;"
		"mvfw4:clip;"
		"thsad:float:opt;"
		"thsadc:float:opt;"
		"plane:int:opt;"
		"limit:float:opt;"
		"limitc:float:opt;"
		"thscd1:float:opt;"
		"thscd2:float:opt;"
		, mvdegrainCreate<4>, 0, plugin);
	registerFunc("Degrain5",
		"clip:clip;"
		"super:clip;"
		"mvbw:clip;"
		"mvfw:clip;"
		"mvbw2:clip;"
		"mvfw2:clip;"
		"mvbw3:clip;"
		"mvfw3:clip;"
		"mvbw4:clip;"
		"mvfw4:clip;"
		"mvbw5:clip;"
		"mvfw5:clip;"
		"thsad:float:opt;"
		"thsadc:float:opt;"
		"plane:int:opt;"
		"limit:float:opt;"
		"limitc:float:opt;"
		"thscd1:float:opt;"
		"thscd2:float:opt;"
		, mvdegrainCreate<5>, 0, plugin);
	registerFunc("Degrain6",
		"clip:clip;"
		"super:clip;"
		"mvbw:clip;"
		"mvfw:clip;"
		"mvbw2:clip;"
		"mvfw2:clip;"
		"mvbw3:clip;"
		"mvfw3:clip;"
		"mvbw4:clip;"
		"mvfw4:clip;"
		"mvbw5:clip;"
		"mvfw5:clip;"
		"mvbw6:clip;"
		"mvfw6:clip;"
		"thsad:float:opt;"
		"thsadc:float:opt;"
		"plane:int:opt;"
		"limit:float:opt;"
		"limitc:float:opt;"
		"thscd1:float:opt;"
		"thscd2:float:opt;"
		, mvdegrainCreate<6>, 0, plugin);
}
