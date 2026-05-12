/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "amiga_blit_info.h"
#include "amiga_blittervis.h"
#include "amiga_custom_ui.h"
#include "libretro_host.h"

#define EMU_AMI_BLITTER_VIS_POINTS_CAP_DEFAULT (2304u * 1620u)
#define EMU_AMI_BLITTER_VIS_LINE_TABLE_CAP_MAX (1u << 20)
#define EMU_AMI_BLITTER_VIS_WORD_SHIFT_PIXELS 16
#define EMU_AMI_BLITTER_VIS_Y_GAP_MAX 1024u
#define EMU_AMI_BLITTER_VIS_MODE_OVERLAY 0x2u
#define EMU_AMI_BLITTER_VIS_ALPHA_MAX 0xb0u
#define EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MIN 1024u
#define EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MAX (1u << 20)
#define EMU_AMI_BLITTER_VIS_RETAINED_HISTORY 4u

typedef struct emu_ami_blitter_vis_line_stat {
    uint32_t blitId;
    uint32_t y;
    uint32_t minX;
    uint32_t maxX;
    uint32_t count;
    uint8_t used;
} emu_ami_blitter_vis_line_stat_t;

typedef struct emu_ami_blitter_vis_analysis {
    uint32_t lineCount;
    uint32_t droppedEntries;
    uint32_t comparedPairs;
    int maxAbsDelta;
    uint32_t maxAbsDeltaBlitId;
    uint32_t maxAbsDeltaY;
    uint32_t maxAbsDeltaPrevY;
    int maxAbsDeltaMin;
    int maxAbsDeltaMax;
    uint32_t maxPrevMinX;
    uint32_t maxPrevMaxX;
    uint32_t maxCurrMinX;
    uint32_t maxCurrMaxX;
    uint32_t blitsWithMinXVariance;
    uint32_t maxMinXSpread;
    uint32_t maxMinXSpreadBlitId;
    uint32_t maxMinXSpreadLowY;
    uint32_t maxMinXSpreadHighY;
    uint32_t maxMinXSpreadLowMinX;
    uint32_t maxMinXSpreadHighMinX;
    uint32_t wordShiftSameCount;
    uint32_t firstWordShiftBlitId;
    uint32_t firstWordShiftY;
    uint32_t firstWordShiftPrevY;
    int firstWordShiftMin;
    int firstWordShiftMax;
} emu_ami_blitter_vis_analysis_t;

typedef struct emu_ami_blitter_vis_cache {
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    uint32_t *pixels;
    size_t pixelsCap;
    struct emu_ami_blitter_vis_retained_pixel *retainedPixels;
    size_t retainedCap;
    uint32_t *retainedActivePixels;
    uint32_t *retainedActiveSlots;
    size_t retainedActiveCap;
    size_t retainedActiveCount;
    uint32_t *blitFrameIds;
    uint32_t *blitFrameValues;
    e9k_debug_ami_blitter_vis_point_t *blitMetaValues;
    size_t blitFrameCap;
    size_t blitFrameCount;
    int texWidth;
    int texHeight;
    e9k_debug_ami_blitter_vis_point_t *points;
    size_t pointsCap;
    size_t pointCount;
    emu_ami_blitter_vis_line_stat_t *lineTable;
    size_t lineTableCap;
    emu_ami_blitter_vis_line_stat_t *lineList;
    size_t lineListCap;
    uint32_t overlayFrameCounter;
    uint32_t hoveredBlitId;
    int hasRetainedOverlay;
    int hasLatestStats;
    e9k_debug_ami_blitter_vis_stats_t latestStats;
} emu_ami_blitter_vis_cache_t;

typedef struct emu_ami_blitter_vis_retained_pixel {
    uint32_t blitId;
    uint32_t frame;
    uint32_t rgb;
    uint32_t sourceInfo;
    uint32_t sourceDataAddr;
    uint32_t channelAAddr;
    uint32_t channelBAddr;
    uint32_t channelCAddr;
    uint32_t channelDAddr;
    int16_t channelAModulo;
    int16_t channelBModulo;
    int16_t channelCModulo;
    int16_t channelDModulo;
    uint16_t widthWords;
    uint16_t heightLines;
    uint16_t sourceRowBytes;
    int16_t sourceModulo;
    uint8_t sourceChannelsMask;
    uint8_t minterm;
    uint8_t sourceDescending;
    uint8_t lineMode;
    uint8_t historyCount;
    uint32_t historyBlitIds[EMU_AMI_BLITTER_VIS_RETAINED_HISTORY];
} emu_ami_blitter_vis_retained_pixel_t;

typedef struct emu_ami_blitter_vis_hit {
    uint16_t x;
    uint16_t y;
    uint16_t xEnd;
    uint32_t blitId;
    uint32_t sourceAddr;
    uint32_t sourceDataAddr;
    uint32_t channelAAddr;
    uint32_t channelBAddr;
    uint32_t channelCAddr;
    uint32_t channelDAddr;
    int16_t channelAModulo;
    int16_t channelBModulo;
    int16_t channelCModulo;
    int16_t channelDModulo;
    uint16_t widthWords;
    uint16_t heightLines;
    uint16_t sourceRowBytes;
    int16_t sourceModulo;
    uint8_t sourceChannelsMask;
    uint8_t minterm;
    uint8_t sourceIsCopper;
    uint8_t sourceDescending;
    uint8_t lineMode;
} emu_ami_blitter_vis_hit_t;

#define EMU_AMI_BLITTER_VIS_MAX_HITS_AT_POINT 8

static int
amiga_blittervis_getBlitMeta(const emu_ami_blitter_vis_cache_t *cache,
                              uint32_t blitId,
                              e9k_debug_ami_blitter_vis_point_t *outPoint,
                              uint32_t *outFrame);

static emu_ami_blitter_vis_cache_t amiga_blittervis_cache = {0};

static uint32_t
amiga_blittervis_colorFromId(uint32_t blitId)
{
    uint32_t h = blitId ? blitId : 1u;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;

    uint8_t r = (uint8_t)(64u + (h & 0x7fu));
    uint8_t g = (uint8_t)(64u + ((h >> 8) & 0x7fu));
    uint8_t b = (uint8_t)(64u + ((h >> 16) & 0x7fu));
    return (uint32_t)(0xb0u << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static uint32_t
amiga_blittervis_retainedPackSource(uint32_t sourceAddr, int sourceIsCopper)
{
    uint32_t packed = sourceAddr & 0x00ffffffu;
    if (sourceIsCopper) {
        packed |= 0x80000000u;
    }
    return packed;
}

static __attribute__((unused)) uint32_t
amiga_blittervis_retainedSourceAddr(uint32_t sourceInfo)
{
    return sourceInfo & 0x00ffffffu;
}

static __attribute__((unused)) int
amiga_blittervis_retainedSourceIsCopper(uint32_t sourceInfo)
{
    return (sourceInfo & 0x80000000u) != 0u ? 1 : 0;
}

static void
amiga_blittervis_retainedActivate(size_t pixelIndex)
{
    if (!amiga_blittervis_cache.retainedActivePixels ||
        !amiga_blittervis_cache.retainedActiveSlots ||
        pixelIndex >= amiga_blittervis_cache.retainedCap ||
        pixelIndex >= amiga_blittervis_cache.retainedActiveCap) {
        return;
    }
    if (amiga_blittervis_cache.retainedActiveSlots[pixelIndex] != 0u) {
        return;
    }
    if (amiga_blittervis_cache.retainedActiveCount >= amiga_blittervis_cache.retainedActiveCap) {
        return;
    }
    amiga_blittervis_cache.retainedActivePixels[amiga_blittervis_cache.retainedActiveCount] = (uint32_t)pixelIndex;
    amiga_blittervis_cache.retainedActiveSlots[pixelIndex] = (uint32_t)(amiga_blittervis_cache.retainedActiveCount + 1u);
    amiga_blittervis_cache.retainedActiveCount++;
}

static void
amiga_blittervis_retainedRemove(size_t pixelIndex)
{
    if (!amiga_blittervis_cache.retainedActivePixels ||
        !amiga_blittervis_cache.retainedActiveSlots ||
        pixelIndex >= amiga_blittervis_cache.retainedCap ||
        pixelIndex >= amiga_blittervis_cache.retainedActiveCap) {
        return;
    }
    uint32_t slotPlusOne = amiga_blittervis_cache.retainedActiveSlots[pixelIndex];
    if (slotPlusOne == 0u || amiga_blittervis_cache.retainedActiveCount == 0u) {
        return;
    }
    size_t slot = (size_t)(slotPlusOne - 1u);
    size_t lastSlot = amiga_blittervis_cache.retainedActiveCount - 1u;
    uint32_t movedPixelIndex = amiga_blittervis_cache.retainedActivePixels[lastSlot];
    amiga_blittervis_cache.retainedActiveSlots[pixelIndex] = 0u;
    if (slot != lastSlot) {
        amiga_blittervis_cache.retainedActivePixels[slot] = movedPixelIndex;
        amiga_blittervis_cache.retainedActiveSlots[movedPixelIndex] = (uint32_t)(slot + 1u);
    }
    amiga_blittervis_cache.retainedActiveCount = lastSlot;
}

static uint32_t
amiga_blittervis_hoveredIdAtPoint(const SDL_Rect *dst,
                                   int mouseX,
                                   int mouseY,
                                   uint32_t srcWidth,
                                   uint32_t srcHeight,
                                   int overlayMode,
                                   size_t fetchedCount)
{
    if (!dst || dst->w <= 0 || dst->h <= 0 || srcWidth == 0u || srcHeight == 0u) {
        return 0u;
    }
    if (mouseX < dst->x || mouseX >= dst->x + dst->w || mouseY < dst->y || mouseY >= dst->y + dst->h) {
        return 0u;
    }

    int localX = mouseX - dst->x;
    int localY = mouseY - dst->y;
    uint32_t srcX = (uint32_t)(((uint64_t)localX * srcWidth) / (uint32_t)dst->w);
    uint32_t srcY = (uint32_t)(((uint64_t)localY * srcHeight) / (uint32_t)dst->h);
    if (srcX >= srcWidth) {
        srcX = srcWidth - 1u;
    }
    if (srcY >= srcHeight) {
        srcY = srcHeight - 1u;
    }

    if (overlayMode &&
        amiga_blittervis_cache.retainedPixels &&
        amiga_blittervis_cache.retainedCap >= ((size_t)srcWidth * (size_t)srcHeight)) {
        size_t pixelIndex = (size_t)srcY * (size_t)srcWidth + (size_t)srcX;
        return amiga_blittervis_cache.retainedPixels[pixelIndex].blitId;
    }

    for (size_t i = 0u; i < fetchedCount; ++i) {
        uint32_t spanX = (uint32_t)amiga_blittervis_cache.points[i].x;
        uint32_t spanY = (uint32_t)amiga_blittervis_cache.points[i].y;
        uint32_t spanXEnd = (uint32_t)amiga_blittervis_cache.points[i].xEnd;
        if (spanXEnd < spanX) {
            spanXEnd = spanX;
        }
        if (spanY == srcY && srcX >= spanX && srcX <= spanXEnd) {
            return amiga_blittervis_cache.points[i].blitId;
        }
    }
    return 0u;
}

static void
amiga_blittervis_copyPointFromHit(e9k_debug_ami_blitter_vis_point_t *outPoint, const emu_ami_blitter_vis_hit_t *hit)
{
    if (!outPoint || !hit) {
        return;
    }
    memset(outPoint, 0, sizeof(*outPoint));
    outPoint->x = hit->x;
    outPoint->y = hit->y;
    outPoint->xEnd = hit->xEnd;
    outPoint->blitId = hit->blitId;
    outPoint->sourceAddr = hit->sourceAddr;
    outPoint->sourceDataAddr = hit->sourceDataAddr;
    outPoint->channelAAddr = hit->channelAAddr;
    outPoint->channelBAddr = hit->channelBAddr;
    outPoint->channelCAddr = hit->channelCAddr;
    outPoint->channelDAddr = hit->channelDAddr;
    outPoint->channelAModulo = hit->channelAModulo;
    outPoint->channelBModulo = hit->channelBModulo;
    outPoint->channelCModulo = hit->channelCModulo;
    outPoint->channelDModulo = hit->channelDModulo;
    outPoint->widthWords = hit->widthWords;
    outPoint->heightLines = hit->heightLines;
    outPoint->sourceRowBytes = hit->sourceRowBytes;
    outPoint->sourceModulo = hit->sourceModulo;
    outPoint->sourceChannelsMask = hit->sourceChannelsMask;
    outPoint->minterm = hit->minterm;
    outPoint->sourceIsCopper = hit->sourceIsCopper;
    outPoint->sourceDescending = hit->sourceDescending;
    outPoint->lineMode = hit->lineMode;
}

static int
amiga_blittervis_appendHit(emu_ami_blitter_vis_hit_t *outHits,
                            size_t *inOutCount,
                            size_t hitCap,
                            const emu_ami_blitter_vis_hit_t *hit)
{
    if (!outHits || !inOutCount || !hit || hit->blitId == 0u) {
        return 0;
    }
    for (size_t i = 0u; i < *inOutCount; ++i) {
        if (outHits[i].blitId == hit->blitId) {
            return 0;
        }
    }
    if (*inOutCount >= hitCap) {
        return 0;
    }
    outHits[*inOutCount] = *hit;
    *inOutCount += 1u;
    return 1;
}

static void
amiga_blittervis_retainedRememberBlit(emu_ami_blitter_vis_retained_pixel_t *retainedPixel, uint32_t blitId)
{
    if (!retainedPixel || blitId == 0u) {
        return;
    }

    size_t existingIndex = EMU_AMI_BLITTER_VIS_RETAINED_HISTORY;
    for (size_t i = 0u; i < retainedPixel->historyCount; ++i) {
        if (retainedPixel->historyBlitIds[i] == blitId) {
            existingIndex = i;
            break;
        }
    }

    if (existingIndex == 0u) {
        return;
    }

    if (existingIndex < EMU_AMI_BLITTER_VIS_RETAINED_HISTORY) {
        for (size_t i = existingIndex; i > 0u; --i) {
            retainedPixel->historyBlitIds[i] = retainedPixel->historyBlitIds[i - 1u];
        }
        retainedPixel->historyBlitIds[0] = blitId;
        return;
    }

    size_t moveCount = retainedPixel->historyCount;
    if (moveCount >= EMU_AMI_BLITTER_VIS_RETAINED_HISTORY) {
        moveCount = EMU_AMI_BLITTER_VIS_RETAINED_HISTORY - 1u;
    } else {
        retainedPixel->historyCount += 1u;
    }
    for (size_t i = moveCount; i > 0u; --i) {
        retainedPixel->historyBlitIds[i] = retainedPixel->historyBlitIds[i - 1u];
    }
    retainedPixel->historyBlitIds[0] = blitId;
}

static size_t
amiga_blittervis_hitsAtPoint(const SDL_Rect *dst,
                              int mouseX,
                              int mouseY,
                              uint32_t srcWidth,
                              uint32_t srcHeight,
                              int overlayMode,
                              uint32_t frameCounter,
                              uint32_t decayFrames,
                              size_t fetchedCount,
                              emu_ami_blitter_vis_hit_t *outHits,
                              size_t hitCap)
{
    size_t hitCount = 0u;
    emu_ami_blitter_vis_hit_t hit = {0};
    e9k_debug_ami_blitter_vis_point_t meta = {0};
    uint32_t blitFrame = 0u;

    if (outHits && hitCap > 0u) {
        memset(outHits, 0, hitCap * sizeof(*outHits));
    }
    if (!dst || dst->w <= 0 || dst->h <= 0 || srcWidth == 0u || srcHeight == 0u) {
        return 0u;
    }
    if (mouseX < dst->x || mouseX >= dst->x + dst->w || mouseY < dst->y || mouseY >= dst->y + dst->h) {
        return 0u;
    }

    int localX = mouseX - dst->x;
    int localY = mouseY - dst->y;
    uint32_t srcX = (uint32_t)(((uint64_t)localX * srcWidth) / (uint32_t)dst->w);
    uint32_t srcY = (uint32_t)(((uint64_t)localY * srcHeight) / (uint32_t)dst->h);
    if (srcX >= srcWidth) {
        srcX = srcWidth - 1u;
    }
    if (srcY >= srcHeight) {
        srcY = srcHeight - 1u;
    }

    if (overlayMode &&
        amiga_blittervis_cache.retainedPixels &&
        amiga_blittervis_cache.retainedCap >= ((size_t)srcWidth * (size_t)srcHeight)) {
        size_t pixelIndex = (size_t)srcY * (size_t)srcWidth + (size_t)srcX;
        const emu_ami_blitter_vis_retained_pixel_t *retainedPixel = &amiga_blittervis_cache.retainedPixels[pixelIndex];
        for (size_t i = 0u; i < retainedPixel->historyCount; ++i) {
            uint32_t blitId = retainedPixel->historyBlitIds[i];

            if (!blitId) {
                continue;
            }
            if (!amiga_blittervis_getBlitMeta(&amiga_blittervis_cache, blitId, &meta, &blitFrame)) {
                continue;
            }
            if (decayFrames > 0u && (frameCounter - blitFrame) >= decayFrames) {
                continue;
            }
            hit.x = meta.x;
            hit.y = meta.y;
            hit.xEnd = meta.xEnd;
            hit.blitId = meta.blitId;
            hit.sourceAddr = meta.sourceAddr;
            hit.sourceIsCopper = meta.sourceIsCopper ? 1u : 0u;
            hit.sourceDataAddr = meta.sourceDataAddr;
            hit.channelAAddr = meta.channelAAddr;
            hit.channelBAddr = meta.channelBAddr;
            hit.channelCAddr = meta.channelCAddr;
            hit.channelDAddr = meta.channelDAddr;
            hit.channelAModulo = meta.channelAModulo;
            hit.channelBModulo = meta.channelBModulo;
            hit.channelCModulo = meta.channelCModulo;
            hit.channelDModulo = meta.channelDModulo;
            hit.widthWords = meta.widthWords;
            hit.heightLines = meta.heightLines;
            hit.sourceRowBytes = meta.sourceRowBytes;
            hit.sourceModulo = meta.sourceModulo;
            hit.sourceChannelsMask = meta.sourceChannelsMask;
            hit.minterm = meta.minterm;
            hit.sourceDescending = meta.sourceDescending;
            hit.lineMode = meta.lineMode;
            (void)amiga_blittervis_appendHit(outHits, &hitCount, hitCap, &hit);
        }
        return hitCount;
    }

    for (size_t i = 0u; i < fetchedCount; ++i) {
        uint32_t spanX = (uint32_t)amiga_blittervis_cache.points[i].x;
        uint32_t spanY = (uint32_t)amiga_blittervis_cache.points[i].y;
        uint32_t spanXEnd = (uint32_t)amiga_blittervis_cache.points[i].xEnd;

        if (spanXEnd < spanX) {
            spanXEnd = spanX;
        }
        if (spanY == srcY && srcX >= spanX && srcX <= spanXEnd) {
            hit.x = amiga_blittervis_cache.points[i].x;
            hit.y = amiga_blittervis_cache.points[i].y;
            hit.xEnd = amiga_blittervis_cache.points[i].xEnd;
            hit.blitId = amiga_blittervis_cache.points[i].blitId;
            hit.sourceAddr = amiga_blittervis_cache.points[i].sourceAddr;
            hit.sourceIsCopper = amiga_blittervis_cache.points[i].sourceIsCopper ? 1u : 0u;
            hit.sourceDataAddr = amiga_blittervis_cache.points[i].sourceDataAddr;
            hit.channelAAddr = amiga_blittervis_cache.points[i].channelAAddr;
            hit.channelBAddr = amiga_blittervis_cache.points[i].channelBAddr;
            hit.channelCAddr = amiga_blittervis_cache.points[i].channelCAddr;
            hit.channelDAddr = amiga_blittervis_cache.points[i].channelDAddr;
            hit.channelAModulo = amiga_blittervis_cache.points[i].channelAModulo;
            hit.channelBModulo = amiga_blittervis_cache.points[i].channelBModulo;
            hit.channelCModulo = amiga_blittervis_cache.points[i].channelCModulo;
            hit.channelDModulo = amiga_blittervis_cache.points[i].channelDModulo;
            hit.widthWords = amiga_blittervis_cache.points[i].widthWords;
            hit.heightLines = amiga_blittervis_cache.points[i].heightLines;
            hit.sourceRowBytes = amiga_blittervis_cache.points[i].sourceRowBytes;
            hit.sourceModulo = amiga_blittervis_cache.points[i].sourceModulo;
            hit.sourceChannelsMask = amiga_blittervis_cache.points[i].sourceChannelsMask;
            hit.minterm = amiga_blittervis_cache.points[i].minterm;
            hit.sourceDescending = amiga_blittervis_cache.points[i].sourceDescending;
            hit.lineMode = amiga_blittervis_cache.points[i].lineMode;
            (void)amiga_blittervis_appendHit(outHits, &hitCount, hitCap, &hit);
            if (hitCount >= hitCap) {
                break;
            }
        }
    }
    return hitCount;
}

static int
amiga_blittervis_abs(int value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

static uint32_t
amiga_blittervis_lineHash(uint32_t blitId, uint32_t y)
{
    uint32_t mixed = (blitId * 2654435761u) ^ (y * 2246822519u);
    return mixed;
}

static int
amiga_blittervis_lineCompare(const void *lhs, const void *rhs)
{
    const emu_ami_blitter_vis_line_stat_t *left = (const emu_ami_blitter_vis_line_stat_t *)lhs;
    const emu_ami_blitter_vis_line_stat_t *right = (const emu_ami_blitter_vis_line_stat_t *)rhs;
    if (left->blitId < right->blitId) {
        return -1;
    }
    if (left->blitId > right->blitId) {
        return 1;
    }
    if (left->y < right->y) {
        return -1;
    }
    if (left->y > right->y) {
        return 1;
    }
    return 0;
}

static uint32_t
amiga_blittervis_blitHash(uint32_t blitId)
{
    return blitId * 2654435761u;
}

static uint8_t
amiga_blittervis_alphaForAge(uint32_t age, uint32_t decayFrames)
{
    if (decayFrames <= 1u) {
        return (uint8_t)EMU_AMI_BLITTER_VIS_ALPHA_MAX;
    }
    if (age >= decayFrames) {
        return 0u;
    }
    uint32_t range = decayFrames - 1u;
    uint32_t alpha = (EMU_AMI_BLITTER_VIS_ALPHA_MAX * (range - age)) / range;
    return (uint8_t)alpha;
}

static __attribute__((unused)) int
amiga_blittervis_ensureBlitFrameTable(emu_ami_blitter_vis_cache_t *cache, size_t neededEntries)
{
    if (!cache) {
        return 0;
    }
    size_t target = neededEntries;
    if (target < (size_t)EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MIN / 2u) {
        target = (size_t)EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MIN / 2u;
    }
    size_t desiredCap = cache->blitFrameCap;
    if (desiredCap < (size_t)EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MIN) {
        desiredCap = (size_t)EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MIN;
    }
    while ((target * 2u) > desiredCap && desiredCap < (size_t)EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MAX) {
        desiredCap <<= 1u;
    }
    if (desiredCap > (size_t)EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MAX) {
        desiredCap = (size_t)EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MAX;
    }
    if (cache->blitFrameCap >= desiredCap && cache->blitFrameIds && cache->blitFrameValues && cache->blitMetaValues) {
        return 1;
    }

    uint32_t *nextIds = (uint32_t *)calloc(desiredCap, sizeof(*nextIds));
    uint32_t *nextValues = (uint32_t *)calloc(desiredCap, sizeof(*nextValues));
    e9k_debug_ami_blitter_vis_point_t *nextMetaValues =
        (e9k_debug_ami_blitter_vis_point_t *)calloc(desiredCap, sizeof(*nextMetaValues));
    if (!nextIds || !nextValues || !nextMetaValues) {
        free(nextIds);
        free(nextValues);
        free(nextMetaValues);
        return 0;
    }

    size_t nextCount = 0;
    if (cache->blitFrameIds && cache->blitFrameValues && cache->blitMetaValues && cache->blitFrameCap) {
        size_t nextMask = desiredCap - 1u;
        for (size_t i = 0; i < cache->blitFrameCap; ++i) {
            uint32_t blitId = cache->blitFrameIds[i];
            if (!blitId) {
                continue;
            }
            size_t index = (size_t)amiga_blittervis_blitHash(blitId) & nextMask;
            for (size_t probe = 0; probe < desiredCap; ++probe) {
                if (!nextIds[index]) {
                    nextIds[index] = blitId;
                    nextValues[index] = cache->blitFrameValues[i];
                    nextMetaValues[index] = cache->blitMetaValues[i];
                    nextCount++;
                    break;
                }
                index = (index + 1u) & nextMask;
            }
        }
    }

    free(cache->blitFrameIds);
    free(cache->blitFrameValues);
    free(cache->blitMetaValues);
    cache->blitFrameIds = nextIds;
    cache->blitFrameValues = nextValues;
    cache->blitMetaValues = nextMetaValues;
    cache->blitFrameCap = desiredCap;
    cache->blitFrameCount = nextCount;
    return 1;
}

static __attribute__((unused)) int
amiga_blittervis_setBlitRecord(emu_ami_blitter_vis_cache_t *cache,
                                const e9k_debug_ami_blitter_vis_point_t *point,
                                uint32_t frameCounter)
{
    uint32_t blitId = 0u;

    if (!cache || !point) {
        return 0;
    }
    blitId = point->blitId;
    if (!blitId) {
        return 0;
    }
    if (!amiga_blittervis_ensureBlitFrameTable(cache, cache->blitFrameCount + 1u)) {
        return 0;
    }
    if ((cache->blitFrameCount * 4u) >= (cache->blitFrameCap * 3u)) {
        if (cache->blitFrameCap >= (size_t)EMU_AMI_BLITTER_VIS_BLIT_TABLE_CAP_MAX ||
            !amiga_blittervis_ensureBlitFrameTable(cache, cache->blitFrameCount + 1u)) {
            return 0;
        }
    }

    size_t mask = cache->blitFrameCap - 1u;
    size_t index = (size_t)amiga_blittervis_blitHash(blitId) & mask;
    for (size_t probe = 0; probe < cache->blitFrameCap; ++probe) {
        if (!cache->blitFrameIds[index]) {
            cache->blitFrameIds[index] = blitId;
            cache->blitFrameValues[index] = frameCounter;
            cache->blitMetaValues[index] = *point;
            cache->blitFrameCount++;
            return 1;
        }
        if (cache->blitFrameIds[index] == blitId) {
            cache->blitFrameValues[index] = frameCounter;
            cache->blitMetaValues[index] = *point;
            return 1;
        }
        index = (index + 1u) & mask;
    }
    return 0;
}

static __attribute__((unused)) int
amiga_blittervis_getBlitFrame(const emu_ami_blitter_vis_cache_t *cache, uint32_t blitId, uint32_t *outFrame)
{
    if (outFrame) {
        *outFrame = 0u;
    }
    if (!cache || !blitId || !cache->blitFrameIds || !cache->blitFrameValues || !cache->blitFrameCap) {
        return 0;
    }
    size_t mask = cache->blitFrameCap - 1u;
    size_t index = (size_t)amiga_blittervis_blitHash(blitId) & mask;
    for (size_t probe = 0; probe < cache->blitFrameCap; ++probe) {
        uint32_t id = cache->blitFrameIds[index];
        if (!id) {
            return 0;
        }
        if (id == blitId) {
            if (outFrame) {
                *outFrame = cache->blitFrameValues[index];
            }
            return 1;
        }
        index = (index + 1u) & mask;
    }
    return 0;
}

static int
amiga_blittervis_getBlitMeta(const emu_ami_blitter_vis_cache_t *cache,
                              uint32_t blitId,
                              e9k_debug_ami_blitter_vis_point_t *outPoint,
                              uint32_t *outFrame)
{
    if (outPoint) {
        memset(outPoint, 0, sizeof(*outPoint));
    }
    if (outFrame) {
        *outFrame = 0u;
    }
    if (!cache || !blitId || !cache->blitFrameIds || !cache->blitFrameValues || !cache->blitMetaValues || !cache->blitFrameCap) {
        return 0;
    }
    size_t mask = cache->blitFrameCap - 1u;
    size_t index = (size_t)amiga_blittervis_blitHash(blitId) & mask;
    for (size_t probe = 0; probe < cache->blitFrameCap; ++probe) {
        uint32_t id = cache->blitFrameIds[index];

        if (!id) {
            return 0;
        }
        if (id == blitId) {
            if (outPoint) {
                *outPoint = cache->blitMetaValues[index];
            }
            if (outFrame) {
                *outFrame = cache->blitFrameValues[index];
            }
            return 1;
        }
        index = (index + 1u) & mask;
    }
    return 0;
}

static size_t
amiga_blittervis_recommendedLineTableCap(size_t fetchedCount)
{
    size_t target = fetchedCount;
    if (target < 1024u) {
        target = 1024u;
    }
    if (target > (size_t)EMU_AMI_BLITTER_VIS_LINE_TABLE_CAP_MAX / 2u) {
        target = (size_t)EMU_AMI_BLITTER_VIS_LINE_TABLE_CAP_MAX / 2u;
    }
    size_t cap = 1024u;
    while (cap < target * 2u && cap < (size_t)EMU_AMI_BLITTER_VIS_LINE_TABLE_CAP_MAX) {
        cap <<= 1;
    }
    if (cap > (size_t)EMU_AMI_BLITTER_VIS_LINE_TABLE_CAP_MAX) {
        cap = (size_t)EMU_AMI_BLITTER_VIS_LINE_TABLE_CAP_MAX;
    }
    return cap;
}

static int
amiga_blittervis_ensureLineStorage(emu_ami_blitter_vis_cache_t *cache, size_t fetchedCount)
{
    if (!cache) {
        return 0;
    }
    size_t desiredCap = amiga_blittervis_recommendedLineTableCap(fetchedCount);
    if (cache->lineTableCap < desiredCap) {
        emu_ami_blitter_vis_line_stat_t *nextTable =
            (emu_ami_blitter_vis_line_stat_t *)realloc(cache->lineTable, desiredCap * sizeof(*nextTable));
        if (!nextTable) {
            return 0;
        }
        cache->lineTable = nextTable;
        cache->lineTableCap = desiredCap;
    }
    if (cache->lineListCap < cache->lineTableCap) {
        emu_ami_blitter_vis_line_stat_t *nextList =
            (emu_ami_blitter_vis_line_stat_t *)realloc(cache->lineList, cache->lineTableCap * sizeof(*nextList));
        if (!nextList) {
            return 0;
        }
        cache->lineList = nextList;
        cache->lineListCap = cache->lineTableCap;
    }
    return 1;
}

static __attribute__((unused)) int
amiga_blittervis_analyzePoints(emu_ami_blitter_vis_cache_t *cache, size_t fetchedCount, emu_ami_blitter_vis_analysis_t *out)
{
    if (!cache || !out) {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->firstWordShiftBlitId = 0u;

    if (!fetchedCount) {
        return 1;
    }
    if (!amiga_blittervis_ensureLineStorage(cache, fetchedCount)) {
        return 0;
    }
    if (!cache->lineTable || !cache->lineList || !cache->lineTableCap) {
        return 0;
    }

    memset(cache->lineTable, 0, cache->lineTableCap * sizeof(*cache->lineTable));
    size_t mask = cache->lineTableCap - 1u;
    for (size_t i = 0; i < fetchedCount; ++i) {
        uint32_t blitId = cache->points[i].blitId;
        if (!blitId) {
            continue;
        }
        uint32_t y = (uint32_t)cache->points[i].y;
        uint32_t x = (uint32_t)cache->points[i].x;
        uint32_t xEnd = (uint32_t)cache->points[i].xEnd;
        if (xEnd < x) {
            xEnd = x;
        }
        uint32_t hash = amiga_blittervis_lineHash(blitId, y);
        size_t index = (size_t)hash & mask;
        emu_ami_blitter_vis_line_stat_t *entry = NULL;
        for (size_t probe = 0; probe < cache->lineTableCap; ++probe) {
            emu_ami_blitter_vis_line_stat_t *candidate = &cache->lineTable[index];
            if (!candidate->used) {
                candidate->used = 1u;
                candidate->blitId = blitId;
                candidate->y = y;
                candidate->minX = x;
                candidate->maxX = xEnd;
                candidate->count = xEnd - x + 1u;
                entry = candidate;
                break;
            }
            if (candidate->blitId == blitId && candidate->y == y) {
                entry = candidate;
                break;
            }
            index = (index + 1u) & mask;
        }
        if (!entry) {
            out->droppedEntries++;
            continue;
        }
        if (entry->count != xEnd - x + 1u || entry->minX != x || entry->maxX != xEnd) {
            if (x < entry->minX) {
                entry->minX = x;
            }
            if (xEnd > entry->maxX) {
                entry->maxX = xEnd;
            }
            entry->count += xEnd - x + 1u;
        }
    }

    size_t emitCount = 0u;
    for (size_t i = 0; i < cache->lineTableCap; ++i) {
        if (!cache->lineTable[i].used) {
            continue;
        }
        cache->lineList[emitCount++] = cache->lineTable[i];
    }
    out->lineCount = (uint32_t)emitCount;
    if (emitCount == 0u) {
        return 1;
    }

    qsort(cache->lineList, emitCount, sizeof(cache->lineList[0]), amiga_blittervis_lineCompare);
    size_t groupStart = 0u;
    while (groupStart < emitCount) {
        uint32_t blitId = cache->lineList[groupStart].blitId;
        uint32_t lowMinX = cache->lineList[groupStart].minX;
        uint32_t highMinX = cache->lineList[groupStart].minX;
        uint32_t lowY = cache->lineList[groupStart].y;
        uint32_t highY = cache->lineList[groupStart].y;
        size_t groupEnd = groupStart + 1u;
        while (groupEnd < emitCount && cache->lineList[groupEnd].blitId == blitId) {
            const emu_ami_blitter_vis_line_stat_t *entry = &cache->lineList[groupEnd];
            if (entry->minX < lowMinX) {
                lowMinX = entry->minX;
                lowY = entry->y;
            }
            if (entry->minX > highMinX) {
                highMinX = entry->minX;
                highY = entry->y;
            }
            groupEnd++;
        }
        if (highMinX > lowMinX) {
            uint32_t spread = highMinX - lowMinX;
            out->blitsWithMinXVariance++;
            if (spread > out->maxMinXSpread) {
                out->maxMinXSpread = spread;
                out->maxMinXSpreadBlitId = blitId;
                out->maxMinXSpreadLowY = lowY;
                out->maxMinXSpreadHighY = highY;
                out->maxMinXSpreadLowMinX = lowMinX;
                out->maxMinXSpreadHighMinX = highMinX;
            }
        }
        groupStart = groupEnd;
    }

    for (size_t i = 1u; i < emitCount; ++i) {
        const emu_ami_blitter_vis_line_stat_t *prev = &cache->lineList[i - 1u];
        const emu_ami_blitter_vis_line_stat_t *curr = &cache->lineList[i];
        if (prev->blitId != curr->blitId) {
            continue;
        }
        if (curr->y <= prev->y) {
            continue;
        }
        uint32_t yGap = curr->y - prev->y;
        if (yGap > EMU_AMI_BLITTER_VIS_Y_GAP_MAX) {
            continue;
        }

        int minDelta = (int)curr->minX - (int)prev->minX;
        int maxDelta = (int)curr->maxX - (int)prev->maxX;
        int pairAbs = amiga_blittervis_abs(minDelta);
        if (amiga_blittervis_abs(maxDelta) > pairAbs) {
            pairAbs = amiga_blittervis_abs(maxDelta);
        }
        out->comparedPairs++;
        if (pairAbs > out->maxAbsDelta) {
            out->maxAbsDelta = pairAbs;
            out->maxAbsDeltaBlitId = curr->blitId;
            out->maxAbsDeltaY = curr->y;
            out->maxAbsDeltaPrevY = prev->y;
            out->maxAbsDeltaMin = minDelta;
            out->maxAbsDeltaMax = maxDelta;
            out->maxPrevMinX = prev->minX;
            out->maxPrevMaxX = prev->maxX;
            out->maxCurrMinX = curr->minX;
            out->maxCurrMaxX = curr->maxX;
        }
        if (amiga_blittervis_abs(minDelta) == EMU_AMI_BLITTER_VIS_WORD_SHIFT_PIXELS &&
            amiga_blittervis_abs(maxDelta) == EMU_AMI_BLITTER_VIS_WORD_SHIFT_PIXELS &&
            ((minDelta < 0 && maxDelta < 0) || (minDelta > 0 && maxDelta > 0))) {
            out->wordShiftSameCount++;
            if (out->firstWordShiftBlitId == 0u) {
                out->firstWordShiftBlitId = curr->blitId;
                out->firstWordShiftY = curr->y;
                out->firstWordShiftPrevY = prev->y;
                out->firstWordShiftMin = minDelta;
                out->firstWordShiftMax = maxDelta;
            }
        }
    }
    return 1;
}

void
amiga_blittervis_renderOverlay(e9ui_context_t *ctx, SDL_Rect *dst)
{
    if (!ctx || !ctx->renderer || !dst || dst->w <= 0 || dst->h <= 0) {
        return;
    }

    int enabled = 0;
    if (!libretro_host_amiga_getBlitterDebug(&enabled) || !enabled) {
        amiga_blittervis_cache.hoveredBlitId = 0u;
        amiga_blittervis_cache.pointCount = 0u;
        return;
    }

    uint32_t srcWidth = 0;
    uint32_t srcHeight = 0;
    if (!amiga_blittervis_cache.pointsCap) {
        amiga_blittervis_cache.pointsCap = EMU_AMI_BLITTER_VIS_POINTS_CAP_DEFAULT;
        amiga_blittervis_cache.points = (e9k_debug_ami_blitter_vis_point_t *)realloc(amiga_blittervis_cache.points,
                                                                                       amiga_blittervis_cache.pointsCap * sizeof(*amiga_blittervis_cache.points));
        if (!amiga_blittervis_cache.points) {
            amiga_blittervis_cache.pointsCap = 0u;
            return;
        }
    }

    size_t fetchedCount = 0u;
    fetchedCount = libretro_host_amiga_readBlitterVisSpans(amiga_blittervis_cache.points,
                                                             amiga_blittervis_cache.pointsCap,
                                                             &srcWidth,
                                                             &srcHeight);
    e9k_debug_ami_blitter_vis_stats_t stats;
    int hasStats = libretro_host_amiga_readBlitterVisStats(&stats) ? 1 : 0;
    if (hasStats) {
        amiga_blittervis_cache.latestStats = stats;
        amiga_blittervis_cache.hasLatestStats = 1;
    } else {
        memset(&amiga_blittervis_cache.latestStats, 0, sizeof(amiga_blittervis_cache.latestStats));
        amiga_blittervis_cache.hasLatestStats = 0;
    }
    uint32_t mode = 0u;
    uint32_t frameCounter = 0u;
    int overlayMode = 1;
    uint32_t decayFrames = 0u;
    if (hasStats) {
        mode = stats.mode;
        frameCounter = stats.frameCounter;
        overlayMode = ((mode & EMU_AMI_BLITTER_VIS_MODE_OVERLAY) != 0u) ? 1 : 0;
    } else {
        frameCounter = ++amiga_blittervis_cache.overlayFrameCounter;
    }
    if (overlayMode) {
        int uiDecay = amiga_custom_ui_getBlitterVisDecay();
        if (uiDecay < 0) {
            uiDecay = 0;
        }
        decayFrames = (uint32_t)uiDecay;
    }
    if (!srcWidth || !srcHeight) {
        amiga_blittervis_cache.hoveredBlitId = 0u;
        amiga_blittervis_cache.pointCount = 0u;
        if (amiga_blittervis_cache.hasRetainedOverlay && amiga_blittervis_cache.texture) {
            SDL_SetTextureBlendMode(amiga_blittervis_cache.texture, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(ctx->renderer, amiga_blittervis_cache.texture, NULL, dst);
        }
        return;
    }

    if (fetchedCount > amiga_blittervis_cache.pointsCap) {
        e9k_debug_ami_blitter_vis_point_t *nextPoints = (e9k_debug_ami_blitter_vis_point_t *)realloc(amiga_blittervis_cache.points,
                                                                                                       fetchedCount * sizeof(*nextPoints));
        if (!nextPoints) {
            return;
        }
        amiga_blittervis_cache.points = nextPoints;
        amiga_blittervis_cache.pointsCap = fetchedCount;
        fetchedCount = libretro_host_amiga_readBlitterVisSpans(amiga_blittervis_cache.points,
                                                                 amiga_blittervis_cache.pointsCap,
                                                                 &srcWidth,
                                                                 &srcHeight);
    }

    if (!srcWidth || !srcHeight) {
        amiga_blittervis_cache.hoveredBlitId = 0u;
        amiga_blittervis_cache.pointCount = 0u;
        if (amiga_blittervis_cache.hasRetainedOverlay && amiga_blittervis_cache.texture) {
            SDL_SetTextureBlendMode(amiga_blittervis_cache.texture, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(ctx->renderer, amiga_blittervis_cache.texture, NULL, dst);
        }
        return;
    }

    int textureWidth = (int)srcWidth;
    int textureHeight = (int)srcHeight;
    if (textureWidth <= 0 || textureHeight <= 0) {
        amiga_blittervis_cache.hoveredBlitId = 0u;
        amiga_blittervis_cache.pointCount = 0u;
        return;
    }

    if (amiga_blittervis_cache.renderer != ctx->renderer) {
        if (amiga_blittervis_cache.texture) {
            SDL_DestroyTexture(amiga_blittervis_cache.texture);
            amiga_blittervis_cache.texture = NULL;
        }
        amiga_blittervis_cache.renderer = ctx->renderer;
        amiga_blittervis_cache.texWidth = 0;
        amiga_blittervis_cache.texHeight = 0;
        amiga_blittervis_cache.hasRetainedOverlay = 0;
    }

    int textureRecreated = 0;
    if (!amiga_blittervis_cache.texture ||
        amiga_blittervis_cache.texWidth != textureWidth ||
        amiga_blittervis_cache.texHeight != textureHeight) {
        if (amiga_blittervis_cache.texture) {
            SDL_DestroyTexture(amiga_blittervis_cache.texture);
            amiga_blittervis_cache.texture = NULL;
        }
        amiga_blittervis_cache.texture = SDL_CreateTexture(ctx->renderer,
                                                            SDL_PIXELFORMAT_ARGB8888,
                                                            SDL_TEXTUREACCESS_STREAMING,
                                                            textureWidth,
                                                            textureHeight);
        if (!amiga_blittervis_cache.texture) {
            return;
        }
        amiga_blittervis_cache.texWidth = textureWidth;
        amiga_blittervis_cache.texHeight = textureHeight;
        amiga_blittervis_cache.hasRetainedOverlay = 0;
        textureRecreated = 1;
    }

    size_t pixelCount = (size_t)textureWidth * (size_t)textureHeight;
    if (pixelCount > amiga_blittervis_cache.pixelsCap) {
        size_t oldPixelsCap = amiga_blittervis_cache.pixelsCap;
        uint32_t *nextPixels = (uint32_t *)realloc(amiga_blittervis_cache.pixels, pixelCount * sizeof(*nextPixels));
        if (!nextPixels) {
            return;
        }
        amiga_blittervis_cache.pixels = nextPixels;
        amiga_blittervis_cache.pixelsCap = pixelCount;
        memset(amiga_blittervis_cache.pixels + oldPixelsCap,
               0,
               (pixelCount - oldPixelsCap) * sizeof(*amiga_blittervis_cache.pixels));
    }

    if (pixelCount > amiga_blittervis_cache.retainedCap) {
        emu_ami_blitter_vis_retained_pixel_t *nextRetainedPixels =
            (emu_ami_blitter_vis_retained_pixel_t *)realloc(amiga_blittervis_cache.retainedPixels, pixelCount * sizeof(*nextRetainedPixels));
        if (!nextRetainedPixels) {
            return;
        }
        amiga_blittervis_cache.retainedPixels = nextRetainedPixels;
        uint32_t *nextRetainedActivePixels =
            (uint32_t *)realloc(amiga_blittervis_cache.retainedActivePixels, pixelCount * sizeof(*nextRetainedActivePixels));
        if (!nextRetainedActivePixels) {
            return;
        }
        amiga_blittervis_cache.retainedActivePixels = nextRetainedActivePixels;
        uint32_t *nextRetainedActiveSlots =
            (uint32_t *)realloc(amiga_blittervis_cache.retainedActiveSlots, pixelCount * sizeof(*nextRetainedActiveSlots));
        if (!nextRetainedActiveSlots) {
            return;
        }
        amiga_blittervis_cache.retainedActiveSlots = nextRetainedActiveSlots;
        size_t oldRetainedCap = amiga_blittervis_cache.retainedCap;
        amiga_blittervis_cache.retainedCap = pixelCount;
        amiga_blittervis_cache.retainedActiveCap = pixelCount;
        if (pixelCount > oldRetainedCap) {
            memset(amiga_blittervis_cache.retainedPixels + oldRetainedCap,
                   0,
                   (pixelCount - oldRetainedCap) * sizeof(*amiga_blittervis_cache.retainedPixels));
            memset(amiga_blittervis_cache.retainedActiveSlots + oldRetainedCap,
                   0,
                   (pixelCount - oldRetainedCap) * sizeof(*amiga_blittervis_cache.retainedActiveSlots));
        }
    }

    if (textureRecreated &&
        amiga_blittervis_cache.retainedPixels &&
        amiga_blittervis_cache.retainedCap >= pixelCount) {
        memset(amiga_blittervis_cache.pixels, 0, pixelCount * sizeof(*amiga_blittervis_cache.pixels));
        memset(amiga_blittervis_cache.retainedPixels, 0, pixelCount * sizeof(*amiga_blittervis_cache.retainedPixels));
        if (amiga_blittervis_cache.retainedActiveSlots) {
            memset(amiga_blittervis_cache.retainedActiveSlots, 0, pixelCount * sizeof(*amiga_blittervis_cache.retainedActiveSlots));
        }
        amiga_blittervis_cache.retainedActiveCount = 0u;
        if (amiga_blittervis_cache.blitFrameIds && amiga_blittervis_cache.blitFrameCap) {
            memset(amiga_blittervis_cache.blitFrameIds, 0, amiga_blittervis_cache.blitFrameCap * sizeof(*amiga_blittervis_cache.blitFrameIds));
        }
        if (amiga_blittervis_cache.blitMetaValues && amiga_blittervis_cache.blitFrameCap) {
            memset(amiga_blittervis_cache.blitMetaValues, 0, amiga_blittervis_cache.blitFrameCap * sizeof(*amiga_blittervis_cache.blitMetaValues));
        }
        amiga_blittervis_cache.blitFrameCount = 0u;
    }

    int hasVisiblePixels = 0;
    if (overlayMode &&
        amiga_blittervis_cache.retainedPixels &&
        amiga_blittervis_cache.retainedActivePixels &&
        amiga_blittervis_cache.retainedActiveSlots &&
        amiga_blittervis_cache.retainedCap >= pixelCount) {
        uint32_t lastScatterBlitId = 0u;
        uint32_t lastScatterRgb = 0u;
        for (size_t i = 0; i < fetchedCount; i++) {
            uint32_t x = amiga_blittervis_cache.points[i].x;
            uint32_t y = amiga_blittervis_cache.points[i].y;
            uint32_t xEnd = amiga_blittervis_cache.points[i].xEnd;
            if (xEnd < x) {
                xEnd = x;
            }
            if (x >= srcWidth || y >= srcHeight) {
                continue;
            }
            if (xEnd >= srcWidth) {
                xEnd = srcWidth - 1u;
            }
            uint32_t blitId = amiga_blittervis_cache.points[i].blitId;
            if (!blitId) {
                continue;
            }
            (void)amiga_blittervis_setBlitRecord(&amiga_blittervis_cache, &amiga_blittervis_cache.points[i], frameCounter);
            uint32_t rgb = lastScatterRgb;
            if (blitId != lastScatterBlitId) {
                rgb = amiga_blittervis_colorFromId(blitId) & 0x00ffffffu;
                lastScatterBlitId = blitId;
                lastScatterRgb = rgb;
            }
            uint32_t sourceInfo = amiga_blittervis_retainedPackSource(amiga_blittervis_cache.points[i].sourceAddr,
                                                                       amiga_blittervis_cache.points[i].sourceIsCopper ? 1 : 0);
            for (uint32_t spanX = x; spanX <= xEnd; ++spanX) {
                size_t pixelIndex = (size_t)y * (size_t)srcWidth + (size_t)spanX;
                emu_ami_blitter_vis_retained_pixel_t *retainedPixel = &amiga_blittervis_cache.retainedPixels[pixelIndex];
                amiga_blittervis_retainedActivate(pixelIndex);
                amiga_blittervis_retainedRememberBlit(retainedPixel, blitId);
                retainedPixel->blitId = blitId;
                retainedPixel->frame = frameCounter;
                retainedPixel->rgb = rgb;
                retainedPixel->sourceInfo = sourceInfo;
                retainedPixel->sourceDataAddr = amiga_blittervis_cache.points[i].sourceDataAddr;
                retainedPixel->channelAAddr = amiga_blittervis_cache.points[i].channelAAddr;
                retainedPixel->channelBAddr = amiga_blittervis_cache.points[i].channelBAddr;
                retainedPixel->channelCAddr = amiga_blittervis_cache.points[i].channelCAddr;
                retainedPixel->channelDAddr = amiga_blittervis_cache.points[i].channelDAddr;
                retainedPixel->channelAModulo = amiga_blittervis_cache.points[i].channelAModulo;
                retainedPixel->channelBModulo = amiga_blittervis_cache.points[i].channelBModulo;
                retainedPixel->channelCModulo = amiga_blittervis_cache.points[i].channelCModulo;
                retainedPixel->channelDModulo = amiga_blittervis_cache.points[i].channelDModulo;
                retainedPixel->widthWords = amiga_blittervis_cache.points[i].widthWords;
                retainedPixel->heightLines = amiga_blittervis_cache.points[i].heightLines;
                retainedPixel->sourceRowBytes = amiga_blittervis_cache.points[i].sourceRowBytes;
                retainedPixel->sourceModulo = amiga_blittervis_cache.points[i].sourceModulo;
                retainedPixel->sourceChannelsMask = amiga_blittervis_cache.points[i].sourceChannelsMask;
                retainedPixel->minterm = amiga_blittervis_cache.points[i].minterm;
                retainedPixel->sourceDescending = amiga_blittervis_cache.points[i].sourceDescending;
                retainedPixel->lineMode = amiga_blittervis_cache.points[i].lineMode;
            }
        }
        size_t activeIndex = 0u;
        while (activeIndex < amiga_blittervis_cache.retainedActiveCount) {
            size_t pixelIndex = (size_t)amiga_blittervis_cache.retainedActivePixels[activeIndex];
            emu_ami_blitter_vis_retained_pixel_t *retainedPixel = &amiga_blittervis_cache.retainedPixels[pixelIndex];
            if (!retainedPixel->blitId) {
                amiga_blittervis_cache.pixels[pixelIndex] = 0u;
                amiga_blittervis_retainedRemove(pixelIndex);
                continue;
            }
            uint32_t blitFrame = retainedPixel->frame;
            uint32_t age = frameCounter - blitFrame;
            if (decayFrames > 0u && age < decayFrames) {
                uint32_t rgb = retainedPixel->rgb;
                uint8_t alpha = amiga_blittervis_alphaForAge(age, decayFrames);
                amiga_blittervis_cache.pixels[pixelIndex] = ((uint32_t)alpha << 24) | rgb;
                hasVisiblePixels = 1;
                activeIndex++;
            } else {
                amiga_blittervis_cache.pixels[pixelIndex] = 0u;
                memset(retainedPixel, 0, sizeof(*retainedPixel));
                amiga_blittervis_retainedRemove(pixelIndex);
            }
        }
    } else {
        uint32_t lastScatterBlitId = 0u;
        uint32_t lastScatterColor = 0u;
        memset(amiga_blittervis_cache.pixels, 0, pixelCount * sizeof(*amiga_blittervis_cache.pixels));
        for (size_t i = 0; i < fetchedCount; i++) {
            uint32_t x = amiga_blittervis_cache.points[i].x;
            uint32_t y = amiga_blittervis_cache.points[i].y;
            uint32_t xEnd = amiga_blittervis_cache.points[i].xEnd;
            if (xEnd < x) {
                xEnd = x;
            }
            if (x >= srcWidth || y >= srcHeight) {
                continue;
            }
            if (xEnd >= srcWidth) {
                xEnd = srcWidth - 1u;
            }
            uint32_t blitId = amiga_blittervis_cache.points[i].blitId;
            uint32_t color = lastScatterColor;
            if (blitId != lastScatterBlitId) {
                color = amiga_blittervis_colorFromId(blitId);
                lastScatterBlitId = blitId;
                lastScatterColor = color;
            }
            for (uint32_t spanX = x; spanX <= xEnd; ++spanX) {
                amiga_blittervis_cache.pixels[(size_t)y * (size_t)srcWidth + (size_t)spanX] = color;
            }
            hasVisiblePixels = 1;
        }
    }
    amiga_blittervis_cache.pointCount = fetchedCount;
    SDL_UpdateTexture(amiga_blittervis_cache.texture,
                      NULL,
                      amiga_blittervis_cache.pixels,
                      textureWidth * (int)sizeof(*amiga_blittervis_cache.pixels));
    amiga_blittervis_cache.hasRetainedOverlay = hasVisiblePixels ? 1 : 0;
    amiga_blittervis_cache.hoveredBlitId =
        amiga_blittervis_hoveredIdAtPoint(dst, ctx->mouseX, ctx->mouseY, srcWidth, srcHeight, overlayMode, fetchedCount);
    SDL_SetTextureBlendMode(amiga_blittervis_cache.texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(ctx->renderer, amiga_blittervis_cache.texture, NULL, dst);
}

int
amiga_blittervis_getLatestStats(e9k_debug_ami_blitter_vis_stats_t *outStats)
{
    if (!outStats) {
        return 0;
    }
    memset(outStats, 0, sizeof(*outStats));
    if (!amiga_blittervis_cache.hasLatestStats) {
        return 0;
    }
    *outStats = amiga_blittervis_cache.latestStats;
    return 1;
}

uint32_t
amiga_blittervis_getHoveredBlitId(void)
{
    return amiga_blittervis_cache.hoveredBlitId;
}

uint32_t
amiga_blittervis_getColor(uint32_t blitId)
{
    return amiga_blittervis_colorFromId(blitId);
}

void
amiga_blittervis_destroy(void)
{
    if (amiga_blittervis_cache.texture) {
        SDL_DestroyTexture(amiga_blittervis_cache.texture);
        amiga_blittervis_cache.texture = NULL;
    }
    free(amiga_blittervis_cache.pixels);
    free(amiga_blittervis_cache.retainedPixels);
    free(amiga_blittervis_cache.retainedActivePixels);
    free(amiga_blittervis_cache.retainedActiveSlots);
    free(amiga_blittervis_cache.blitFrameIds);
    free(amiga_blittervis_cache.blitFrameValues);
    free(amiga_blittervis_cache.blitMetaValues);
    free(amiga_blittervis_cache.points);
    free(amiga_blittervis_cache.lineTable);
    free(amiga_blittervis_cache.lineList);
    memset(&amiga_blittervis_cache, 0, sizeof(amiga_blittervis_cache));
}

int
amiga_blittervis_handleOverlayEvent(e9ui_context_t *ctx, const SDL_Rect *dst, const e9ui_event_t *ev)
{
    emu_ami_blitter_vis_hit_t blitHits[EMU_AMI_BLITTER_VIS_MAX_HITS_AT_POINT] = {0};
    e9k_debug_ami_blitter_vis_point_t blitInfos[EMU_AMI_BLITTER_VIS_MAX_HITS_AT_POINT] = {0};
    size_t blitHitCount = 0u;
    uint32_t srcWidth = 0u;
    uint32_t srcHeight = 0u;
    uint32_t frameCounter = 0u;
    uint32_t decayFrames = 0u;
    int overlayMode = 1;

    (void)ctx;
    if (!dst || !ev) {
        return 0;
    }
    if (ev->type != SDL_MOUSEBUTTONDOWN && ev->type != SDL_MOUSEBUTTONUP) {
        return 0;
    }
    if (ev->button.button != SDL_BUTTON_LEFT) {
        return 0;
    }

    if (amiga_blittervis_cache.texWidth <= 0 || amiga_blittervis_cache.texHeight <= 0) {
        return 0;
    }

    srcWidth = (uint32_t)amiga_blittervis_cache.texWidth;
    srcHeight = (uint32_t)amiga_blittervis_cache.texHeight;
    if (amiga_blittervis_cache.hasLatestStats) {
        overlayMode = ((amiga_blittervis_cache.latestStats.mode & EMU_AMI_BLITTER_VIS_MODE_OVERLAY) != 0u) ? 1 : 0;
        frameCounter = amiga_blittervis_cache.latestStats.frameCounter;
    } else {
        frameCounter = amiga_blittervis_cache.overlayFrameCounter;
    }
    if (overlayMode) {
        int uiDecay = amiga_custom_ui_getBlitterVisDecay();

        if (uiDecay < 0) {
            uiDecay = 0;
        }
        decayFrames = (uint32_t)uiDecay;
    }

    blitHitCount = amiga_blittervis_hitsAtPoint(dst,
                                                ev->button.x,
                                                ev->button.y,
                                                srcWidth,
                                                srcHeight,
                                                overlayMode,
                                                frameCounter,
                                                decayFrames,
                                                amiga_blittervis_cache.pointCount,
                                                blitHits,
                                                EMU_AMI_BLITTER_VIS_MAX_HITS_AT_POINT);
    if (blitHitCount == 0u) {
        return 0;
    }
    if (ev->type == SDL_MOUSEBUTTONDOWN) {
        for (size_t i = 0u; i < blitHitCount; ++i) {
            amiga_blittervis_copyPointFromHit(&blitInfos[i], &blitHits[i]);
        }
        amiga_blit_info_showHits(blitInfos, blitHitCount, 0u);
    }
    return 1;
}
