/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "amiga_custom_regs.h"
#include "amiga_memsave.h"
#include "debugger.h"
#include "e9ui_box.h"
#include "e9ui_button.h"
#include "e9ui_center.h"
#include "e9ui_flow.h"
#include "e9ui_modal.h"
#include "e9ui_overlay.h"
#include "e9ui_stack.h"
#include "e9ui_text.h"
#include "libretro_host.h"
#include "platform.h"
#include "settings.h"
#include "strutil.h"
#include "target.h"

#define AMIGA_MEMSAVE_OVERVIEW_DEFAULT_RANGE1_BASE 0xC00000u
#define AMIGA_MEMSAVE_OVERVIEW_FAST_RANGE_BASE 0x200000u
#define AMIGA_MEMSAVE_EXPORT_MAX_RANGES 64
#define AMIGA_MEMSAVE_EXPORT_CHUNK 65536u

enum
{
    amiga_memsave_ram_type_chip = 0,
    amiga_memsave_ram_type_slow,
    amiga_memsave_ram_type_fast,
    amiga_memsave_ram_type_other,
    amiga_memsave_ram_type_count
};

typedef struct amiga_memsave_save_plan {
    target_memory_range_t ranges[AMIGA_MEMSAVE_EXPORT_MAX_RANGES];
    size_t rangeCount;
    char exportPaths[amiga_memsave_ram_type_count][PATH_MAX];
    char customRegsPath[PATH_MAX];
    char collisionMessage[PATH_MAX];
    char sequenceButtonLabel[PATH_MAX];
    int exportTypes[amiga_memsave_ram_type_count];
    int exportTypeCount;
    e9ui_component_t *modal;
} amiga_memsave_save_plan_t;

static uint16_t
amiga_memsave_regValue(const e9k_debug_ami_custom_reg_state_t *regs, uint16_t regOffset)
{
    uint16_t normalized = (uint16_t)(regOffset & 0x01feu);
    return regs ? regs[normalized >> 1].value : 0u;
}

static int
amiga_memsave_getAddressLimits(uint32_t *outMinAddr, uint32_t *outMaxAddr)
{
    if (outMinAddr) {
        *outMinAddr = 0u;
    }
    if (outMaxAddr) {
        *outMaxAddr = 0x00ffffffu;
    }
    if (target && target->memoryGetLimits) {
        return target->memoryGetLimits(outMinAddr, outMaxAddr);
    }
    return 0;
}

static int
amiga_memsave_readRange(uint32_t baseAddr, uint8_t *data, size_t size)
{
    uint32_t minAddr = 0u;
    uint32_t maxAddr = 0x00ffffffu;
    uint64_t readStart = 0u;
    uint64_t readEnd = 0u;
    int hasLimits = 0;

    if (!data || size == 0u) {
        return 0;
    }

    memset(data, 0, size);
    hasLimits = amiga_memsave_getAddressLimits(&minAddr, &maxAddr);
    if (!hasLimits) {
        minAddr = 0u;
        maxAddr = 0x00ffffffu;
    }

    readStart = (uint64_t)(baseAddr & 0x00ffffffu);
    readEnd = readStart + (uint64_t)size - 1u;
    if (readStart > (uint64_t)maxAddr || readEnd < (uint64_t)minAddr) {
        return 0;
    }
    if (readStart < (uint64_t)minAddr) {
        readStart = minAddr;
    }
    if (readEnd > (uint64_t)maxAddr) {
        readEnd = maxAddr;
    }
    if (readEnd < readStart) {
        return 0;
    }

    size_t dstOffset = (size_t)(readStart - (uint64_t)(baseAddr & 0x00ffffffu));
    size_t readSize = (size_t)(readEnd - readStart + 1u);
    if (dstOffset + readSize > size) {
        return 0;
    }
    return libretro_host_debugReadMemory((uint32_t)readStart, data + dstOffset, readSize) ? 1 : 0;
}

static int
amiga_memsave_ramTypeFromBaseAddr(uint32_t baseAddr)
{
    uint32_t start = baseAddr & 0x00ffffffu;

    if (start == 0x000000u) {
        return amiga_memsave_ram_type_chip;
    }
    if (start == AMIGA_MEMSAVE_OVERVIEW_DEFAULT_RANGE1_BASE) {
        return amiga_memsave_ram_type_slow;
    }
    if (start == AMIGA_MEMSAVE_OVERVIEW_FAST_RANGE_BASE) {
        return amiga_memsave_ram_type_fast;
    }
    return amiga_memsave_ram_type_other;
}

static const char *
amiga_memsave_lastPathSeparator(const char *path)
{
    const char *slash = NULL;
    const char *backslash = NULL;

    if (!path) {
        return NULL;
    }
    slash = strrchr(path, '/');
    backslash = strrchr(path, '\\');
    if (slash && backslash) {
        return slash > backslash ? slash : backslash;
    }
    return slash ? slash : backslash;
}

static const char *
amiga_memsave_basename(const char *path)
{
    const char *sep = amiga_memsave_lastPathSeparator(path);

    return sep ? sep + 1 : path;
}

static int
amiga_memsave_copyConfigName(char *out, size_t cap, const char *configPath)
{
    const char *base = NULL;
    const char *dot = NULL;

    if (!out || cap == 0u || !configPath || !configPath[0]) {
        return 0;
    }

    base = amiga_memsave_basename(configPath);
    if (!base || !base[0]) {
        return 0;
    }

    strutil_strlcpy(out, cap, base);
    if (out[0] == '\0') {
        return 0;
    }
    dot = strrchr(out, '.');
    if (dot && dot > out) {
        *(char*)dot = '\0';
    }
    if (out[0] == '\0') {
        return 0;
    }
    return 1;
}

static const char *
amiga_memsave_ramTypeFileTag(int ramType)
{
    switch (ramType) {
    case amiga_memsave_ram_type_chip:
        return "chip";
    case amiga_memsave_ram_type_slow:
        return "slow";
    case amiga_memsave_ram_type_fast:
        return "fast";
    default:
        return "other";
    }
}

static int
amiga_memsave_buildExportFileName(char *out, size_t cap, const char *configName, int ramType)
{
    char stem[PATH_MAX];
    const char *tag = amiga_memsave_ramTypeFileTag(ramType);
    size_t stemLen = 0u;

    if (!out || cap == 0u || !configName || !configName[0] || !tag || !tag[0]) {
        return 0;
    }
    strutil_join3Trunc(stem, sizeof(stem), configName, "-", tag);
    stemLen = strlen(configName) + 1u + strlen(tag);
    if (strlen(stem) != stemLen) {
        return 0;
    }
    strutil_join2Trunc(out, cap, stem, ".bin");
    if (strlen(out) != stemLen + 4u) {
        return 0;
    }
    return 1;
}

static int
amiga_memsave_buildCustomRegsFileName(char *out, size_t cap, const char *configName)
{
    char stem[PATH_MAX];
    size_t stemLen = 0u;

    if (!out || cap == 0u || !configName || !configName[0]) {
        return 0;
    }
    strutil_join2Trunc(stem, sizeof(stem), configName, "-custom-regs");
    stemLen = strlen(configName) + strlen("-custom-regs");
    if (strlen(stem) != stemLen) {
        return 0;
    }
    strutil_join2Trunc(out, cap, stem, ".txt");
    if (strlen(out) != stemLen + 4u) {
        return 0;
    }
    return 1;
}

static int
amiga_memsave_collectExportRanges(target_memory_range_t *outRanges, size_t cap, size_t *outCount)
{
    size_t count = 0u;
    size_t write = 0u;

    *outCount = 0u;
    if (!target || !target->memoryTrackGetRanges) {
        return 0;
    }
    if (!target->memoryTrackGetRanges(outRanges, cap, &count) || count == 0u) {
        return 0;
    }
    for (size_t i = 0; i < count; ++i) {
        if (outRanges[i].size == 0u) {
            continue;
        }
        outRanges[write++] = outRanges[i];
    }
    *outCount = write;
    return write > 0u ? 1 : 0;
}

static int
amiga_memsave_buildIncrementedPath(char *out, size_t cap, const char *path, unsigned index)
{
    const char *lastSep = NULL;
    const char *dot = NULL;
    const char *ext = NULL;
    char suffix[32];
    size_t stemLen = 0u;
    size_t suffixLen = 0u;
    size_t extLen = 0u;
    size_t pathLen = 0u;

    if (!out || cap == 0u || !path || !path[0]) {
        return 0;
    }
    lastSep = amiga_memsave_lastPathSeparator(path);
    dot = strrchr(path, '.');
    pathLen = strlen(path);
    if (dot && (!lastSep || dot > lastSep)) {
        ext = dot;
        stemLen = (size_t)(dot - path);
        extLen = pathLen - stemLen;
    } else {
        ext = path + pathLen;
        stemLen = pathLen;
        extLen = 0u;
    }

    if (snprintf(suffix, sizeof(suffix), "-%u", index) < 0) {
        return 0;
    }
    suffix[sizeof(suffix) - 1u] = '\0';
    suffixLen = strlen(suffix);
    if (stemLen + suffixLen + extLen >= cap) {
        return 0;
    }

    memcpy(out, path, stemLen);
    memcpy(out + stemLen, suffix, suffixLen);
    if (extLen > 0u) {
        memcpy(out + stemLen + suffixLen, ext, extLen);
    }
    out[stemLen + suffixLen + extLen] = '\0';
    return 1;
}

static int
amiga_memsave_resolveIncrementedPath(char *path, size_t cap)
{
    char candidate[PATH_MAX];

    if (!path || cap == 0u || !path[0]) {
        return 0;
    }
    if (!settings_pathExistsFile(path)) {
        return 1;
    }

    for (unsigned index = 1u; index < 1000000u; ++index) {
        if (!amiga_memsave_buildIncrementedPath(candidate, sizeof(candidate), path, index)) {
            return 0;
        }
        if (!settings_pathExistsFile(candidate)) {
            strutil_strlcpy(path, cap, candidate);
            return strcmp(path, candidate) == 0 ? 1 : 0;
        }
    }
    return 0;
}

static int
amiga_memsave_copyBasenameNoExt(char *out, size_t cap, const char *path)
{
    const char *base = NULL;
    const char *dot = NULL;
    size_t len = 0u;

    if (!out || cap == 0u || !path || !path[0]) {
        return 0;
    }
    base = amiga_memsave_basename(path);
    if (!base[0]) {
        return 0;
    }
    dot = strrchr(base, '.');
    if (dot) {
        len = (size_t)(dot - base);
    } else {
        len = strlen(base);
    }
    if (len == 0u || len >= cap) {
        return 0;
    }
    memcpy(out, base, len);
    out[len] = '\0';
    return 1;
}

static int
amiga_memsave_buildCollisionMessage(char *out, size_t cap, const char *path)
{
    char basename[PATH_MAX];

    if (!out || cap == 0u || !path || !path[0]) {
        return 0;
    }
    if (!amiga_memsave_copyBasenameNoExt(basename, sizeof(basename), path)) {
        return 0;
    }
    strutil_join2Trunc(out, cap, basename, " already exists");
    return strlen(out) == strlen(basename) + strlen(" already exists") ? 1 : 0;
}

static int
amiga_memsave_buildSequenceButtonLabel(char *out, size_t cap, const char *path)
{
    char candidate[PATH_MAX];
    char basename[PATH_MAX];

    if (!out || cap == 0u || !path || !path[0]) {
        return 0;
    }
    out[0] = '\0';
    for (unsigned index = 1u; index < 1000000u; ++index) {
        if (!amiga_memsave_buildIncrementedPath(candidate, sizeof(candidate), path, index)) {
            return 0;
        }
        if (!settings_pathExistsFile(candidate)) {
            if (!amiga_memsave_copyBasenameNoExt(basename, sizeof(basename), candidate)) {
                return 0;
            }
            strutil_join2Trunc(out, cap, "Save as ", basename);
            return strlen(out) == strlen("Save as ") + strlen(basename) ? 1 : 0;
        }
    }
    return 0;
}

static int
amiga_memsave_writeCustomRegsExport(FILE *out)
{
    const e9k_debug_ami_custom_reg_state_t *regs = libretro_host_amiga_getCustomRegs();

    if (!out) {
        return 0;
    }

    if (fputs("=== CUSTOM CHIPSET REGISTERS ===\n", out) < 0) {
        return 0;
    }
    if (!regs) {
        if (fputs("UNAVAILABLE\n", out) < 0) {
            return 0;
        }
        return 1;
    }

    for (uint16_t regOffset = 0u; regOffset <= 0x01feu; regOffset += 2u) {
        const char *regName = amiga_custom_regs_nameForOffset(regOffset);
        uint16_t regValue = amiga_memsave_regValue(regs, regOffset);

        if (!regName || !regName[0]) {
            regName = "UNKNOWN";
        }
        if (fprintf(out, "$DFF%03X %-10s $%04X\n", (unsigned)regOffset, regName, (unsigned)regValue) < 0) {
            return 0;
        }
    }

    return 1;
}

static int
amiga_memsave_writeCustomRegsExportPath(const char *finalPath)
{
    char tempPath[PATH_MAX];
    FILE *out = NULL;

    if (!finalPath || !finalPath[0]) {
        return 0;
    }
    strutil_join2Trunc(tempPath, sizeof(tempPath), finalPath, ".tmp");
    if (strlen(tempPath) != strlen(finalPath) + 4u) {
        return 0;
    }
    out = fopen(tempPath, "wb");
    if (!out) {
        return 0;
    }
    if (!amiga_memsave_writeCustomRegsExport(out)) {
        fclose(out);
        remove(tempPath);
        return 0;
    }
    if (fclose(out) != 0) {
        remove(tempPath);
        return 0;
    }
    if (!debugger_platform_replaceFile(tempPath, finalPath)) {
        remove(tempPath);
        return 0;
    }
    return 1;
}

static int
amiga_memsave_writeExportType(const target_memory_range_t *ranges,
                              size_t rangeCount,
                              int ramType,
                              const char *finalPath)
{
    uint8_t buffer[AMIGA_MEMSAVE_EXPORT_CHUNK];
    char tempPath[PATH_MAX];
    FILE *out = NULL;
    int wroteAny = 0;

    if (!ranges || rangeCount == 0u || !finalPath || !finalPath[0]) {
        return 0;
    }
    strutil_join2Trunc(tempPath, sizeof(tempPath), finalPath, ".tmp");
    if (strlen(tempPath) != strlen(finalPath) + 4u) {
        return 0;
    }

    out = fopen(tempPath, "wb");
    if (!out) {
        return 0;
    }

    for (size_t i = 0; i < rangeCount; ++i) {
        uint32_t addr = 0u;
        uint32_t remaining = 0u;

        if (ranges[i].size == 0u || amiga_memsave_ramTypeFromBaseAddr(ranges[i].baseAddr) != ramType) {
            continue;
        }

        addr = ranges[i].baseAddr;
        remaining = ranges[i].size;
        wroteAny = 1;
        while (remaining > 0u) {
            size_t chunkSize = remaining < (uint32_t)sizeof(buffer) ? (size_t)remaining : sizeof(buffer);

            if (!amiga_memsave_readRange(addr, buffer, chunkSize)) {
                fclose(out);
                remove(tempPath);
                return 0;
            }
            if (fwrite(buffer, 1u, chunkSize, out) != chunkSize) {
                fclose(out);
                remove(tempPath);
                return 0;
            }
            addr += (uint32_t)chunkSize;
            remaining -= (uint32_t)chunkSize;
        }
    }
    if (fclose(out) != 0) {
        remove(tempPath);
        return 0;
    }
    if (!wroteAny) {
        remove(tempPath);
        return 1;
    }
    if (!debugger_platform_replaceFile(tempPath, finalPath)) {
        remove(tempPath);
        return 0;
    }
    return 1;
}

static int
amiga_memsave_finishSavePlan(amiga_memsave_save_plan_t *plan, int saveAsSequence)
{
    if (!plan || plan->rangeCount == 0u || plan->exportTypeCount == 0) {
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return 0;
    }

    for (int i = 0; i < plan->exportTypeCount; ++i) {
        int ramType = plan->exportTypes[i];

        if (saveAsSequence &&
            !amiga_memsave_resolveIncrementedPath(plan->exportPaths[ramType], sizeof(plan->exportPaths[ramType]))) {
            e9ui_showTransientMessage("RAM SAVE FAILED");
            return 0;
        }
        if (!amiga_memsave_writeExportType(plan->ranges,
                                           plan->rangeCount,
                                           ramType,
                                           plan->exportPaths[ramType])) {
            e9ui_showTransientMessage("RAM SAVE FAILED");
            return 0;
        }
    }
    if (saveAsSequence &&
        !amiga_memsave_resolveIncrementedPath(plan->customRegsPath, sizeof(plan->customRegsPath))) {
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return 0;
    }
    if (!amiga_memsave_writeCustomRegsExportPath(plan->customRegsPath)) {
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return 0;
    }

    e9ui_showTransientMessage("RAM SAVED");
    return 1;
}

static void
amiga_memsave_requestCloseSaveModal(amiga_memsave_save_plan_t *plan)
{
    e9ui_component_t *modal = NULL;

    if (!plan || !plan->modal) {
        if (plan) {
            alloc_free(plan);
        }
        return;
    }
    modal = plan->modal;
    plan->modal = NULL;
    e9ui_modal_setCloseCallback(modal, NULL, NULL);
    e9ui_setHidden(modal, 1);
    if (e9ui && !e9ui->pendingRemove) {
        e9ui->pendingRemove = modal;
    }
    alloc_free(plan);
}

static void
amiga_memsave_saveModalClosed(e9ui_component_t *modal, void *user)
{
    amiga_memsave_save_plan_t *plan = (amiga_memsave_save_plan_t*)user;

    (void)modal;
    if (plan) {
        plan->modal = NULL;
        e9ui_modal_setCloseCallback(modal, NULL, NULL);
        alloc_free(plan);
    }
}

static void
amiga_memsave_saveOverwriteClicked(e9ui_context_t *ctx, void *user)
{
    amiga_memsave_save_plan_t *plan = (amiga_memsave_save_plan_t*)user;

    (void)ctx;
    amiga_memsave_finishSavePlan(plan, 0);
    amiga_memsave_requestCloseSaveModal(plan);
}

static void
amiga_memsave_saveSequenceClicked(e9ui_context_t *ctx, void *user)
{
    amiga_memsave_save_plan_t *plan = (amiga_memsave_save_plan_t*)user;

    (void)ctx;
    amiga_memsave_finishSavePlan(plan, 1);
    amiga_memsave_requestCloseSaveModal(plan);
}

static void
amiga_memsave_saveCancelClicked(e9ui_context_t *ctx, void *user)
{
    amiga_memsave_save_plan_t *plan = (amiga_memsave_save_plan_t*)user;

    (void)ctx;
    amiga_memsave_requestCloseSaveModal(plan);
}

static e9ui_component_t *
amiga_memsave_makeSaveCollisionModalBody(amiga_memsave_save_plan_t *plan)
{
    e9ui_component_t *message = e9ui_stack_makeVertical();
    e9ui_component_t *contentBox = NULL;
    e9ui_component_t *center = NULL;
    e9ui_component_t *overwriteButton = NULL;
    e9ui_component_t *sequenceButton = NULL;
    e9ui_component_t *cancelButton = NULL;
    e9ui_component_t *footer = NULL;
    e9ui_component_t *overlay = NULL;

    if (!message) {
        return NULL;
    }
    e9ui_stack_addFixed(message,
                        e9ui_text_make(plan->collisionMessage[0] ?
                                       plan->collisionMessage :
                                       "RAM dump files already exist"));

    contentBox = e9ui_box_make(message);
    e9ui_box_setPadding(contentBox, 16);
    center = e9ui_center_make(contentBox);
    e9ui_center_setSize(center, 520, 80);

    overwriteButton = e9ui_button_make("Overwrite", amiga_memsave_saveOverwriteClicked, plan);
    sequenceButton = e9ui_button_make(plan->sequenceButtonLabel[0] ? plan->sequenceButtonLabel : "Save as sequence",
                                      amiga_memsave_saveSequenceClicked,
                                      plan);
    cancelButton = e9ui_button_make("Cancel", amiga_memsave_saveCancelClicked, plan);
    footer = e9ui_flow_make();
    e9ui_flow_setPadding(footer, 0);
    e9ui_flow_setSpacing(footer, 8);
    e9ui_flow_setWrap(footer, 0);
    e9ui_button_setTheme(overwriteButton, e9ui_theme_button_preset_red());
    e9ui_button_setGlowPulse(overwriteButton, 1);
    e9ui_flow_add(footer, overwriteButton);
    e9ui_button_setTheme(sequenceButton, e9ui_theme_button_preset_green());
    e9ui_button_setGlowPulse(sequenceButton, 1);
    e9ui_flow_add(footer, sequenceButton);
    e9ui_flow_add(footer, cancelButton);

    overlay = e9ui_overlay_make(center, footer);
    e9ui_overlay_setAnchor(overlay, e9ui_anchor_bottom_right);
    e9ui_overlay_setMargin(overlay, 12);
    return overlay;
}

static int
amiga_memsave_showSaveCollisionModal(e9ui_context_t *ctx, amiga_memsave_save_plan_t *plan)
{
    int modalW = 0;
    int modalH = 0;
    int x = 0;
    int y = 0;
    e9ui_rect_t rect;
    e9ui_component_t *body = NULL;

    if (!ctx || !plan) {
        return 0;
    }
    modalW = e9ui_scale_px(ctx, 600);
    modalH = e9ui_scale_px(ctx, 180);
    if (modalW < 1) {
        modalW = 1;
    }
    if (modalH < 1) {
        modalH = 1;
    }
    x = (ctx->winW - modalW) / 2;
    y = (ctx->winH - modalH) / 2;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    rect = (e9ui_rect_t){ x, y, modalW, modalH };
    plan->modal = e9ui_modal_show(ctx, "RAM dump files exist", rect, amiga_memsave_saveModalClosed, plan);
    if (!plan->modal) {
        return 0;
    }
    body = amiga_memsave_makeSaveCollisionModalBody(plan);
    if (!body) {
        amiga_memsave_requestCloseSaveModal(plan);
        return 1;
    }
    e9ui_modal_setBodyChild(plan->modal, body, ctx);
    return 1;
}

void
amiga_memsave_onSave(e9ui_context_t *ctx, void *user)
{
    target_memory_range_t ranges[AMIGA_MEMSAVE_EXPORT_MAX_RANGES];
    char defaultDir[PATH_MAX];
    char fileName[PATH_MAX];
    char regsFileName[PATH_MAX];
    char configName[PATH_MAX];
    amiga_memsave_save_plan_t *plan = NULL;
    const char *folder = NULL;
    const char *configPath = NULL;
    const char *defaultPath = NULL;
    size_t rangeCount = 0u;
    int overwriteCount = 0;

    (void)user;
    if (!ctx) {
        return;
    }

    configPath = libretro_host_getRomPath();
    if (!configPath || !amiga_memsave_copyConfigName(configName, sizeof(configName), configPath)) {
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return;
    }
    if (!amiga_memsave_collectExportRanges(ranges, countof(ranges), &rangeCount) || rangeCount == 0u) {
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return;
    }

    if (debugger.libretro.saveDir[0] && settings_pathExistsDir(debugger.libretro.saveDir)) {
        defaultPath = debugger.libretro.saveDir;
    } else if (configPath && configPath[0]) {
        defaultPath = configPath;
    } else if (platform_getCurrentDir(defaultDir, sizeof(defaultDir))) {
        defaultPath = defaultDir;
    } else {
        defaultPath = ".";
    }
    folder = platform_selectFolderDialog("Select RAM dump folder", defaultPath);
    if (!folder || !folder[0]) {
        return;
    }
    if (!settings_pathExistsDir(folder)) {
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return;
    }

    plan = (amiga_memsave_save_plan_t*)alloc_calloc(1, sizeof(*plan));
    if (!plan) {
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return;
    }
    plan->rangeCount = rangeCount;
    memcpy(plan->ranges, ranges, sizeof(ranges[0]) * rangeCount);

    for (int ramType = 0; ramType < amiga_memsave_ram_type_count; ++ramType) {
        int present = 0;

        for (size_t i = 0; i < rangeCount; ++i) {
            if (ranges[i].size != 0u && amiga_memsave_ramTypeFromBaseAddr(ranges[i].baseAddr) == ramType) {
                present = 1;
                break;
            }
        }
        if (!present) {
            continue;
        }
        if (!amiga_memsave_buildExportFileName(fileName, sizeof(fileName), configName, ramType) ||
            !debugger_platform_pathJoin(plan->exportPaths[ramType],
                                        sizeof(plan->exportPaths[ramType]),
                                        folder,
                                        fileName)) {
            alloc_free(plan);
            e9ui_showTransientMessage("RAM SAVE FAILED");
            return;
        }
        plan->exportTypes[plan->exportTypeCount++] = ramType;
        if (settings_pathExistsFile(plan->exportPaths[ramType])) {
            if (!plan->collisionMessage[0] &&
                !amiga_memsave_buildCollisionMessage(plan->collisionMessage,
                                                     sizeof(plan->collisionMessage),
                                                     plan->exportPaths[ramType])) {
                alloc_free(plan);
                e9ui_showTransientMessage("RAM SAVE FAILED");
                return;
            }
            if (!plan->sequenceButtonLabel[0] &&
                !amiga_memsave_buildSequenceButtonLabel(plan->sequenceButtonLabel,
                                                        sizeof(plan->sequenceButtonLabel),
                                                        plan->exportPaths[ramType])) {
                alloc_free(plan);
                e9ui_showTransientMessage("RAM SAVE FAILED");
                return;
            }
            overwriteCount++;
        }
    }

    if (plan->exportTypeCount == 0) {
        alloc_free(plan);
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return;
    }

    if (!amiga_memsave_buildCustomRegsFileName(regsFileName, sizeof(regsFileName), configName) ||
        !debugger_platform_pathJoin(plan->customRegsPath, sizeof(plan->customRegsPath), folder, regsFileName)) {
        alloc_free(plan);
        e9ui_showTransientMessage("RAM SAVE FAILED");
        return;
    }
    if (settings_pathExistsFile(plan->customRegsPath)) {
        if (!plan->collisionMessage[0] &&
            !amiga_memsave_buildCollisionMessage(plan->collisionMessage,
                                                 sizeof(plan->collisionMessage),
                                                 plan->customRegsPath)) {
            alloc_free(plan);
            e9ui_showTransientMessage("RAM SAVE FAILED");
            return;
        }
        if (!plan->sequenceButtonLabel[0] &&
            !amiga_memsave_buildSequenceButtonLabel(plan->sequenceButtonLabel,
                                                    sizeof(plan->sequenceButtonLabel),
                                                    plan->customRegsPath)) {
            alloc_free(plan);
            e9ui_showTransientMessage("RAM SAVE FAILED");
            return;
        }
        overwriteCount++;
    }

    if (overwriteCount > 0) {
        if (!amiga_memsave_showSaveCollisionModal(ctx, plan)) {
            alloc_free(plan);
            e9ui_showTransientMessage("RAM SAVE FAILED");
            return;
        }
        return;
    }

    amiga_memsave_finishSavePlan(plan, 0);
    alloc_free(plan);
}
