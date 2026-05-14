/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include "e9ui.h"
#include "e9ui_scrollbar.h"
#include "e9ui_step_buttons.h"
#include "e9ui_window.h"
#include "mega_memview.h"

#define MEGA_MEMVIEW_RAM_BASE_MIN 0x00ff0000u
#define MEGA_MEMVIEW_RAM_BASE_MAX 0x00ffffffu
#define MEGA_MEMVIEW_ZRAM_BASE_MIN 0x00000000u
#define MEGA_MEMVIEW_ZRAM_BASE_MAX 0x00001fffu
#define MEGA_MEMVIEW_Z80_PROCESSOR_ID 1u
#define MEGA_MEMVIEW_DEFAULT_RAM_ROW_BYTES 32u
#define MEGA_MEMVIEW_MAX_RAM_ROW_BYTES 256u
#define MEGA_MEMVIEW_DEFAULT_VRAM_TILES_PER_ROW 16u
#define MEGA_MEMVIEW_MAX_VRAM_TILES_PER_ROW 64u
#define MEGA_MEMVIEW_TILE_BYTES 32u
#define MEGA_MEMVIEW_TILE_W 8
#define MEGA_MEMVIEW_TILE_H 8
#define MEGA_MEMVIEW_TILE_GAP_PX 2
#define MEGA_MEMVIEW_GUTTER_GAP_PX 8
#define MEGA_MEMVIEW_OVERVIEW_GUTTER_PX 56
#define MEGA_MEMVIEW_RIGHT_PAD_PX 16
#define MEGA_MEMVIEW_TOP_PAD_PX 8
#define MEGA_MEMVIEW_BOTTOM_PAD_PX 8
#define MEGA_MEMVIEW_RAM_BIT_PX 2
#define MEGA_MEMVIEW_RAM_ROW_PX 2
#define MEGA_MEMVIEW_ZOOM_MIN 1
#define MEGA_MEMVIEW_ZOOM_MAX 32
#define MEGA_MEMVIEW_ZOOM_DEFAULT 8
#define MEGA_MEMVIEW_OVERVIEW_ZOOM_MIN 1
#define MEGA_MEMVIEW_OVERVIEW_ZOOM_MAX 32
#define MEGA_MEMVIEW_OVERVIEW_ZOOM_DEFAULT 1
#define MEGA_MEMVIEW_FOLLOW_SCORE_MARGIN_NUM 3u
#define MEGA_MEMVIEW_FOLLOW_SCORE_MARGIN_DEN 2u
#define MEGA_MEMVIEW_FOLLOW_MIN_CHANGED_TILES 8u
#define MEGA_MEMVIEW_FOLLOW_MIN_ACTIVE_TILES 16u
#define MEGA_MEMVIEW_FOLLOW_MIN_SCORE 256u
#define MEGA_MEMVIEW_FOLLOW_STABLE_FRAMES 3u
#define MEGA_MEMVIEW_TOOLBAR_MAX_ROW_ITEMS 64
#define MEGA_MEMVIEW_ROM_ENTRY_MAX 12
#define MEGA_MEMVIEW_SPRITE_COUNT 382u
#define MEGA_MEMVIEW_SPRITE_FIRST_VISIBLE 1u
#define MEGA_MEMVIEW_SPRITE_CHAIN_SCAN_END 381u
#define MEGA_MEMVIEW_SPRITE_MAX_ROWS 32u
#define MEGA_MEMVIEW_SPRITE_VRAM_WORDS_PER_SPRITE 64u
#define MEGA_MEMVIEW_SPRITE_TILE_ROW_WORDS 2u
#define MEGA_MEMVIEW_SCB3_WORD_OFFSET 0x8200u
#define MEGA_MEMVIEW_SCB3_CHAIN_FLAG 0x40u
#define MEGA_MEMVIEW_SCB3_ROW_MASK 0x3fu
#define MEGA_MEMVIEW_SCB3_YPOS_MASK 0x01ffu
#define MEGA_MEMVIEW_SCB3_YPOS_SHIFT 7u
#define MEGA_MEMVIEW_SPRITE_TILE_HIGH_MASK 0x00f0u
#define MEGA_MEMVIEW_SPRITE_TILE_HIGH_SHIFT 12u
#define MEGA_MEMVIEW_SPRITE_PALETTE_SHIFT 8u
#define MEGA_MEMVIEW_SPRITE_PALETTE_MASK 0x00ffu
#define MEGA_MEMVIEW_FNV1A64_OFFSET_BASIS 1469598103934665603ull
#define MEGA_MEMVIEW_FNV1A64_PRIME 1099511628211ull
#define MEGA_MEMVIEW_VRAM_USAGE_SAT 0x01u
#define MEGA_MEMVIEW_VRAM_USAGE_HSCROLL 0x02u
#define MEGA_MEMVIEW_VRAM_USAGE_PLANE_A_TABLE 0x04u
#define MEGA_MEMVIEW_VRAM_USAGE_PLANE_B_TABLE 0x08u
#define MEGA_MEMVIEW_VRAM_USAGE_WINDOW_TABLE 0x10u

typedef enum mega_memview_mode {
    mega_memview_mode_ram = 0,
    mega_memview_mode_vram = 1,
    mega_memview_mode_zram = 2,
    mega_memview_mode_roms = 3
} mega_memview_mode_t;

typedef struct mega_memview_overview_range {
    uint32_t baseAddr;
    uint32_t sizeBytes;
} mega_memview_overview_range_t;

typedef struct mega_memview_plane_table {
    uint32_t wordOffset;
    uint32_t cols;
    uint32_t rows;
} mega_memview_plane_table_t;

typedef struct mega_memview_state mega_memview_state_t;

struct mega_memview_state {
    e9ui_window_state_t windowState;
    e9ui_context_t ctx;
    e9ui_component_t *root;
    e9ui_component_t *canvas;
    e9ui_component_t *modeButtonRam;
    e9ui_component_t *modeButtonVram;
    e9ui_component_t *modeButtonZram;
    e9ui_component_t *modeButtonRoms;
    e9ui_component_t *addressBox;
    e9ui_component_t *widthBox;
    e9ui_component_t *widthSeek;
    e9ui_component_t *zoomSeek;
    e9ui_component_t *overviewZoomSeek;
    e9ui_component_t *overlayBodyHost;
    mega_memview_mode_t mode;
    uint32_t ramBaseAddr;
    uint32_t vramBaseAddr;
    uint32_t zramBaseAddr;
    uint32_t ramRowBytes;
    uint32_t vramTilesPerRow;
    int zoomLevel;
    int overviewZoomLevel;
    int showAddressColumn;
    int showOverviewColumn;
    int followActiveSprites;
    int colorSourceSprites;
    int colorSourcePlaneA;
    int colorSourcePlaneB;
    int colorSourceHScroll;
    int modeHasSaved;
    int ramBaseAddrHasSaved;
    int vramBaseAddrHasSaved;
    int zramBaseAddrHasSaved;
    int ramRowBytesHasSaved;
    int vramTilesPerRowHasSaved;
    int zoomHasSaved;
    int overviewZoomHasSaved;
    int showAddressColumnHasSaved;
    int showOverviewColumnHasSaved;
    int followActiveSpritesHasSaved;
    int colorSourceSpritesHasSaved;
    int colorSourcePlaneAHasSaved;
    int colorSourcePlaneBHasSaved;
    int colorSourceHScrollHasSaved;
    e9ui_step_buttons_state_t stepButtons;
    e9ui_scrollbar_state_t hScrollbar;
    int scrollX;
    int romsScrollY;
    int contentPixelWidth;
    SDL_Renderer *renderer;
    SDL_Texture *mainTexture;
    uint32_t *mainPixels;
    size_t mainPixelsCap;
    int mainTextureW;
    int mainTextureH;
    int mainVramCacheValid;
    uint32_t mainVramCacheBaseAddr;
    uint32_t mainVramCacheVramTiles;
    int mainVramCacheTilesPerRow;
    int mainVramCacheVisibleRows;
    int mainVramCachePixelPx;
    int mainVramCacheTileGapPx;
    int mainVramCacheTexW;
    int mainVramCacheTexH;
    uint64_t mainVramCacheContentToken;
    uint64_t mainVramCachePaletteToken;
    int mainRomsCacheValid;
    int mainRomsCacheTexW;
    int mainRomsCacheTexH;
    int mainRomsCacheCols;
    int mainRomsCacheCardW;
    int mainRomsCacheCardH;
    int mainRomsCacheContentW;
    int mainRomsCacheContentH;
    uint64_t mainRomsCacheToken;
    e9k_debug_rom_entry_t mainRomsEntries[MEGA_MEMVIEW_ROM_ENTRY_MAX];
    size_t mainRomsEntryCount;
    uint64_t mainRomsContentToken;
    int mainRomsEntriesValid;
    e9k_debug_mega_sprite_entry_t *followPrevSpriteEntries;
    size_t followPrevSpriteEntryCount;
    uint32_t followPendingStartRow;
    uint32_t followPendingFrames;
    SDL_Texture *overviewTexture;
    uint32_t *overviewPixels;
    size_t overviewPixelsCap;
    int overviewTextureW;
    int overviewTextureH;
    uint32_t *overviewTileRows;
    uint32_t *overviewTileCols;
    size_t overviewTileMapCap;
    uint32_t overviewTileMapVramTiles;
    uint32_t overviewTileMapTilesPerRow;
    SDL_Texture *overviewBackgroundTexture;
    uint32_t *overviewBackgroundPixels;
    size_t overviewBackgroundPixelsCap;
    int overviewBackgroundTextureW;
    int overviewBackgroundTextureH;
    uint32_t overviewWindowStartRow;
    uint32_t overviewWindowVisibleRows;
    uint32_t overviewBackgroundStartRow;
    uint32_t overviewBackgroundVisibleRows;
    uint32_t overviewBackgroundTilesPerRow;
    int overviewBackgroundMode;
    uint64_t overviewBackgroundContentToken;
    mega_memview_overview_range_t overviewRanges[2];
    int (*widthSeekDefaultHandleEvent)(e9ui_component_t *self, e9ui_context_t *ctx, const e9ui_event_t *ev);
    int (*zoomSeekDefaultHandleEvent)(e9ui_component_t *self, e9ui_context_t *ctx, const e9ui_event_t *ev);
    int (*overviewZoomSeekDefaultHandleEvent)(e9ui_component_t *self, e9ui_context_t *ctx, const e9ui_event_t *ev);
};

uint32_t
mega_memview_clampRamRowBytes(uint32_t rowBytes);

int
mega_memview_clampOverviewZoomLevel(int zoomLevel);

uint32_t
mega_memview_clampVramTilesPerRow(uint32_t tilesPerRow);

uint32_t
mega_memview_clampRamBaseAddr(uint32_t baseAddr);

uint32_t
mega_memview_clampZramBaseAddr(uint32_t baseAddr);

uint32_t
mega_memview_vramSize(void);

uint32_t
mega_memview_clampVramBaseAddr(uint32_t baseAddr);

uint32_t
mega_memview_vramTotalRows(const mega_memview_state_t *ui);

uint32_t
mega_memview_currentBaseAddr(const mega_memview_state_t *ui);

uint32_t
mega_memview_currentRowBytes(const mega_memview_state_t *ui);

int
mega_memview_readRange(const mega_memview_state_t *ui, uint32_t addr, void *out, size_t sizeBytes);

int
mega_memview_vdpPlaneTable(const e9k_debug_mega_sprite_state_t *videoState,
                             int plane,
                             mega_memview_plane_table_t *out);

uint32_t
mega_memview_argb(uint8_t r, uint8_t g, uint8_t b);

uint32_t
mega_memview_amberColor(unsigned index);

int
mega_memview_ensureTexture(SDL_Renderer *renderer,
                             SDL_Texture **texture,
                             uint32_t **pixels,
                             size_t *pixelsCap,
                             int *textureW,
                             int *textureH,
                             int width,
                             int height);

uint8_t
mega_memview_readVramPixel(const uint8_t *data, size_t sizeBytes, uint32_t tileBaseAddr, unsigned x, unsigned y);

int
mega_memview_measureAddressGutterPx(const e9ui_context_t *ctx, TTF_Font *font);

void
mega_memview_setView(mega_memview_state_t *ui, uint32_t baseAddr, uint32_t rowBytes, int resetScroll);

void
mega_memview_initOverviewRanges(mega_memview_state_t *ui);

int
mega_memview_overviewRangeCount(const mega_memview_state_t *ui);

e9ui_rect_t
mega_memview_overviewBounds(const mega_memview_state_t *ui, e9ui_component_t *self, const e9ui_context_t *ctx, TTF_Font *font);

e9ui_rect_t
mega_memview_overviewContentBounds(const e9ui_context_t *ctx, const e9ui_rect_t *overviewBounds);

int
mega_memview_rebuildOverviewBackgroundTexture(mega_memview_state_t *ui, e9ui_context_t *ctx);

void
mega_memview_renderOverviewSelection(mega_memview_state_t *ui,
                                       e9ui_context_t *ctx,
                                       const e9ui_rect_t *overviewBounds,
                                       int visibleRows);

int
mega_memview_rebuildOverviewTexture(mega_memview_state_t *ui,
                                      e9ui_context_t *ctx,
                                      const e9ui_rect_t *overviewBounds);

int
mega_memview_overviewNavigate(mega_memview_state_t *ui, e9ui_component_t *self, e9ui_context_t *ctx, int mx, int my);
