#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "MVClip.hpp"
#include "MVFrame.h"
#include "SADFunctions.hpp"

auto CheckAndPadMaskSmall(auto MaskSmall, auto nBlkXP, auto nBlkYP, auto nBlkX, auto nBlkY) {
	if (nBlkXP > nBlkX)
		for (auto j : Range{ nBlkY })
			for (auto dx : Range{ nBlkX, nBlkXP })
				MaskSmall[j * nBlkXP + dx] = MaskSmall[j * nBlkXP + nBlkX - 1];
	if (nBlkYP > nBlkY)
		for (auto i : Range{ nBlkXP })
			for (auto dy : Range{ nBlkY, nBlkYP })
				MaskSmall[nBlkXP * dy + i] = MaskSmall[nBlkXP * (nBlkY - 1) + i];
}

auto CheckAndPadSmallY(auto VXSmallY, auto VYSmallY, auto nBlkXP, auto nBlkYP, auto nBlkX, auto nBlkY) {
	using PixelType = std::decay_t<decltype(VXSmallY[0])>;
	constexpr auto Zero = static_cast<PixelType>(0);
	if (nBlkXP > nBlkX)
		for (auto j : Range{ nBlkY })
			for (auto dx : Range{ nBlkX, nBlkXP }) {
				VXSmallY[j * nBlkXP + dx] = std::min(VXSmallY[j * nBlkXP + nBlkX - 1], Zero);
				VYSmallY[j * nBlkXP + dx] = VYSmallY[j * nBlkXP + nBlkX - 1];
			}
	if (nBlkYP > nBlkY)
		for (auto i : Range{ nBlkXP })
			for (auto dy : Range{ nBlkY, nBlkYP }) {
				VXSmallY[nBlkXP * dy + i] = VXSmallY[nBlkXP * (nBlkY - 1) + i];
				VYSmallY[nBlkXP * dy + i] = std::min(VYSmallY[nBlkXP * (nBlkY - 1) + i], Zero);
			}
}

template <typename PixelType>
static auto RealMerge4PlanesToBig(uint8_t* pel2Plane_u8, int32_t pel2Pitch, const uint8_t* pPlane0_u8, const uint8_t* pPlane1_u8,
	const uint8_t* pPlane2_u8, const uint8_t* pPlane3_u8, int32_t width, int32_t height, int32_t pitch) {
	for (auto h = 0; h < height; ++h) {
		for (auto w = 0; w < width; ++w) {
			auto pel2Plane = reinterpret_cast<PixelType*>(pel2Plane_u8);
			auto pPlane0 = reinterpret_cast<const PixelType*>(pPlane0_u8);
			auto pPlane1 = reinterpret_cast<const PixelType*>(pPlane1_u8);
			pel2Plane[w << 1] = pPlane0[w];
			pel2Plane[(w << 1) + 1] = pPlane1[w];
		}
		pel2Plane_u8 += pel2Pitch;
		for (auto w = 0; w < width; ++w) {
			auto pel2Plane = reinterpret_cast<PixelType*>(pel2Plane_u8);
			auto pPlane2 = reinterpret_cast<const PixelType*>(pPlane2_u8);
			auto pPlane3 = reinterpret_cast<const PixelType*>(pPlane3_u8);
			pel2Plane[w << 1] = pPlane2[w];
			pel2Plane[(w << 1) + 1] = pPlane3[w];
		}
		pel2Plane_u8 += pel2Pitch;
		pPlane0_u8 += pitch;
		pPlane1_u8 += pitch;
		pPlane2_u8 += pitch;
		pPlane3_u8 += pitch;
	}
}

static auto Merge4PlanesToBig(uint8_t* pel2Plane, int32_t pel2Pitch, const uint8_t* pPlane0, const uint8_t* pPlane1, const uint8_t* pPlane2, const uint8_t* pPlane3, int32_t width, int32_t height, int32_t pitch) {
	RealMerge4PlanesToBig<float>(pel2Plane, pel2Pitch, pPlane0, pPlane1, pPlane2, pPlane3, width, height, pitch);
}

template <typename PixelType>
static void RealMerge16PlanesToBig(uint8_t* pel4Plane_u8, int32_t pel4Pitch,
	const uint8_t* pPlane0_u8, const uint8_t* pPlane1_u8, const uint8_t* pPlane2_u8, const uint8_t* pPlane3_u8,
	const uint8_t* pPlane4_u8, const uint8_t* pPlane5_u8, const uint8_t* pPlane6_u8, const uint8_t* pPlane7_u8,
	const uint8_t* pPlane8_u8, const uint8_t* pPlane9_u8, const uint8_t* pPlane10_u8, const uint8_t* pPlane11_u8,
	const uint8_t* pPlane12_u8, const uint8_t* pPlane13_u8, const uint8_t* pPlane14_u8, const uint8_t* pPlane15_u8,
	int32_t width, int32_t height, int32_t pitch) {
	for (auto h = 0; h < height; ++h) {
		for (auto w = 0; w < width; ++w) {
			PixelType* pel4Plane = (PixelType*)pel4Plane_u8;
			const PixelType* pPlane0 = (const PixelType*)pPlane0_u8;
			const PixelType* pPlane1 = (const PixelType*)pPlane1_u8;
			const PixelType* pPlane2 = (const PixelType*)pPlane2_u8;
			const PixelType* pPlane3 = (const PixelType*)pPlane3_u8;
			pel4Plane[w << 2] = pPlane0[w];
			pel4Plane[(w << 2) + 1] = pPlane1[w];
			pel4Plane[(w << 2) + 2] = pPlane2[w];
			pel4Plane[(w << 2) + 3] = pPlane3[w];
		}
		pel4Plane_u8 += pel4Pitch;
		for (auto w = 0; w < width; ++w) {
			PixelType* pel4Plane = (PixelType*)pel4Plane_u8;
			const PixelType* pPlane4 = (const PixelType*)pPlane4_u8;
			const PixelType* pPlane5 = (const PixelType*)pPlane5_u8;
			const PixelType* pPlane6 = (const PixelType*)pPlane6_u8;
			const PixelType* pPlane7 = (const PixelType*)pPlane7_u8;
			pel4Plane[w << 2] = pPlane4[w];
			pel4Plane[(w << 2) + 1] = pPlane5[w];
			pel4Plane[(w << 2) + 2] = pPlane6[w];
			pel4Plane[(w << 2) + 3] = pPlane7[w];
		}
		pel4Plane_u8 += pel4Pitch;
		for (auto w = 0; w < width; ++w) {
			PixelType* pel4Plane = (PixelType*)pel4Plane_u8;
			const PixelType* pPlane8 = (const PixelType*)pPlane8_u8;
			const PixelType* pPlane9 = (const PixelType*)pPlane9_u8;
			const PixelType* pPlane10 = (const PixelType*)pPlane10_u8;
			const PixelType* pPlane11 = (const PixelType*)pPlane11_u8;
			pel4Plane[w << 2] = pPlane8[w];
			pel4Plane[(w << 2) + 1] = pPlane9[w];
			pel4Plane[(w << 2) + 2] = pPlane10[w];
			pel4Plane[(w << 2) + 3] = pPlane11[w];
		}
		pel4Plane_u8 += pel4Pitch;
		for (auto w = 0; w < width; ++w) {
			PixelType* pel4Plane = (PixelType*)pel4Plane_u8;
			const PixelType* pPlane12 = (const PixelType*)pPlane12_u8;
			const PixelType* pPlane13 = (const PixelType*)pPlane13_u8;
			const PixelType* pPlane14 = (const PixelType*)pPlane14_u8;
			const PixelType* pPlane15 = (const PixelType*)pPlane15_u8;
			pel4Plane[w << 2] = pPlane12[w];
			pel4Plane[(w << 2) + 1] = pPlane13[w];
			pel4Plane[(w << 2) + 2] = pPlane14[w];
			pel4Plane[(w << 2) + 3] = pPlane15[w];
		}
		pel4Plane_u8 += pel4Pitch;
		pPlane0_u8 += pitch;
		pPlane1_u8 += pitch;
		pPlane2_u8 += pitch;
		pPlane3_u8 += pitch;
		pPlane4_u8 += pitch;
		pPlane5_u8 += pitch;
		pPlane6_u8 += pitch;
		pPlane7_u8 += pitch;
		pPlane8_u8 += pitch;
		pPlane9_u8 += pitch;
		pPlane10_u8 += pitch;
		pPlane11_u8 += pitch;
		pPlane12_u8 += pitch;
		pPlane13_u8 += pitch;
		pPlane14_u8 += pitch;
		pPlane15_u8 += pitch;
	}
}

static void Merge16PlanesToBig(uint8_t* pel4Plane, int32_t pel4Pitch,
	const uint8_t* pPlane0, const uint8_t* pPlane1, const uint8_t* pPlane2, const uint8_t* pPlane3,
	const uint8_t* pPlane4, const uint8_t* pPlane5, const uint8_t* pPlane6, const uint8_t* pPlane7,
	const uint8_t* pPlane8, const uint8_t* pPlane9, const uint8_t* pPlane10, const uint8_t* pPlane11,
	const uint8_t* pPlane12, const uint8_t* pPlane13, const uint8_t* pPlane14, const uint8_t* pPlane15,
	int32_t width, int32_t height, int32_t pitch) {
	RealMerge16PlanesToBig<float>(pel4Plane, pel4Pitch, pPlane0, pPlane1, pPlane2, pPlane3, pPlane4, pPlane5, pPlane6, pPlane7, pPlane8, pPlane9, pPlane10, pPlane11, pPlane12, pPlane13, pPlane14, pPlane15, width, height, pitch);
}

static void MakeVectorSmallMasks(MVClipBalls& mvClip, int32_t nBlkX, int32_t nBlkY, int32_t* VXSmallY, int32_t pitchVXSmallY, int32_t* VYSmallY, int32_t pitchVYSmallY) {
	for (auto by = 0; by < nBlkY; ++by)
		for (auto bx = 0; bx < nBlkX; ++bx) {
			auto i = bx + by * nBlkX;
			auto& block = mvClip[0][i];
			int32_t vx = block.GetMV().x;
			int32_t vy = block.GetMV().y;
			VXSmallY[bx + by * pitchVXSmallY] = vx;
			VYSmallY[bx + by * pitchVYSmallY] = vy;
		}
}

static void VectorSmallMaskYToHalfUV(int32_t* VSmallY, int32_t nBlkX, int32_t nBlkY, int32_t* VSmallUV, int32_t ratioUV) {
	if (ratioUV == 2) {
		for (auto by = 0; by < nBlkY; ++by) {
			for (auto bx = 0; bx < nBlkX; ++bx)
				VSmallUV[bx] = VSmallY[bx] >> 1;
			VSmallY += nBlkX;
			VSmallUV += nBlkX;
		}
	}
	else {
		for (auto by = 0; by < nBlkY; ++by) {
			for (auto bx = 0; bx < nBlkX; ++bx)
				VSmallUV[bx] = VSmallY[bx];
			VSmallY += nBlkX;
			VSmallUV += nBlkX;
		}
	}
}

template <typename PixelType>
static void RealBlend(uint8_t* pdst, const uint8_t* psrc, const uint8_t* pref, int32_t height, int32_t width, int32_t dst_pitch, int32_t src_pitch, int32_t ref_pitch, int32_t time256) {
	for (auto h = 0; h < height; ++h) {
		for (auto w = 0; w < width; ++w) {
			const PixelType* psrc_ = (const PixelType*)psrc;
			const PixelType* pref_ = (const PixelType*)pref;
			PixelType* pdst_ = (PixelType*)pdst;
			pdst_[w] = (psrc_[w] * (256 - time256) + pref_[w] * time256) / 256;
		}
		pdst += dst_pitch;
		psrc += src_pitch;
		pref += ref_pitch;
	}
}

static void Blend(uint8_t* pdst, const uint8_t* psrc, const uint8_t* pref, int32_t height, int32_t width, int32_t dst_pitch, int32_t src_pitch, int32_t ref_pitch, int32_t time256) {
	RealBlend<float>(pdst, psrc, pref, height, width, dst_pitch, src_pitch, ref_pitch, time256);
}

static inline void ByteOccMask(double* occMask, int32_t occlusion, double occnorm, double fGamma) {
	if (fGamma == 1.0)
		*occMask = std::max(*occMask, std::min((255. * occlusion * occnorm), 255.));
	else
		*occMask = std::max(*occMask, std::min((255. * pow(occlusion * occnorm, fGamma)), 255.));
}

static void MakeVectorOcclusionMaskTime(MVClipBalls& mvClip, bool isb, int32_t nBlkX, int32_t nBlkY, double dMaskNormDivider, double fGamma, int32_t nPel, double* occMask, int32_t occMaskPitch, int32_t time256, int32_t nBlkStepX, int32_t nBlkStepY) {
	for (auto i = 0; i < occMaskPitch * nBlkY; ++i)
		occMask[i] = 0.;
	int time4096X = time256 * 16 / (nBlkStepX * nPel);
	int time4096Y = time256 * 16 / (nBlkStepY * nPel);
	double occnormX = 80. / (dMaskNormDivider * nBlkStepX * nPel);
	double occnormY = 80. / (dMaskNormDivider * nBlkStepY * nPel);
	int32_t occlusion;
	for (auto by = 0; by < nBlkY; ++by)
		for (auto bx = 0; bx < nBlkX; ++bx) {
			int32_t i = bx + by * nBlkX;
			auto& block = mvClip[0][i];
			int32_t vx = block.GetMV().x;
			int32_t vy = block.GetMV().y;
			if (bx < nBlkX - 1) {
				int32_t i1 = i + 1;
				auto& block1 = mvClip[0][i1];
				int32_t vx1 = block1.GetMV().x;
				if (vx1 < vx) {
					occlusion = vx - vx1;
					int32_t minb = isb ? std::max(0, bx + 1 - occlusion * time4096X / 4096) : bx;
					int32_t maxb = isb ? bx + 1 : std::min(bx + 1 - occlusion * time4096X / 4096, nBlkX - 1);
					for (int bxi = minb; bxi <= maxb; ++bxi)
						ByteOccMask(&occMask[bxi + by * occMaskPitch], occlusion, occnormX, fGamma);
				}
			}
			if (by < nBlkY - 1) {
				int32_t i1 = i + nBlkX;
				auto& block1 = mvClip[0][i1];
				int32_t vy1 = block1.GetMV().y;
				if (vy1 < vy) {
					occlusion = vy - vy1;
					int32_t minb = isb ? std::max(0, by + 1 - occlusion * time4096Y / 4096) : by;
					int32_t maxb = isb ? by + 1 : std::min(by + 1 - occlusion * time4096Y / 4096, nBlkY - 1);
					for (int byi = minb; byi <= maxb; ++byi)
						ByteOccMask(&occMask[bx + byi * occMaskPitch], occlusion, occnormY, fGamma);
				}
			}
		}
}

static double ByteNorm(double sad, double dSADNormFactor, double fGamma) {
	double l = 255. * pow(sad * dSADNormFactor, fGamma);
	return (l > 255.) ? 255. : l;
}

static void MakeSADMaskTime(MVClipBalls& mvClip, int32_t nBlkX, int32_t nBlkY, double dSADNormFactor, double fGamma, int32_t nPel, double* Mask, int32_t MaskPitch, int32_t time256, int32_t nBlkStepX, int32_t nBlkStepY) {
	for (auto i = 0; i < nBlkY * MaskPitch; ++i)
		Mask[i] = 0.;
	int32_t time4096X = (256 - time256) * 16 / (nBlkStepX * nPel);
	int32_t time4096Y = (256 - time256) * 16 / (nBlkStepY * nPel);
	for (auto by = 0; by < nBlkY; ++by) {
		for (auto bx = 0; bx < nBlkX; ++bx) {
			auto i = bx + by * nBlkX;
			auto& block = mvClip[0][i];
			int32_t vx = block.GetMV().x;
			int32_t vy = block.GetMV().y;
			int32_t bxi = bx - vx * time4096X / 4096;
			int32_t byi = by - vy * time4096Y / 4096;
			if (bxi < 0 || bxi >= nBlkX || byi < 0 || byi >= nBlkY) {
				bxi = bx;
				byi = by;
			}
			int32_t i1 = bxi + byi * nBlkX;
			auto sad = mvClip[0][i1].GetSAD();
			Mask[bx + by * MaskPitch] = ByteNorm(sad, dSADNormFactor, fGamma);
		}
	}
}

static inline float Median3r(float a, float b, float c) {
	if (b <= a) return a;
	else  if (c <= b) return c;
	else return b;
}

template <typename PixelType>
static void RealFlowInterExtra(uint8_t* pdst8, int32_t dst_pitch, const uint8_t* prefB8, const uint8_t* prefF8, int32_t ref_pitch,
	int32_t* VXFullB, int32_t* VXFullF, int32_t* VYFullB, int32_t* VYFullF, double* MaskB, double* MaskF,
	int32_t VPitch, int32_t width, int32_t height, int32_t time256, int32_t nPel,
	int32_t* VXFullBB, int32_t* VXFullFF, int32_t* VYFullBB, int32_t* VYFullFF) {
	const PixelType* prefB = reinterpret_cast<const PixelType*>(prefB8);
	const PixelType* prefF = reinterpret_cast<const PixelType*>(prefF8);
	PixelType* pdst = (PixelType*)pdst8;
	ref_pitch /= sizeof(PixelType);
	dst_pitch /= sizeof(PixelType);
	if (nPel == 1) {
		for (auto h = 0; h < height; ++h) {
			for (auto w = 0; w < width; ++w) {
				auto vxF = (VXFullF[w] * time256) >> 8;
				auto vyF = (VYFullF[w] * time256) >> 8;
				int32_t adrF = vyF * ref_pitch + vxF + w;
				float dstF = prefF[adrF];
				auto vxFF = (VXFullFF[w] * time256) >> 8;
				auto vyFF = (VYFullFF[w] * time256) >> 8;
				int32_t adrFF = vyFF * ref_pitch + vxFF + w;
				float dstFF = prefF[adrFF];
				auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
				auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
				int32_t adrB = vyB * ref_pitch + vxB + w;
				float dstB = prefB[adrB];
				auto vxBB = (VXFullBB[w] * (256 - time256)) >> 8;
				auto vyBB = (VYFullBB[w] * (256 - time256)) >> 8;
				int32_t adrBB = vyBB * ref_pitch + vxBB + w;
				float dstBB = prefB[adrBB];
				float minfb;
				float maxfb;
				if (dstF > dstB) {
					minfb = dstB;
					maxfb = dstF;
				}
				else {
					maxfb = dstB;
					minfb = dstF;
				}
				pdst[w] = static_cast<PixelType>((((Median3r(minfb, dstBB, maxfb) * MaskF[w] + dstF * (255 - MaskF[w])) / 256) * (256 - time256) +
					((Median3r(minfb, dstFF, maxfb) * MaskB[w] + dstB * (255 - MaskB[w])) / 256) * time256) / 256);
			}
			pdst += dst_pitch;
			prefB += ref_pitch;
			prefF += ref_pitch;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}
	else if (nPel == 2) {
		for (auto h = 0; h < height; ++h) {
			for (auto w = 0; w < width; ++w) {
				auto vxF = (VXFullF[w] * time256) >> 8;
				auto vyF = (VYFullF[w] * time256) >> 8;
				int32_t adrF = vyF * ref_pitch + vxF + (w << 1);
				float dstF = prefF[adrF];
				auto vxFF = (VXFullFF[w] * time256) >> 8;
				auto vyFF = (VYFullFF[w] * time256) >> 8;
				int32_t adrFF = vyFF * ref_pitch + vxFF + (w << 1);
				float dstFF = prefF[adrFF];
				auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
				auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
				int32_t adrB = vyB * ref_pitch + vxB + (w << 1);
				float dstB = prefB[adrB];
				auto vxBB = (VXFullBB[w] * (256 - time256)) >> 8;
				auto vyBB = (VYFullBB[w] * (256 - time256)) >> 8;
				int32_t adrBB = vyBB * ref_pitch + vxBB + (w << 1);
				float dstBB = prefB[adrBB];
				float minfb;
				float maxfb;
				if (dstF > dstB) {
					minfb = dstB;
					maxfb = dstF;
				}
				else {
					maxfb = dstB;
					minfb = dstF;
				}
				pdst[w] = static_cast<PixelType>((((Median3r(minfb, dstBB, maxfb) * MaskF[w] + dstF * (255 - MaskF[w])) / 256) * (256 - time256) +
					((Median3r(minfb, dstFF, maxfb) * MaskB[w] + dstB * (255 - MaskB[w])) / 256) * time256) / 256);
			}
			pdst += dst_pitch;
			prefB += ref_pitch << 1;
			prefF += ref_pitch << 1;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}
	else if (nPel == 4) {
		for (auto h = 0; h < height; ++h) {
			for (auto w = 0; w < width; ++w) {
				auto vxF = (VXFullF[w] * time256) >> 8;
				auto vyF = (VYFullF[w] * time256) >> 8;
				float dstF = prefF[vyF * ref_pitch + vxF + (w << 2)];
				auto vxFF = (VXFullFF[w] * time256) >> 8;
				auto vyFF = (VYFullFF[w] * time256) >> 8;
				float dstFF = prefF[vyFF * ref_pitch + vxFF + (w << 2)];
				auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
				auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
				float dstB = prefB[vyB * ref_pitch + vxB + (w << 2)];
				auto vxBB = (VXFullBB[w] * (256 - time256)) >> 8;
				auto vyBB = (VYFullBB[w] * (256 - time256)) >> 8;
				float dstBB = prefB[vyBB * ref_pitch + vxBB + (w << 2)];
				float minfb;
				float maxfb;
				if (dstF > dstB) {
					minfb = dstB;
					maxfb = dstF;
				}
				else {
					maxfb = dstB;
					minfb = dstF;
				}
				pdst[w] = static_cast<PixelType>((((Median3r(minfb, dstBB, maxfb) * MaskF[w] + dstF * (255 - MaskF[w])) / 256) * (256 - time256) +
					((Median3r(minfb, dstFF, maxfb) * MaskB[w] + dstB * (255 - MaskB[w])) / 256) * time256) / 256);
			}
			pdst += dst_pitch;
			prefB += ref_pitch << 2;
			prefF += ref_pitch << 2;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
			VXFullBB += VPitch;
			VYFullBB += VPitch;
			VXFullFF += VPitch;
			VYFullFF += VPitch;
		}
	}
}

static void FlowInterExtra(uint8_t* pdst, int32_t dst_pitch, const uint8_t* prefB, const uint8_t* prefF, int32_t ref_pitch,
	int32_t* VXFullB, int32_t* VXFullF, int32_t* VYFullB, int32_t* VYFullF, double* MaskB, double* MaskF,
	int32_t VPitch, int32_t width, int32_t height, int32_t time256, int32_t nPel,
	int32_t* VXFullBB, int32_t* VXFullFF, int32_t* VYFullBB, int32_t* VYFullFF) {
	RealFlowInterExtra<float>(pdst, dst_pitch, prefB, prefF, ref_pitch, VXFullB, VXFullF, VYFullB, VYFullF, MaskB, MaskF, VPitch, width, height, time256, nPel, VXFullBB, VXFullFF, VYFullBB, VYFullFF);
}

template <typename PixelType>
static void RealFlowInter(uint8_t* pdst8, int32_t dst_pitch, const uint8_t* prefB8, const uint8_t* prefF8, int32_t ref_pitch,
	int32_t* VXFullB, int32_t* VXFullF, int32_t* VYFullB, int32_t* VYFullF, double* MaskB, double* MaskF,
	int32_t VPitch, int32_t width, int32_t height, int32_t time256, int32_t nPel) {
	const PixelType* prefB = reinterpret_cast<const PixelType*>(prefB8);
	const PixelType* prefF = reinterpret_cast<const PixelType*>(prefF8);
	PixelType* pdst = reinterpret_cast<PixelType*>(pdst8);
	ref_pitch /= sizeof(PixelType);
	dst_pitch /= sizeof(PixelType);
	if (nPel == 1) {
		for (auto h = 0; h < height; ++h) {
			for (auto w = 0; w < width; ++w) {
				auto vxF = (VXFullF[w] * time256) >> 8;
				auto vyF = (VYFullF[w] * time256) >> 8;
				double dstF = prefF[vyF * ref_pitch + vxF + w];
				float dstF0 = prefF[w];
				auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
				auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
				double dstB = prefB[vyB * ref_pitch + vxB + w];
				float dstB0 = prefB[w];
				pdst[w] = static_cast<PixelType>((((dstF * (255 - MaskF[w]) + ((MaskF[w] * (dstB * (255 - MaskB[w]) + MaskB[w] * dstF0)) / 256)) / 256) * (256 - time256) +
					((dstB * (255 - MaskB[w]) + ((MaskB[w] * (dstF * (255 - MaskF[w]) + MaskF[w] * dstB0)) / 256)) / 256) * time256) / 256);
			}
			pdst += dst_pitch;
			prefB += ref_pitch;
			prefF += ref_pitch;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
	else if (nPel == 2) {
		for (auto h = 0; h < height; ++h) {
			for (auto w = 0; w < width; ++w) {
				auto vxF = (VXFullF[w] * time256) >> 8;
				auto vyF = (VYFullF[w] * time256) >> 8;
				float dstF = prefF[vyF * ref_pitch + vxF + (w << 1)];
				float dstF0 = prefF[(w << 1)];
				auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
				auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
				float dstB = prefB[vyB * ref_pitch + vxB + (w << 1)];
				float dstB0 = prefB[(w << 1)];
				pdst[w] = static_cast<PixelType>((((dstF * (255 - MaskF[w]) + ((MaskF[w] * (dstB * (255 - MaskB[w]) + MaskB[w] * dstF0)) / 256)) / 256) * (256 - time256) +
					((dstB * (255 - MaskB[w]) + ((MaskB[w] * (dstF * (255 - MaskF[w]) + MaskF[w] * dstB0)) / 256)) / 256) * time256) / 256);
			}
			pdst += dst_pitch;
			prefB += ref_pitch << 1;
			prefF += ref_pitch << 1;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
	else if (nPel == 4) {
		for (auto h = 0; h < height; ++h) {
			for (auto w = 0; w < width; ++w) {
				auto vxF = (VXFullF[w] * time256) >> 8;
				auto vyF = (VYFullF[w] * time256) >> 8;
				float dstF = prefF[vyF * ref_pitch + vxF + (w << 2)];
				float dstF0 = prefF[(w << 2)];
				auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
				auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
				float dstB = prefB[vyB * ref_pitch + vxB + (w << 2)];
				float dstB0 = prefB[(w << 2)];
				pdst[w] = static_cast<PixelType>((((dstF * (255 - MaskF[w]) + ((MaskF[w] * (dstB * (255 - MaskB[w]) + MaskB[w] * dstF0)) / 256)) / 256) * (256 - time256) +
					((dstB * (255 - MaskB[w]) + ((MaskB[w] * (dstF * (255 - MaskF[w]) + MaskF[w] * dstB0)) / 256)) / 256) * time256) / 256);
			}
			pdst += dst_pitch;
			prefB += ref_pitch << 2;
			prefF += ref_pitch << 2;
			VXFullB += VPitch;
			VYFullB += VPitch;
			VXFullF += VPitch;
			VYFullF += VPitch;
			MaskB += VPitch;
			MaskF += VPitch;
		}
	}
}

static void FlowInter(uint8_t* pdst, int32_t dst_pitch, const uint8_t* prefB, const uint8_t* prefF, int32_t ref_pitch,
	int32_t* VXFullB, int32_t* VXFullF, int32_t* VYFullB, int32_t* VYFullF, double* MaskB, double* MaskF,
	int32_t VPitch, int32_t width, int32_t height, int32_t time256, int32_t nPel) {
	RealFlowInter<float>(pdst, dst_pitch, prefB, prefF, ref_pitch, VXFullB, VXFullF, VYFullB, VYFullF, MaskB, MaskF, VPitch, width, height, time256, nPel);
}

template <typename PixelType>
static void RealFlowInterSimple(uint8_t* pdst8, int32_t dst_pitch, const uint8_t* prefB8, const uint8_t* prefF8, int32_t ref_pitch,
	int32_t* VXFullB, int32_t* VXFullF, int32_t* VYFullB, int32_t* VYFullF, double* MaskB, double* MaskF,
	int32_t VPitch, int32_t width, int32_t height, int32_t time256, int32_t nPel) {
	const PixelType* prefB = reinterpret_cast<const PixelType*>(prefB8);
	const PixelType* prefF = reinterpret_cast<const PixelType*>(prefF8);
	PixelType* pdst = (PixelType*)pdst8;
	ref_pitch /= sizeof(PixelType);
	dst_pitch /= sizeof(PixelType);
	if (time256 == 128) {
		if (nPel == 1) {
			for (auto h = 0; h < height; ++h) {
				for (auto w = 0; w < width; w += 2) {
					auto vxF = VXFullF[w] >> 1;
					auto vyF = VYFullF[w] >> 1;
					int32_t addrF = vyF * ref_pitch + vxF + w;
					float dstF = prefF[addrF];
					float dstF1 = prefF[addrF + 1];
					auto vxB = VXFullB[w] >> 1;
					auto vyB = VYFullB[w] >> 1;
					int32_t addrB = vyB * ref_pitch + vxB + w;
					float dstB = prefB[addrB];
					float dstB1 = prefB[addrB + 1];
					pdst[w] = static_cast<PixelType>((((dstF + dstB) * 256) + (dstB - dstF) * (MaskF[w] - MaskB[w])) / 512);
					pdst[w + 1] = static_cast<PixelType>((((dstF1 + dstB1) * 256) + (dstB1 - dstF1) * (MaskF[w + 1] - MaskB[w + 1])) / 512);
				}
				pdst += dst_pitch;
				prefB += ref_pitch;
				prefF += ref_pitch;
				VXFullB += VPitch;
				VYFullB += VPitch;
				VXFullF += VPitch;
				VYFullF += VPitch;
				MaskB += VPitch;
				MaskF += VPitch;
			}
		}
		else if (nPel == 2) {
			for (auto h = 0; h < height; ++h) {
				for (auto w = 0; w < width; w += 1) {
					auto vxF = VXFullF[w] >> 1;
					auto vyF = VYFullF[w] >> 1;
					float dstF = prefF[vyF * ref_pitch + vxF + (w << 1)];
					auto vxB = VXFullB[w] >> 1;
					auto vyB = VYFullB[w] >> 1;
					float dstB = prefB[vyB * ref_pitch + vxB + (w << 1)];
					pdst[w] = static_cast<PixelType>((((dstF + dstB) * 256) + (dstB - dstF) * (MaskF[w] - MaskB[w])) / 512);
				}
				pdst += dst_pitch;
				prefB += ref_pitch << 1;
				prefF += ref_pitch << 1;
				VXFullB += VPitch;
				VYFullB += VPitch;
				VXFullF += VPitch;
				VYFullF += VPitch;
				MaskB += VPitch;
				MaskF += VPitch;
			}
		}
		else if (nPel == 4) {
			for (auto h = 0; h < height; ++h) {
				for (auto w = 0; w < width; w += 1) {
					auto vxF = VXFullF[w] >> 1;
					auto vyF = VYFullF[w] >> 1;
					float dstF = prefF[vyF * ref_pitch + vxF + (w << 2)];
					auto vxB = VXFullB[w] >> 1;
					auto vyB = VYFullB[w] >> 1;
					float dstB = prefB[vyB * ref_pitch + vxB + (w << 2)];
					pdst[w] = static_cast<PixelType>((((dstF + dstB) * 256) + (dstB - dstF) * (MaskF[w] - MaskB[w])) / 512);
				}
				pdst += dst_pitch;
				prefB += ref_pitch << 2;
				prefF += ref_pitch << 2;
				VXFullB += VPitch;
				VYFullB += VPitch;
				VXFullF += VPitch;
				VYFullF += VPitch;
				MaskB += VPitch;
				MaskF += VPitch;
			}
		}
	}
	else {
		if (nPel == 1) {
			for (auto h = 0; h < height; ++h) {
				for (auto w = 0; w < width; w += 2) {
					auto vxF = (VXFullF[w] * time256) >> 8;
					auto vyF = (VYFullF[w] * time256) >> 8;
					int32_t addrF = vyF * ref_pitch + vxF + w;
					float dstF = prefF[addrF];
					float dstF1 = prefF[addrF + 1];
					auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
					auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
					int32_t addrB = vyB * ref_pitch + vxB + w;
					float dstB = prefB[addrB];
					float dstB1 = prefB[addrB + 1];
					pdst[w] = static_cast<PixelType>((((dstF * 255 + (dstB - dstF) * MaskF[w])) * (256 - time256) +
						((dstB * 255 - (dstB - dstF) * MaskB[w])) * time256) / 65536);
					pdst[w + 1] = static_cast<PixelType>((((dstF1 * 255 + (dstB1 - dstF1) * MaskF[w + 1])) * (256 - time256) +
						((dstB1 * 255 - (dstB1 - dstF1) * MaskB[w + 1])) * time256) / 65536);
				}
				pdst += dst_pitch;
				prefB += ref_pitch;
				prefF += ref_pitch;
				VXFullB += VPitch;
				VYFullB += VPitch;
				VXFullF += VPitch;
				VYFullF += VPitch;
				MaskB += VPitch;
				MaskF += VPitch;
			}
		}
		else if (nPel == 2) {
			for (auto h = 0; h < height; ++h) {
				for (auto w = 0; w < width; w += 1) {
					auto vxF = (VXFullF[w] * time256) >> 8;
					auto vyF = (VYFullF[w] * time256) >> 8;
					float dstF = prefF[vyF * ref_pitch + vxF + (w << 1)];
					auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
					auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
					float dstB = prefB[vyB * ref_pitch + vxB + (w << 1)];
					pdst[w] = static_cast<PixelType>((((dstF * (255 - MaskF[w]) + dstB * MaskF[w]) / 256) * (256 - time256) +
						((dstB * (255 - MaskB[w]) + dstF * MaskB[w]) / 256) * time256) / 256);
				}
				pdst += dst_pitch;
				prefB += ref_pitch << 1;
				prefF += ref_pitch << 1;
				VXFullB += VPitch;
				VYFullB += VPitch;
				VXFullF += VPitch;
				VYFullF += VPitch;
				MaskB += VPitch;
				MaskF += VPitch;
			}
		}
		else if (nPel == 4) {
			for (auto h = 0; h < height; ++h) {
				for (auto w = 0; w < width; w += 1) {
					auto vxF = (VXFullF[w] * time256) >> 8;
					auto vyF = (VYFullF[w] * time256) >> 8;
					float dstF = prefF[vyF * ref_pitch + vxF + (w << 2)];
					auto vxB = (VXFullB[w] * (256 - time256)) >> 8;
					auto vyB = (VYFullB[w] * (256 - time256)) >> 8;
					float dstB = prefB[vyB * ref_pitch + vxB + (w << 2)];
					pdst[w] = static_cast<PixelType>((((dstF * (255 - MaskF[w]) + dstB * MaskF[w]) / 256) * (256 - time256) +
						((dstB * (255 - MaskB[w]) + dstF * MaskB[w]) / 256) * time256) / 256);
				}
				pdst += dst_pitch;
				prefB += ref_pitch << 2;
				prefF += ref_pitch << 2;
				VXFullB += VPitch;
				VYFullB += VPitch;
				VXFullF += VPitch;
				VYFullF += VPitch;
				MaskB += VPitch;
				MaskF += VPitch;
			}
		}
	}
}

static void FlowInterSimple(uint8_t* pdst, int32_t dst_pitch, const uint8_t* prefB, const uint8_t* prefF, int32_t ref_pitch,
	int32_t* VXFullB, int32_t* VXFullF, int32_t* VYFullB, int32_t* VYFullF, double* MaskB, double* MaskF,
	int32_t VPitch, int32_t width, int32_t height, int32_t time256, int32_t nPel) {
	RealFlowInterSimple<float>(pdst, dst_pitch, prefB, prefF, ref_pitch, VXFullB, VXFullF, VYFullB, VYFullF, MaskB, MaskF, VPitch, width, height, time256, nPel);
}

