/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <stdint.h>
#include <string.h>
#include <SDL.h>

#include "alloc.h"
#include "e9ui.h"
#include "libretro_host.h"
#include "mega_memview_internal.h"

static void
mega_memview_vramOverviewWindow(mega_memview_state_t *ui,
                                  uint32_t *outStartRow,
                                  uint32_t *outVisibleRows)
{
    uint32_t totalRows = mega_memview_vramTotalRows(ui);
    uint32_t zoomLevel = (uint32_t)mega_memview_clampOverviewZoomLevel(ui->overviewZoomLevel);
    uint32_t visibleRows = 0u;
    uint32_t currentTile = 0u;
    uint32_t currentRow = 0u;
    uint32_t startRow = 0u;
    uint32_t marginRows = 0u;

    if (outStartRow) {
        *outStartRow = 0u;
    }
    if (outVisibleRows) {
        *outVisibleRows = totalRows;
    }
    if (totalRows == 0u) {
        return;
    }
    visibleRows = (totalRows + zoomLevel - 1u) / zoomLevel;
    if (visibleRows < 1u) {
        visibleRows = 1u;
    }
    if (visibleRows > totalRows) {
        visibleRows = totalRows;
    }
    currentTile = ui->vramBaseAddr / MEGA_MEMVIEW_TILE_BYTES;
    currentRow = currentTile / mega_memview_clampVramTilesPerRow(ui->vramTilesPerRow);
    marginRows = visibleRows / 8u;
    if (marginRows < 1u) {
        marginRows = 1u;
    }
    if (marginRows * 2u >= visibleRows) {
        marginRows = visibleRows > 1u ? (visibleRows - 1u) / 2u : 0u;
    }
    startRow = ui->overviewWindowVisibleRows == visibleRows ? ui->overviewWindowStartRow : 0u;
    if (startRow + visibleRows > totalRows) {
        startRow = totalRows - visibleRows;
    }
    if (currentRow < startRow + marginRows) {
        if (currentRow > marginRows) {
            startRow = currentRow - marginRows;
        } else {
            startRow = 0u;
        }
    } else if (currentRow >= startRow + visibleRows - marginRows) {
        startRow = currentRow + marginRows + 1u > visibleRows ? currentRow + marginRows + 1u - visibleRows : 0u;
    }
    if (startRow + visibleRows > totalRows) {
        startRow = totalRows - visibleRows;
    }
    ui->overviewWindowStartRow = startRow;
    ui->overviewWindowVisibleRows = visibleRows;
    if (outStartRow) {
        *outStartRow = startRow;
    }
    if (outVisibleRows) {
        *outVisibleRows = visibleRows;
    }
}

static uint64_t
mega_memview_hashMemoryRange(const mega_memview_state_t *ui,
                               uint32_t startAddr,
                               uint32_t endAddr,
                               uint64_t hash,
                               int readRange)
{
    uint8_t chunk[256];

    for (uint32_t addr = startAddr; addr <= endAddr; addr += (uint32_t)sizeof(chunk)) {
        size_t toRead = sizeof(chunk);

        if (addr + toRead - 1u > endAddr) {
            toRead = (size_t)(endAddr - addr + 1u);
        }
        memset(chunk, 0, sizeof(chunk));
        if (readRange) {
            (void)mega_memview_readRange(ui, addr, chunk, toRead);
        } else {
            (void)libretro_host_debugReadMemory(addr, chunk, toRead);
        }
        for (size_t i = 0; i < toRead; ++i) {
            hash ^= (uint64_t)chunk[i];
            hash *= MEGA_MEMVIEW_FNV1A64_PRIME;
        }
        if (toRead < sizeof(chunk)) {
            break;
        }
    }
    return hash;
}

static uint64_t
mega_memview_overviewContentToken(const mega_memview_state_t *ui)
{
    uint64_t hash = MEGA_MEMVIEW_FNV1A64_OFFSET_BASIS;

    if (ui->mode == mega_memview_mode_vram) {
        size_t vramSize = 0u;
        const uint8_t *vram = NULL;

        vram = (const uint8_t *)libretro_host_getMemory(RETRO_MEMORY_VIDEO_RAM, &vramSize);
        if (!vram) {
            return 0u;
        }
        hash ^= (uint64_t)vramSize;
        hash *= MEGA_MEMVIEW_FNV1A64_PRIME;
        for (size_t i = 0u; i < vramSize; ++i) {
            hash ^= (uint64_t)vram[i];
            hash *= MEGA_MEMVIEW_FNV1A64_PRIME;
        }
        return hash;
    } else if (ui->mode == mega_memview_mode_zram) {
        return mega_memview_hashMemoryRange(ui, MEGA_MEMVIEW_ZRAM_BASE_MIN, MEGA_MEMVIEW_ZRAM_BASE_MAX, hash, 1);
    } else {
        return mega_memview_hashMemoryRange(ui, MEGA_MEMVIEW_RAM_BASE_MIN, MEGA_MEMVIEW_RAM_BASE_MAX, hash, 0);
    }
}

static int
mega_memview_measureOverviewGutterPx(const e9ui_context_t *ctx)
{
    e9ui_context_t tempCtx = ctx ? *ctx : (e9ui ? e9ui->ctx : (e9ui_context_t){ 0 });
    return e9ui_scale_px(&tempCtx, MEGA_MEMVIEW_OVERVIEW_GUTTER_PX);
}

static int
mega_memview_ensureOverviewTileMap(mega_memview_state_t *ui, uint32_t vramTiles, uint32_t tilesPerRow)
{
    uint32_t *newRows = NULL;
    uint32_t *newCols = NULL;

    if (!ui || vramTiles == 0u || tilesPerRow == 0u) {
        return 0;
    }
    if (ui->overviewTileRows &&
        ui->overviewTileCols &&
        ui->overviewTileMapCap >= vramTiles &&
        ui->overviewTileMapVramTiles == vramTiles &&
        ui->overviewTileMapTilesPerRow == tilesPerRow) {
        return 1;
    }
    if (ui->overviewTileMapCap < vramTiles) {
        newRows = (uint32_t *)realloc(ui->overviewTileRows, (size_t)vramTiles * sizeof(*ui->overviewTileRows));
        if (!newRows) {
            return 0;
        }
        ui->overviewTileRows = newRows;
        newCols = (uint32_t *)realloc(ui->overviewTileCols, (size_t)vramTiles * sizeof(*ui->overviewTileCols));
        if (!newCols) {
            return 0;
        }
        ui->overviewTileCols = newCols;
        ui->overviewTileMapCap = vramTiles;
    }
    for (uint32_t tile = 0u; tile < vramTiles; ++tile) {
        ui->overviewTileRows[tile] = tile / tilesPerRow;
        ui->overviewTileCols[tile] = tile % tilesPerRow;
    }
    ui->overviewTileMapVramTiles = vramTiles;
    ui->overviewTileMapTilesPerRow = tilesPerRow;
    return 1;
}

static uint16_t
mega_memview_readOverviewVramWord(const uint8_t *vram, size_t vramSize, uint32_t wordOffset)
{
    size_t byteOffset = (size_t)wordOffset * 2u;

    if (!vram || byteOffset + 1u >= vramSize) {
        return 0u;
    }
    return (uint16_t)vram[byteOffset] | (uint16_t)((uint16_t)vram[byteOffset + 1u] << 8);
}

static uint32_t
mega_memview_blendOverviewColor(uint32_t dst, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
    uint32_t invAlpha = 255u - (uint32_t)alpha;
    uint8_t dstR = (uint8_t)((dst >> 16) & 0xffu);
    uint8_t dstG = (uint8_t)((dst >> 8) & 0xffu);
    uint8_t dstB = (uint8_t)(dst & 0xffu);

    return 0xff000000u |
           ((((uint32_t)r * (uint32_t)alpha + (uint32_t)dstR * invAlpha) / 255u) << 16) |
           ((((uint32_t)g * (uint32_t)alpha + (uint32_t)dstG * invAlpha) / 255u) << 8) |
           (((uint32_t)b * (uint32_t)alpha + (uint32_t)dstB * invAlpha) / 255u);
}

static void
mega_memview_markOverviewPaletteTile(mega_memview_state_t *ui,
                                       const e9ui_rect_t *contentBounds,
                                       uint32_t overviewStartRow,
                                       uint32_t overviewVisibleRows,
                                       const int *colX,
                                       const int *rowY,
                                       uint32_t spriteTileNum,
                                       uint32_t vramTiles)
{
    uint32_t tileRow = 0u;
    uint32_t tileCol = 0u;
    uint32_t tilesPerRow = 0u;
    uint32_t localRow = 0u;
    int x0 = 0;
    int x1 = 0;
    int y0 = 0;
    int y1 = 0;

    if (!ui || !contentBounds || !colX || !rowY || vramTiles == 0u || overviewVisibleRows == 0u) {
        return;
    }
    spriteTileNum %= vramTiles;
    tileRow = ui->overviewTileRows[spriteTileNum];
    tileCol = ui->overviewTileCols[spriteTileNum];
    tilesPerRow = ui->overviewTileMapTilesPerRow;
    if (tilesPerRow == 0u || tileRow < overviewStartRow || tileRow >= overviewStartRow + overviewVisibleRows) {
        return;
    }

    localRow = tileRow - overviewStartRow;
    x0 = colX[tileCol];
    x1 = (int)(((uint64_t)(tileCol + 1u) * (uint64_t)contentBounds->w) / (uint64_t)tilesPerRow);
    y0 = rowY[localRow];
    y1 = (int)(((uint64_t)(localRow + 1u) * (uint64_t)contentBounds->h) / (uint64_t)overviewVisibleRows);
    if (x1 <= x0) {
        x1 = x0 + 1;
    }
    if (y1 <= y0) {
        y1 = y0 + 1;
    }
    if (x1 > contentBounds->w) {
        x1 = contentBounds->w;
    }
    if (y1 > contentBounds->h) {
        y1 = contentBounds->h;
    }
    for (int yy = y0; yy < y1; ++yy) {
        for (int xx = x0; xx < x1; ++xx) {
            uint32_t *pixel = &ui->overviewPixels[(size_t)yy * (size_t)contentBounds->w + (size_t)xx];

            *pixel = mega_memview_blendOverviewColor(*pixel, 90u, 220u, 140u, 88u);
        }
    }
}

static void
mega_memview_markOverviewPlaneTiles(mega_memview_state_t *ui,
                                      const e9k_debug_mega_sprite_state_t *videoState,
                                      const e9ui_rect_t *contentBounds,
                                      uint32_t overviewStartRow,
                                      uint32_t overviewVisibleRows,
                                      const int *colX,
                                      const int *rowY,
                                      uint32_t vramTiles,
                                      int plane)
{
    size_t vramSize = 0u;
    const uint8_t *vram = NULL;
    mega_memview_plane_table_t table;

    if (!ui || !videoState || vramTiles == 0u) {
        return;
    }
    vram = (const uint8_t *)libretro_host_getMemory(RETRO_MEMORY_VIDEO_RAM, &vramSize);
    if (!vram) {
        return;
    }
    if (!mega_memview_vdpPlaneTable(videoState, plane, &table)) {
        return;
    }
    for (uint32_t row = 0u; row < table.rows; ++row) {
        for (uint32_t col = 0u; col < table.cols; ++col) {
            uint16_t name = mega_memview_readOverviewVramWord(vram, vramSize, table.wordOffset + row * table.cols + col);

            mega_memview_markOverviewPaletteTile(ui,
                                                  contentBounds,
                                                  overviewStartRow,
                                                  overviewVisibleRows,
                                                  colX,
                                                  rowY,
                                                  (uint32_t)(name & 0x07ffu),
                                                  vramTiles);
        }
    }
}

static void
mega_memview_renderVramOverviewBackground(mega_memview_state_t *ui, const e9ui_rect_t *contentBounds)
{
    size_t vramSize = 0u;
    const uint8_t *vram = NULL;
    uint32_t vramTiles = mega_memview_vramSize() / MEGA_MEMVIEW_TILE_BYTES;
    uint32_t tilesPerRow = mega_memview_clampVramTilesPerRow(ui->vramTilesPerRow);
    uint32_t totalRows = mega_memview_vramTotalRows(ui);
    uint32_t overviewStartRow = 0u;
    uint32_t overviewVisibleRows = totalRows;

    vram = (const uint8_t *)libretro_host_getMemory(RETRO_MEMORY_VIDEO_RAM, &vramSize);
    if (!vram || vramSize < MEGA_MEMVIEW_TILE_BYTES || vramTiles == 0u || tilesPerRow == 0u || totalRows == 0u) {
        return;
    }

    mega_memview_vramOverviewWindow(ui, &overviewStartRow, &overviewVisibleRows);
    for (uint32_t tileRow = overviewStartRow; tileRow < overviewStartRow + overviewVisibleRows; ++tileRow) {
        uint32_t localRow = tileRow - overviewStartRow;
        int y0 = (int)(((uint64_t)localRow * (uint64_t)contentBounds->h) / (uint64_t)overviewVisibleRows);
        int y1 = (int)(((uint64_t)(localRow + 1u) * (uint64_t)contentBounds->h) / (uint64_t)overviewVisibleRows);

        if (y1 <= y0) {
            y1 = y0 + 1;
        }
        if (y0 >= contentBounds->h) {
            break;
        }
        if (y1 > contentBounds->h) {
            y1 = contentBounds->h;
        }
        for (uint32_t tileCol = 0; tileCol < tilesPerRow; ++tileCol) {
            uint32_t tileNum = tileRow * tilesPerRow + tileCol;
            uint32_t tileBase = 0u;
            uint64_t pixelSum = 0u;
            uint64_t pixelCount = 0u;
            uint32_t brightness = 0u;
            int x0 = (int)(((uint64_t)tileCol * (uint64_t)contentBounds->w) / (uint64_t)tilesPerRow);
            int x1 = (int)(((uint64_t)(tileCol + 1u) * (uint64_t)contentBounds->w) / (uint64_t)tilesPerRow);
            uint32_t color = 0u;

            if (tileNum >= vramTiles) {
                continue;
            }
            if (x1 <= x0) {
                x1 = x0 + 1;
            }
            if (x0 >= contentBounds->w) {
                continue;
            }
            if (x1 > contentBounds->w) {
                x1 = contentBounds->w;
            }
            tileBase = tileNum * MEGA_MEMVIEW_TILE_BYTES;
            for (unsigned py = 0; py < MEGA_MEMVIEW_TILE_H; py += 2u) {
                for (unsigned px = 0; px < MEGA_MEMVIEW_TILE_W; px += 2u) {
                    pixelSum += mega_memview_readVramPixel(vram, vramSize, tileBase, px, py);
                    ++pixelCount;
                }
            }
            if (pixelCount > 0u) {
                brightness = (uint32_t)(pixelSum / pixelCount);
            }
            if (brightness > 15u) {
                brightness = 15u;
            }
            color = mega_memview_amberColor(brightness);
            for (int yy = y0; yy < y1; ++yy) {
                for (int xx = x0; xx < x1; ++xx) {
                    ui->overviewBackgroundPixels[(size_t)yy * (size_t)contentBounds->w + (size_t)xx] = color;
                }
            }
        }
    }
}

static void
mega_memview_renderRamOverviewBackground(mega_memview_state_t *ui, const e9ui_rect_t *contentBounds)
{
    uint32_t sample[256];
    mega_memview_overview_range_t *range = &ui->overviewRanges[0];
    uint32_t rowBytes = mega_memview_clampRamRowBytes(ui->ramRowBytes);
    uint32_t totalRows = rowBytes > 0u ? (range->sizeBytes + rowBytes - 1u) / rowBytes : 0u;
    const uint8_t offR = 33u;
    const uint8_t offG = 18u;
    const uint8_t offB = 0u;
    const uint8_t onR = 255u;
    const uint8_t onG = 189u;
    const uint8_t onB = 115u;

    if (range->sizeBytes == 0u || rowBytes == 0u || totalRows == 0u) {
        return;
    }

    for (int yy = 0; yy < contentBounds->h; ++yy) {
        uint32_t sourceRow = (uint32_t)(((uint64_t)yy * (uint64_t)totalRows) / (uint64_t)contentBounds->h);
        uint32_t rowBaseAddr = 0u;
        uint32_t readableBytes = rowBytes;
        const uint8_t *sampleBytes = (const uint8_t *)sample;

        if (sourceRow >= totalRows) {
            sourceRow = totalRows - 1u;
        }
        rowBaseAddr = range->baseAddr + sourceRow * rowBytes;
        if (rowBaseAddr < range->baseAddr || rowBaseAddr >= range->baseAddr + range->sizeBytes) {
            continue;
        }
        if (rowBaseAddr + readableBytes > range->baseAddr + range->sizeBytes) {
            readableBytes = range->baseAddr + range->sizeBytes - rowBaseAddr;
        }

        memset(sample, 0, sizeof(sample));
        (void)mega_memview_readRange(ui, rowBaseAddr, sample, readableBytes);
        for (int xx = 0; xx < contentBounds->w; ++xx) {
            uint32_t rowBits = rowBytes * 8u;
            uint32_t startBit = (uint32_t)(((uint64_t)xx * (uint64_t)rowBits) / (uint64_t)contentBounds->w);
            uint32_t endBit = (uint32_t)((((uint64_t)xx + 1u) * (uint64_t)rowBits) / (uint64_t)contentBounds->w);
            uint32_t setBits = 0u;
            uint32_t sampleBits = 0u;
            uint32_t intensity = 0u;
            uint8_t r = 0u;
            uint8_t g = 0u;
            uint8_t b = 0u;
            uint32_t color = 0u;

            if (endBit <= startBit) {
                endBit = startBit + 1u;
            }
            if (endBit > rowBits) {
                endBit = rowBits;
            }
            for (uint32_t bitIndex = startBit; bitIndex < endBit; ++bitIndex) {
                uint32_t byteIndex = bitIndex >> 3;
                uint32_t bitInByte = bitIndex & 7u;

                if (byteIndex >= readableBytes) {
                    continue;
                }
                if (sampleBytes[byteIndex] & (uint8_t)(0x80u >> bitInByte)) {
                    setBits++;
                }
                sampleBits++;
            }
            if (sampleBits == 0u) {
                sampleBits = 1u;
            }
            intensity = (setBits * 255u) / sampleBits;
            r = (uint8_t)(offR + (((uint32_t)(onR - offR) * intensity) / 255u));
            g = (uint8_t)(offG + (((uint32_t)(onG - offG) * intensity) / 255u));
            b = (uint8_t)(offB + (((uint32_t)(onB - offB) * intensity) / 255u));
            color = mega_memview_argb(r, g, b);
            ui->overviewBackgroundPixels[(size_t)yy * (size_t)contentBounds->w + (size_t)xx] = color;
        }
    }
}

void
mega_memview_initOverviewRanges(mega_memview_state_t *ui)
{
    uint32_t vramSize = 0u;

    memset(ui->overviewRanges, 0, sizeof(ui->overviewRanges));
    if (ui->mode == mega_memview_mode_roms) {
        return;
    }
    if (ui->mode == mega_memview_mode_vram) {
        vramSize = mega_memview_vramSize();
        if (vramSize > 0u) {
            ui->overviewRanges[0].baseAddr = 0u;
            ui->overviewRanges[0].sizeBytes = vramSize;
        }
        return;
    }
    if (ui->mode == mega_memview_mode_zram) {
        ui->overviewRanges[0].baseAddr = MEGA_MEMVIEW_ZRAM_BASE_MIN;
        ui->overviewRanges[0].sizeBytes = MEGA_MEMVIEW_ZRAM_BASE_MAX - MEGA_MEMVIEW_ZRAM_BASE_MIN + 1u;
        return;
    }
    ui->overviewRanges[0].baseAddr = MEGA_MEMVIEW_RAM_BASE_MIN;
    ui->overviewRanges[0].sizeBytes = MEGA_MEMVIEW_RAM_BASE_MAX - MEGA_MEMVIEW_RAM_BASE_MIN + 1u;
}

int
mega_memview_overviewRangeCount(const mega_memview_state_t *ui)
{
    int count = 0;

    for (int i = 0; i < (int)(sizeof(ui->overviewRanges) / sizeof(ui->overviewRanges[0])); ++i) {
        if (ui->overviewRanges[i].sizeBytes != 0u) {
            count++;
        }
    }
    return count;
}

e9ui_rect_t
mega_memview_overviewBounds(const mega_memview_state_t *ui, e9ui_component_t *self, const e9ui_context_t *ctx, TTF_Font *font)
{
    e9ui_rect_t bounds = { 0, 0, 0, 0 };
    int x = 0;

    if (!self || !ctx || !ui->showOverviewColumn || mega_memview_overviewRangeCount(ui) <= 0) {
        return bounds;
    }
    x = self->bounds.x;
    if (ui->showAddressColumn) {
        x += mega_memview_measureAddressGutterPx(ctx, font);
        x += e9ui_scale_px(ctx, MEGA_MEMVIEW_GUTTER_GAP_PX);
    }
    bounds.x = x;
    bounds.y = self->bounds.y + e9ui_scale_px(ctx, MEGA_MEMVIEW_TOP_PAD_PX);
    bounds.w = mega_memview_measureOverviewGutterPx(ctx);
    bounds.h = self->bounds.h - e9ui_scale_px(ctx, MEGA_MEMVIEW_TOP_PAD_PX) -
               e9ui_scale_px(ctx, MEGA_MEMVIEW_BOTTOM_PAD_PX);
    if (bounds.h < 1) {
        bounds.h = 1;
    }
    return bounds;
}

e9ui_rect_t
mega_memview_overviewContentBounds(const e9ui_context_t *ctx, const e9ui_rect_t *overviewBounds)
{
    e9ui_rect_t bounds = { 0, 0, 0, 0 };
    int inset = 0;

    if (!ctx || !overviewBounds || overviewBounds->w <= 0 || overviewBounds->h <= 0) {
        return bounds;
    }
    inset = e9ui_scale_px(ctx, 4);
    bounds = *overviewBounds;
    bounds.x += inset;
    bounds.y += inset;
    bounds.w -= inset * 2;
    bounds.h -= inset * 2;
    if (bounds.w < 1) {
        bounds.w = 1;
    }
    if (bounds.h < 1) {
        bounds.h = 1;
    }
    return bounds;
}

int
mega_memview_rebuildOverviewBackgroundTexture(mega_memview_state_t *ui, e9ui_context_t *ctx)
{
    e9ui_rect_t overviewBounds;
    e9ui_rect_t contentBounds;
    uint32_t cacheStartRow = 0u;
    uint32_t cacheVisibleRows = 0u;
    uint32_t cacheTilesPerRow = 0u;
    uint64_t cacheContentToken = 0u;

    if (!ctx || !ui->canvas) {
        return 0;
    }
    overviewBounds = mega_memview_overviewBounds(ui, ui->canvas, ctx, ctx->font);
    contentBounds = mega_memview_overviewContentBounds(ctx, &overviewBounds);
    if (contentBounds.w <= 0 || contentBounds.h <= 0) {
        return 0;
    }
    if (ui->mode == mega_memview_mode_vram) {
        cacheTilesPerRow = mega_memview_clampVramTilesPerRow(ui->vramTilesPerRow);
        mega_memview_vramOverviewWindow(ui, &cacheStartRow, &cacheVisibleRows);
    }
    cacheContentToken = mega_memview_overviewContentToken(ui);
    if (ui->overviewBackgroundTexture &&
        ui->overviewBackgroundTextureW == contentBounds.w &&
        ui->overviewBackgroundTextureH == contentBounds.h &&
        ui->overviewBackgroundMode == (int)ui->mode &&
        ui->overviewBackgroundStartRow == cacheStartRow &&
        ui->overviewBackgroundVisibleRows == cacheVisibleRows &&
        ui->overviewBackgroundTilesPerRow == cacheTilesPerRow &&
        ui->overviewBackgroundContentToken == cacheContentToken) {
        return 1;
    }
    if (!mega_memview_ensureTexture(ctx->renderer,
                                      &ui->overviewBackgroundTexture,
                                      &ui->overviewBackgroundPixels,
                                      &ui->overviewBackgroundPixelsCap,
                                      &ui->overviewBackgroundTextureW,
                                      &ui->overviewBackgroundTextureH,
                                      contentBounds.w,
                                      contentBounds.h)) {
        return 0;
    }

    memset(ui->overviewBackgroundPixels, 0, (size_t)contentBounds.w * (size_t)contentBounds.h * sizeof(*ui->overviewBackgroundPixels));
    if (ui->mode == mega_memview_mode_vram) {
        mega_memview_renderVramOverviewBackground(ui, &contentBounds);
    } else {
        mega_memview_renderRamOverviewBackground(ui, &contentBounds);
    }

    SDL_UpdateTexture(ui->overviewBackgroundTexture,
                      NULL,
                      ui->overviewBackgroundPixels,
                      contentBounds.w * (int)sizeof(*ui->overviewBackgroundPixels));
    ui->overviewBackgroundMode = (int)ui->mode;
    ui->overviewBackgroundStartRow = cacheStartRow;
    ui->overviewBackgroundVisibleRows = cacheVisibleRows;
    ui->overviewBackgroundTilesPerRow = cacheTilesPerRow;
    ui->overviewBackgroundContentToken = cacheContentToken;
    return 1;
}

void
mega_memview_renderOverviewSelection(mega_memview_state_t *ui,
                                       e9ui_context_t *ctx,
                                       const e9ui_rect_t *overviewBounds,
                                       int visibleRows)
{
    e9ui_rect_t contentBounds;
    uint32_t baseAddr = 0u;
    uint64_t viewBytes = 0u;
    mega_memview_overview_range_t *range = NULL;
    uint64_t startOff = 0u;
    uint64_t endOff = 0u;
    SDL_Rect selection;

    if (!ctx || !overviewBounds || overviewBounds->w <= 0 || overviewBounds->h <= 0 || visibleRows <= 0) {
        return;
    }
    range = &ui->overviewRanges[0];
    if (range->sizeBytes == 0u) {
        return;
    }
    contentBounds = mega_memview_overviewContentBounds(ctx, overviewBounds);
    baseAddr = mega_memview_currentBaseAddr(ui);
    viewBytes = (uint64_t)visibleRows * (uint64_t)mega_memview_currentRowBytes(ui);
    if (ui->mode == mega_memview_mode_vram) {
        uint32_t tilesPerRow = mega_memview_clampVramTilesPerRow(ui->vramTilesPerRow);
        uint32_t totalRows = mega_memview_vramTotalRows(ui);
        uint32_t overviewStartRow = 0u;
        uint32_t overviewVisibleRows = totalRows;
        uint32_t baseTile = 0u;
        uint32_t baseRow = 0u;
        uint32_t endRow = 0u;

        mega_memview_vramOverviewWindow(ui, &overviewStartRow, &overviewVisibleRows);
        if (tilesPerRow == 0u || overviewVisibleRows == 0u) {
            return;
        }
        baseTile = baseAddr / MEGA_MEMVIEW_TILE_BYTES;
        baseRow = baseTile / tilesPerRow;
        endRow = baseRow + (uint32_t)visibleRows;
        if (baseRow < overviewStartRow) {
            baseRow = overviewStartRow;
        }
        if (endRow > overviewStartRow + overviewVisibleRows) {
            endRow = overviewStartRow + overviewVisibleRows;
        }
        selection.x = contentBounds.x;
        selection.y = contentBounds.y + (int)((uint64_t)(baseRow - overviewStartRow) * (uint64_t)contentBounds.h / (uint64_t)overviewVisibleRows);
        selection.w = contentBounds.w;
        selection.h = (int)((uint64_t)(endRow - baseRow) * (uint64_t)contentBounds.h / (uint64_t)overviewVisibleRows);
        if (selection.h < 2) {
            selection.h = 2;
        }
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 220);
        SDL_RenderDrawRect(ctx->renderer, &selection);
        return;
    }
    startOff = (uint64_t)(baseAddr - range->baseAddr);
    endOff = startOff + viewBytes;
    if (endOff > range->sizeBytes) {
        endOff = range->sizeBytes;
    }
    selection.x = contentBounds.x;
    selection.y = contentBounds.y + (int)((startOff * (uint64_t)contentBounds.h) / (uint64_t)range->sizeBytes);
    selection.w = contentBounds.w;
    selection.h = (int)(((endOff - startOff) * (uint64_t)contentBounds.h) / (uint64_t)range->sizeBytes);
    if (selection.h < 2) {
        selection.h = 2;
    }
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 220);
    SDL_RenderDrawRect(ctx->renderer, &selection);
}

int
mega_memview_rebuildOverviewTexture(mega_memview_state_t *ui,
                                      e9ui_context_t *ctx,
                                      const e9ui_rect_t *overviewBounds)
{
    e9ui_rect_t contentBounds;
    e9k_debug_mega_sprite_state_t spriteState;
    uint32_t vramTiles = 0u;
    uint32_t tilesPerRow = 0u;
    uint32_t totalRows = 0u;
    uint32_t overviewStartRow = 0u;
    uint32_t overviewVisibleRows = 0u;
    int colX[MEGA_MEMVIEW_MAX_VRAM_TILES_PER_ROW];
    int *rowY = NULL;

    if (!ctx || !overviewBounds || ui->mode != mega_memview_mode_vram) {
        return 0;
    }
    contentBounds = mega_memview_overviewContentBounds(ctx, overviewBounds);
    if (contentBounds.w <= 0 || contentBounds.h <= 0) {
        return 0;
    }
    if (!ui->overviewBackgroundTexture || !ui->overviewBackgroundPixels) {
        return 0;
    }
    if (!mega_memview_ensureTexture(ctx->renderer,
                                      &ui->overviewTexture,
                                      &ui->overviewPixels,
                                      &ui->overviewPixelsCap,
                                      &ui->overviewTextureW,
                                      &ui->overviewTextureH,
                                      contentBounds.w,
                                      contentBounds.h)) {
        return 0;
    }
    memcpy(ui->overviewPixels,
           ui->overviewBackgroundPixels,
           (size_t)contentBounds.w * (size_t)contentBounds.h * sizeof(*ui->overviewPixels));
    if (!libretro_host_megadrive_getSpriteState(&spriteState)) {
        return 0;
    }
    vramTiles = mega_memview_vramSize() / MEGA_MEMVIEW_TILE_BYTES;
    tilesPerRow = mega_memview_clampVramTilesPerRow(ui->vramTilesPerRow);
    totalRows = mega_memview_vramTotalRows(ui);
    mega_memview_vramOverviewWindow(ui, &overviewStartRow, &overviewVisibleRows);
    if (vramTiles == 0u || tilesPerRow == 0u || totalRows == 0u || overviewVisibleRows == 0u) {
        return 0;
    }
    if (!mega_memview_ensureOverviewTileMap(ui, vramTiles, tilesPerRow)) {
        return 0;
    }
    rowY = (int *)alloc_calloc(overviewVisibleRows, sizeof(*rowY));
    if (!rowY) {
        return 0;
    }
    for (uint32_t col = 0u; col < tilesPerRow && col < MEGA_MEMVIEW_MAX_VRAM_TILES_PER_ROW; ++col) {
        colX[col] = (int)(((uint64_t)col * (uint64_t)contentBounds.w) / (uint64_t)tilesPerRow);
        if (colX[col] >= contentBounds.w) {
            colX[col] = contentBounds.w - 1;
        }
    }
    for (uint32_t row = 0u; row < overviewVisibleRows; ++row) {
        rowY[row] = (int)(((uint64_t)row * (uint64_t)contentBounds.h) / (uint64_t)overviewVisibleRows);
        if (rowY[row] >= contentBounds.h) {
            rowY[row] = contentBounds.h - 1;
        }
    }
    if (ui->colorSourceSprites) {
        for (int i = 0; i < spriteState.spriteEntryCount && i < E9K_DEBUG_MEGA_MAX_FRAME_SPRITES; ++i) {
            const e9k_debug_mega_sprite_entry_t *entry = &spriteState.spriteEntries[i];
            uint32_t widthTiles = entry->widthTiles ? entry->widthTiles : 1u;
            uint32_t heightTiles = entry->heightTiles ? entry->heightTiles : 1u;

            if (!(entry->flags & (E9K_DEBUG_MEGA_SPRITEFLAG_VISIBLE | E9K_DEBUG_MEGA_SPRITEFLAG_RENDERED))) {
                continue;
            }
            for (uint32_t tx = 0u; tx < widthTiles; ++tx) {
                for (uint32_t ty = 0u; ty < heightTiles; ++ty) {
                    mega_memview_markOverviewPaletteTile(ui,
                                                          &contentBounds,
                                                          overviewStartRow,
                                                          overviewVisibleRows,
                                                          colX,
                                                          rowY,
                                                          (uint32_t)entry->tileIndex + tx * heightTiles + ty,
                                                          vramTiles);
                }
            }
        }
    }
    if (ui->colorSourcePlaneA) {
        mega_memview_markOverviewPlaneTiles(ui,
                                             &spriteState,
                                             &contentBounds,
                                             overviewStartRow,
                                             overviewVisibleRows,
                                             colX,
                                             rowY,
                                             vramTiles,
                                             0);
    }
    if (ui->colorSourcePlaneB) {
        mega_memview_markOverviewPlaneTiles(ui,
                                             &spriteState,
                                             &contentBounds,
                                             overviewStartRow,
                                             overviewVisibleRows,
                                             colX,
                                             rowY,
                                             vramTiles,
                                             1);
    }
    alloc_free(rowY);
    SDL_UpdateTexture(ui->overviewTexture,
                      NULL,
                      ui->overviewPixels,
                      contentBounds.w * (int)sizeof(*ui->overviewPixels));
    return 1;
}

int
mega_memview_overviewNavigate(mega_memview_state_t *ui, e9ui_component_t *self, e9ui_context_t *ctx, int mx, int my)
{
    e9ui_rect_t overviewBounds;
    e9ui_rect_t contentBounds;
    mega_memview_overview_range_t *range = NULL;
    uint64_t pos = 0u;
    uint32_t baseAddr = 0u;

    if (!self || !ctx) {
        return 0;
    }
    overviewBounds = mega_memview_overviewBounds(ui, self, ctx, ctx->font);
    contentBounds = mega_memview_overviewContentBounds(ctx, &overviewBounds);
    if (mx < contentBounds.x || mx >= contentBounds.x + contentBounds.w ||
        my < contentBounds.y || my >= contentBounds.y + contentBounds.h) {
        return 0;
    }
    range = &ui->overviewRanges[0];
    if (range->sizeBytes == 0u) {
        return 0;
    }
    pos = ((uint64_t)(my - contentBounds.y) * (uint64_t)range->sizeBytes) / (uint64_t)contentBounds.h;
    baseAddr = range->baseAddr + (uint32_t)pos;
    if (ui->mode == mega_memview_mode_vram) {
        uint32_t tilesPerRow = mega_memview_clampVramTilesPerRow(ui->vramTilesPerRow);
        uint32_t totalRows = mega_memview_vramTotalRows(ui);
        uint32_t overviewStartRow = 0u;
        uint32_t overviewVisibleRows = totalRows;
        uint32_t targetRow = 0u;
        uint32_t targetCol = 0u;
        uint32_t targetTile = 0u;

        mega_memview_vramOverviewWindow(ui, &overviewStartRow, &overviewVisibleRows);
        if (tilesPerRow == 0u || overviewVisibleRows == 0u) {
            return 0;
        }
        targetRow = overviewStartRow + (uint32_t)(((uint64_t)(my - contentBounds.y) * (uint64_t)overviewVisibleRows) / (uint64_t)contentBounds.h);
        targetCol = 0u;
        if (targetRow >= totalRows) {
            targetRow = totalRows - 1u;
        }
        targetTile = targetRow * tilesPerRow + targetCol;
        baseAddr = mega_memview_clampVramBaseAddr(targetTile * MEGA_MEMVIEW_TILE_BYTES);
        mega_memview_setView(ui, baseAddr, ui->vramTilesPerRow * MEGA_MEMVIEW_TILE_BYTES, 1);
    } else {
        baseAddr = ui->mode == mega_memview_mode_zram ?
            mega_memview_clampZramBaseAddr(baseAddr) :
            mega_memview_clampRamBaseAddr(baseAddr);
        mega_memview_setView(ui, baseAddr, ui->ramRowBytes, 1);
    }
    return 1;
}
