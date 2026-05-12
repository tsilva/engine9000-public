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
#include "neogeo_memview_internal.h"

static void
neogeo_memview_cromOverviewWindow(neogeo_memview_state_t *ui,
                                  uint32_t *outStartRow,
                                  uint32_t *outVisibleRows)
{
    uint32_t totalRows = neogeo_memview_cromTotalRows(ui);
    uint32_t zoomLevel = (uint32_t)neogeo_memview_clampOverviewZoomLevel(ui->overviewZoomLevel);
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
    currentTile = ui->cromBaseAddr / NEOGEO_MEMVIEW_TILE_BYTES;
    currentRow = currentTile / neogeo_memview_clampCromTilesPerRow(ui->cromTilesPerRow);
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
neogeo_memview_hashMemoryRange(const neogeo_memview_state_t *ui,
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
            (void)neogeo_memview_readRange(ui, addr, chunk, toRead);
        } else {
            (void)libretro_host_debugReadMemory(addr, chunk, toRead);
        }
        for (size_t i = 0; i < toRead; ++i) {
            hash ^= (uint64_t)chunk[i];
            hash *= NEOGEO_MEMVIEW_FNV1A64_PRIME;
        }
        if (toRead < sizeof(chunk)) {
            break;
        }
    }
    return hash;
}

static uint64_t
neogeo_memview_overviewContentToken(const neogeo_memview_state_t *ui)
{
    uint64_t hash = NEOGEO_MEMVIEW_FNV1A64_OFFSET_BASIS;

    if (ui->mode == neogeo_memview_mode_crom) {
        e9k_debug_rom_region_t crom = { 0 };

        if (!libretro_host_neogeo_getCRom(&crom)) {
            return 0u;
        }
        hash ^= (uint64_t)(uintptr_t)crom.data;
        hash *= NEOGEO_MEMVIEW_FNV1A64_PRIME;
        hash ^= (uint64_t)crom.size;
        hash *= NEOGEO_MEMVIEW_FNV1A64_PRIME;
        return hash;
    } else if (ui->mode == neogeo_memview_mode_zram) {
        return neogeo_memview_hashMemoryRange(ui, NEOGEO_MEMVIEW_ZRAM_BASE_MIN, NEOGEO_MEMVIEW_ZRAM_BASE_MAX, hash, 1);
    } else {
        return neogeo_memview_hashMemoryRange(ui, NEOGEO_MEMVIEW_RAM_BASE_MIN, NEOGEO_MEMVIEW_RAM_BASE_MAX, hash, 0);
    }
}

static int
neogeo_memview_measureOverviewGutterPx(const e9ui_context_t *ctx)
{
    e9ui_context_t tempCtx = ctx ? *ctx : (e9ui ? e9ui->ctx : (e9ui_context_t){ 0 });
    return e9ui_scale_px(&tempCtx, NEOGEO_MEMVIEW_OVERVIEW_GUTTER_PX);
}

static int
neogeo_memview_ensureOverviewTileMap(neogeo_memview_state_t *ui, uint32_t cromTiles, uint32_t tilesPerRow)
{
    uint32_t *newRows = NULL;
    uint32_t *newCols = NULL;

    if (!ui || cromTiles == 0u || tilesPerRow == 0u) {
        return 0;
    }
    if (ui->overviewTileRows &&
        ui->overviewTileCols &&
        ui->overviewTileMapCap >= cromTiles &&
        ui->overviewTileMapCromTiles == cromTiles &&
        ui->overviewTileMapTilesPerRow == tilesPerRow) {
        return 1;
    }
    if (ui->overviewTileMapCap < cromTiles) {
        newRows = (uint32_t *)realloc(ui->overviewTileRows, (size_t)cromTiles * sizeof(*ui->overviewTileRows));
        if (!newRows) {
            return 0;
        }
        ui->overviewTileRows = newRows;
        newCols = (uint32_t *)realloc(ui->overviewTileCols, (size_t)cromTiles * sizeof(*ui->overviewTileCols));
        if (!newCols) {
            return 0;
        }
        ui->overviewTileCols = newCols;
        ui->overviewTileMapCap = cromTiles;
    }
    for (uint32_t tile = 0u; tile < cromTiles; ++tile) {
        ui->overviewTileRows[tile] = tile / tilesPerRow;
        ui->overviewTileCols[tile] = tile % tilesPerRow;
    }
    ui->overviewTileMapCromTiles = cromTiles;
    ui->overviewTileMapTilesPerRow = tilesPerRow;
    return 1;
}

static void
neogeo_memview_renderCromOverviewBackground(neogeo_memview_state_t *ui, const e9ui_rect_t *contentBounds)
{
    e9k_debug_rom_region_t crom = { 0 };
    uint32_t cromTiles = neogeo_memview_cromSize() / NEOGEO_MEMVIEW_TILE_BYTES;
    uint32_t tilesPerRow = neogeo_memview_clampCromTilesPerRow(ui->cromTilesPerRow);
    uint32_t totalRows = neogeo_memview_cromTotalRows(ui);
    uint32_t overviewStartRow = 0u;
    uint32_t overviewVisibleRows = totalRows;

    (void)libretro_host_neogeo_getCRom(&crom);
    if (!crom.data || crom.size < NEOGEO_MEMVIEW_TILE_BYTES || cromTiles == 0u || tilesPerRow == 0u || totalRows == 0u) {
        return;
    }

    neogeo_memview_cromOverviewWindow(ui, &overviewStartRow, &overviewVisibleRows);
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

            if (tileNum >= cromTiles) {
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
            tileBase = tileNum * NEOGEO_MEMVIEW_TILE_BYTES;
            for (unsigned py = 0; py < NEOGEO_MEMVIEW_TILE_H; py += 2u) {
                for (unsigned px = 0; px < NEOGEO_MEMVIEW_TILE_W; px += 2u) {
                    pixelSum += neogeo_memview_readCromPixel(crom.data, crom.size, tileBase, px, py);
                    ++pixelCount;
                }
            }
            if (pixelCount > 0u) {
                brightness = (uint32_t)(pixelSum / pixelCount);
            }
            if (brightness > 15u) {
                brightness = 15u;
            }
            color = neogeo_memview_amberColor(brightness);
            for (int yy = y0; yy < y1; ++yy) {
                for (int xx = x0; xx < x1; ++xx) {
                    ui->overviewBackgroundPixels[(size_t)yy * (size_t)contentBounds->w + (size_t)xx] = color;
                }
            }
        }
    }
}

static void
neogeo_memview_renderRamOverviewBackground(neogeo_memview_state_t *ui, const e9ui_rect_t *contentBounds)
{
    uint32_t sample[256];
    neogeo_memview_overview_range_t *range = &ui->overviewRanges[0];
    uint32_t rowBytes = neogeo_memview_clampRamRowBytes(ui->ramRowBytes);
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
        (void)neogeo_memview_readRange(ui, rowBaseAddr, sample, readableBytes);
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
            color = neogeo_memview_argb(r, g, b);
            ui->overviewBackgroundPixels[(size_t)yy * (size_t)contentBounds->w + (size_t)xx] = color;
        }
    }
}

void
neogeo_memview_initOverviewRanges(neogeo_memview_state_t *ui)
{
    uint32_t cromSize = 0u;

    memset(ui->overviewRanges, 0, sizeof(ui->overviewRanges));
    if (ui->mode == neogeo_memview_mode_roms) {
        return;
    }
    if (ui->mode == neogeo_memview_mode_crom) {
        cromSize = neogeo_memview_cromSize();
        if (cromSize > 0u) {
            ui->overviewRanges[0].baseAddr = 0u;
            ui->overviewRanges[0].sizeBytes = cromSize;
        }
        return;
    }
    if (ui->mode == neogeo_memview_mode_zram) {
        ui->overviewRanges[0].baseAddr = NEOGEO_MEMVIEW_ZRAM_BASE_MIN;
        ui->overviewRanges[0].sizeBytes = NEOGEO_MEMVIEW_ZRAM_BASE_MAX - NEOGEO_MEMVIEW_ZRAM_BASE_MIN + 1u;
        return;
    }
    ui->overviewRanges[0].baseAddr = NEOGEO_MEMVIEW_RAM_BASE_MIN;
    ui->overviewRanges[0].sizeBytes = NEOGEO_MEMVIEW_RAM_BASE_MAX - NEOGEO_MEMVIEW_RAM_BASE_MIN + 1u;
}

int
neogeo_memview_overviewRangeCount(const neogeo_memview_state_t *ui)
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
neogeo_memview_overviewBounds(const neogeo_memview_state_t *ui, e9ui_component_t *self, const e9ui_context_t *ctx, TTF_Font *font)
{
    e9ui_rect_t bounds = { 0, 0, 0, 0 };
    int x = 0;

    if (!self || !ctx || !ui->showOverviewColumn || neogeo_memview_overviewRangeCount(ui) <= 0) {
        return bounds;
    }
    x = self->bounds.x;
    if (ui->showAddressColumn) {
        x += neogeo_memview_measureAddressGutterPx(ctx, font);
        x += e9ui_scale_px(ctx, NEOGEO_MEMVIEW_GUTTER_GAP_PX);
    }
    bounds.x = x;
    bounds.y = self->bounds.y + e9ui_scale_px(ctx, NEOGEO_MEMVIEW_TOP_PAD_PX);
    bounds.w = neogeo_memview_measureOverviewGutterPx(ctx);
    bounds.h = self->bounds.h - e9ui_scale_px(ctx, NEOGEO_MEMVIEW_TOP_PAD_PX) -
               e9ui_scale_px(ctx, NEOGEO_MEMVIEW_BOTTOM_PAD_PX);
    if (bounds.h < 1) {
        bounds.h = 1;
    }
    return bounds;
}

e9ui_rect_t
neogeo_memview_overviewContentBounds(const e9ui_context_t *ctx, const e9ui_rect_t *overviewBounds)
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
neogeo_memview_rebuildOverviewBackgroundTexture(neogeo_memview_state_t *ui, e9ui_context_t *ctx)
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
    overviewBounds = neogeo_memview_overviewBounds(ui, ui->canvas, ctx, ctx->font);
    contentBounds = neogeo_memview_overviewContentBounds(ctx, &overviewBounds);
    if (contentBounds.w <= 0 || contentBounds.h <= 0) {
        return 0;
    }
    if (ui->mode == neogeo_memview_mode_crom) {
        cacheTilesPerRow = neogeo_memview_clampCromTilesPerRow(ui->cromTilesPerRow);
        neogeo_memview_cromOverviewWindow(ui, &cacheStartRow, &cacheVisibleRows);
    }
    cacheContentToken = neogeo_memview_overviewContentToken(ui);
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
    if (!neogeo_memview_ensureTexture(ctx->renderer,
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
    if (ui->mode == neogeo_memview_mode_crom) {
        neogeo_memview_renderCromOverviewBackground(ui, &contentBounds);
    } else {
        neogeo_memview_renderRamOverviewBackground(ui, &contentBounds);
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
neogeo_memview_renderOverviewSelection(neogeo_memview_state_t *ui,
                                       e9ui_context_t *ctx,
                                       const e9ui_rect_t *overviewBounds,
                                       int visibleRows)
{
    e9ui_rect_t contentBounds;
    uint32_t baseAddr = 0u;
    uint64_t viewBytes = 0u;
    neogeo_memview_overview_range_t *range = NULL;
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
    contentBounds = neogeo_memview_overviewContentBounds(ctx, overviewBounds);
    baseAddr = neogeo_memview_currentBaseAddr(ui);
    viewBytes = (uint64_t)visibleRows * (uint64_t)neogeo_memview_currentRowBytes(ui);
    if (ui->mode == neogeo_memview_mode_crom) {
        uint32_t tilesPerRow = neogeo_memview_clampCromTilesPerRow(ui->cromTilesPerRow);
        uint32_t totalRows = neogeo_memview_cromTotalRows(ui);
        uint32_t overviewStartRow = 0u;
        uint32_t overviewVisibleRows = totalRows;
        uint32_t baseTile = 0u;
        uint32_t baseRow = 0u;
        uint32_t endRow = 0u;

        neogeo_memview_cromOverviewWindow(ui, &overviewStartRow, &overviewVisibleRows);
        if (tilesPerRow == 0u || overviewVisibleRows == 0u) {
            return;
        }
        baseTile = baseAddr / NEOGEO_MEMVIEW_TILE_BYTES;
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
neogeo_memview_rebuildOverviewTexture(neogeo_memview_state_t *ui,
                                      e9ui_context_t *ctx,
                                      const e9ui_rect_t *overviewBounds)
{
    e9ui_rect_t contentBounds;
    e9k_debug_sprite_state_t spriteState;
    const uint16_t *vram = NULL;
    uint32_t cromTiles = 0u;
    uint32_t tilesPerRow = 0u;
    uint32_t totalRows = 0u;
    uint32_t overviewStartRow = 0u;
    uint32_t overviewVisibleRows = 0u;
    int colX[NEOGEO_MEMVIEW_MAX_CROM_TILES_PER_ROW];
    int *rowY = NULL;

    if (!ctx || !overviewBounds || ui->mode != neogeo_memview_mode_crom) {
        return 0;
    }
    contentBounds = neogeo_memview_overviewContentBounds(ctx, overviewBounds);
    if (contentBounds.w <= 0 || contentBounds.h <= 0) {
        return 0;
    }
    if (!ui->overviewBackgroundTexture || !ui->overviewBackgroundPixels) {
        return 0;
    }
    if (!neogeo_memview_ensureTexture(ctx->renderer,
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
    if (!libretro_host_neogeo_getSpriteState(&spriteState) || !spriteState.vram || spriteState.vram_words == 0u) {
        return 0;
    }
    cromTiles = neogeo_memview_cromSize() / NEOGEO_MEMVIEW_TILE_BYTES;
    tilesPerRow = neogeo_memview_clampCromTilesPerRow(ui->cromTilesPerRow);
    totalRows = neogeo_memview_cromTotalRows(ui);
    neogeo_memview_cromOverviewWindow(ui, &overviewStartRow, &overviewVisibleRows);
    if (cromTiles == 0u || tilesPerRow == 0u || totalRows == 0u || overviewVisibleRows == 0u) {
        return 0;
    }
    if (!neogeo_memview_ensureOverviewTileMap(ui, cromTiles, tilesPerRow)) {
        return 0;
    }
    rowY = (int *)alloc_calloc(overviewVisibleRows, sizeof(*rowY));
    if (!rowY) {
        return 0;
    }
    for (uint32_t col = 0u; col < tilesPerRow && col < NEOGEO_MEMVIEW_MAX_CROM_TILES_PER_ROW; ++col) {
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
    vram = spriteState.vram;
    for (unsigned i = 0; i < NEOGEO_MEMVIEW_SPRITE_COUNT; ++i) {
        for (unsigned row = 0; row < NEOGEO_MEMVIEW_SPRITE_MAX_ROWS; ++row) {
            unsigned evenWordOffset = i * NEOGEO_MEMVIEW_SPRITE_VRAM_WORDS_PER_SPRITE +
                                      row * NEOGEO_MEMVIEW_SPRITE_TILE_ROW_WORDS;
            unsigned oddWordOffset = evenWordOffset + 1u;
            uint32_t spriteTileNum = 0u;
            uint32_t tileRow = 0u;
            uint32_t tileCol = 0u;
            int markerX = 0;
            int markerY = 0;

            if (oddWordOffset >= spriteState.vram_words) {
                break;
            }
            spriteTileNum = (uint32_t)(vram[evenWordOffset] |
                                       ((vram[oddWordOffset] & NEOGEO_MEMVIEW_SPRITE_TILE_HIGH_MASK) <<
                                        NEOGEO_MEMVIEW_SPRITE_TILE_HIGH_SHIFT));
            spriteTileNum %= cromTiles;
            tileRow = ui->overviewTileRows[spriteTileNum];
            tileCol = ui->overviewTileCols[spriteTileNum];
            if (tileRow < overviewStartRow || tileRow >= overviewStartRow + overviewVisibleRows) {
                continue;
            }
            markerX = colX[tileCol];
            markerY = rowY[tileRow - overviewStartRow];
            for (int yy = markerY - 1; yy <= markerY + 1; ++yy) {
                for (int xx = markerX - 1; xx <= markerX + 1; ++xx) {
                    if (xx >= 0 && xx < contentBounds.w && yy >= 0 && yy < contentBounds.h) {
                        ui->overviewPixels[(size_t)yy * (size_t)contentBounds.w + (size_t)xx] = neogeo_memview_argb(90, 220, 140);
                    }
                }
            }
        }
    }
    alloc_free(rowY);
    SDL_UpdateTexture(ui->overviewTexture,
                      NULL,
                      ui->overviewPixels,
                      contentBounds.w * (int)sizeof(*ui->overviewPixels));
    return 1;
}

int
neogeo_memview_overviewNavigate(neogeo_memview_state_t *ui, e9ui_component_t *self, e9ui_context_t *ctx, int mx, int my)
{
    e9ui_rect_t overviewBounds;
    e9ui_rect_t contentBounds;
    neogeo_memview_overview_range_t *range = NULL;
    uint64_t pos = 0u;
    uint32_t baseAddr = 0u;

    if (!self || !ctx) {
        return 0;
    }
    overviewBounds = neogeo_memview_overviewBounds(ui, self, ctx, ctx->font);
    contentBounds = neogeo_memview_overviewContentBounds(ctx, &overviewBounds);
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
    if (ui->mode == neogeo_memview_mode_crom) {
        uint32_t tilesPerRow = neogeo_memview_clampCromTilesPerRow(ui->cromTilesPerRow);
        uint32_t totalRows = neogeo_memview_cromTotalRows(ui);
        uint32_t overviewStartRow = 0u;
        uint32_t overviewVisibleRows = totalRows;
        uint32_t targetRow = 0u;
        uint32_t targetCol = 0u;
        uint32_t targetTile = 0u;

        neogeo_memview_cromOverviewWindow(ui, &overviewStartRow, &overviewVisibleRows);
        if (tilesPerRow == 0u || overviewVisibleRows == 0u) {
            return 0;
        }
        targetRow = overviewStartRow + (uint32_t)(((uint64_t)(my - contentBounds.y) * (uint64_t)overviewVisibleRows) / (uint64_t)contentBounds.h);
        targetCol = 0u;
        if (targetRow >= totalRows) {
            targetRow = totalRows - 1u;
        }
        targetTile = targetRow * tilesPerRow + targetCol;
        baseAddr = neogeo_memview_clampCromBaseAddr(targetTile * NEOGEO_MEMVIEW_TILE_BYTES);
        neogeo_memview_setView(ui, baseAddr, ui->cromTilesPerRow * NEOGEO_MEMVIEW_TILE_BYTES, 1);
    } else {
        baseAddr = ui->mode == neogeo_memview_mode_zram ?
            neogeo_memview_clampZramBaseAddr(baseAddr) :
            neogeo_memview_clampRamBaseAddr(baseAddr);
        neogeo_memview_setView(ui, baseAddr, ui->ramRowBytes, 1);
    }
    return 1;
}
