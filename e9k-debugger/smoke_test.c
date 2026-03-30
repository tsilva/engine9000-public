/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <SDL.h>
#include <SDL_image.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "smoke_test.h"
#include "debugger.h"
#include "debug.h"
#include "libretro_host.h"
#include "ui_test.h"
#include "strutil.h"

static char smoke_test_folder[PATH_MAX];
static int smoke_test_enabled = 0;
static smoke_test_mode_t smoke_test_mode = SMOKE_TEST_MODE_NONE;
static int smoke_test_openOnFail = 0;

void
smoke_test_setFolder(const char *path)
{
    if (!path || !*path) {
        smoke_test_folder[0] = '\0';
        smoke_test_enabled = 0;
        return;
    }
    strutil_strlcpy(smoke_test_folder, sizeof(smoke_test_folder), path);
}

void
smoke_test_setMode(smoke_test_mode_t mode)
{
    smoke_test_mode = mode;
}

smoke_test_mode_t
smoke_test_getMode(void)
{
    return smoke_test_mode;
}

void
smoke_test_setOpenOnFail(int enable)
{
    smoke_test_openOnFail = enable ? 1 : 0;
}

static int
smoke_test_makeDir(const char *path)
{
    return debugger_platform_makeDir(path);
}

static int
smoke_test_hasSuffix(const char *name, const char *suffix)
{
    if (!name || !suffix) {
        return 0;
    }
    size_t nameLen = strlen(name);
    size_t suffixLen = strlen(suffix);
    if (suffixLen == 0 || nameLen < suffixLen) {
        return 0;
    }
    const char *tail = name + (nameLen - suffixLen);
    if (!debugger_platform_caseInsensitivePaths()) {
        return strcmp(tail, suffix) == 0 ? 1 : 0;
    }
    for (size_t i = 0; i < suffixLen; ++i) {
        if (tolower((unsigned char)tail[i]) != tolower((unsigned char)suffix[i])) {
            return 0;
        }
    }
    return 1;
}

static int
smoke_test_clearFolderEntry(const char *path, void *user)
{
    int clearInputs = user ? (*(const int *)user) : 0;
    if (!path || !*path) {
        return 1;
    }
    const char *slash = strrchr(path, '/');
    const char *back = strrchr(path, '\\');
    const char *base = slash > back ? slash : back;
    const char *name = base ? base + 1 : path;
    if (smoke_test_hasSuffix(name, ".png") ||
        (clearInputs && smoke_test_hasSuffix(name, ".inp"))) {
        remove(path);
    }
    return 1;
}

static void
smoke_test_clearFolder(const char *path, int clearInputs)
{
    if (!path || !*path) {
        return;
    }
    debugger_platform_scanFolder(path, smoke_test_clearFolderEntry, &clearInputs);
}

int
smoke_test_init(void)
{
    if (!smoke_test_folder[0]) {
        smoke_test_enabled = 0;
        return 1;
    }
    if (!smoke_test_makeDir(smoke_test_folder)) {
        debug_error("smoke-test: failed to create folder %s", smoke_test_folder);
        smoke_test_enabled = 0;
        return 0;
    }
    if (smoke_test_mode == SMOKE_TEST_MODE_RECORD) {
        smoke_test_clearFolder(smoke_test_folder, 1);
    } else if (smoke_test_mode == SMOKE_TEST_MODE_REMAKE) {
        smoke_test_clearFolder(smoke_test_folder, 0);
    }
    if (smoke_test_mode == SMOKE_TEST_MODE_NONE) {
        smoke_test_enabled = 0;
        return 1;
    }
    smoke_test_enabled = 1;
    return 1;
}

void
smoke_test_shutdown(void)
{
    smoke_test_enabled = 0;
    smoke_test_mode = SMOKE_TEST_MODE_NONE;
    smoke_test_openOnFail = 0;
}

int
smoke_test_isEnabled(void)
{
    return smoke_test_enabled ? 1 : 0;
}

int
smoke_test_getRecordPath(char *out, size_t cap)
{
    if (!out || cap == 0 || !smoke_test_folder[0]) {
        return 0;
    }
    return debugger_platform_pathJoin(out, cap, smoke_test_folder, "smoketest.inp");
}

void
smoke_test_reset(struct e9k_debugger *dbg)
{
    if (!dbg) {
        return;
    }
    dbg->smokeTestPath[0] = '\0';
    dbg->smokeTestMode = SMOKE_TEST_MODE_NONE;
    dbg->smokeTestCompleted = 0;
    dbg->smokeTestFailed = 0;
    dbg->smokeTestExitCode = -1;
    dbg->smokeTestOpenOnFail = 0;
}

int
smoke_test_bootstrap(struct e9k_debugger *dbg)
{
    if (!dbg) {
        return 0;
    }
    if (dbg->smokeTestMode == SMOKE_TEST_MODE_NONE) {
        return 1;
    }
    if (ui_test_getMode() != UI_TEST_MODE_NONE) {
        debug_error("test: cannot combine --make-test/--remake-test/--test with smoke test options");
        return 0;
    }
    if (dbg->smokeTestMode == SMOKE_TEST_MODE_COMPARE ||
        dbg->smokeTestMode == SMOKE_TEST_MODE_REMAKE) {
        dbg->speedMultiplier = 10;
    }
    if (dbg->smokeTestMode == SMOKE_TEST_MODE_RECORD) {
        if (dbg->playbackPath[0]) {
            debug_error("make-smoke: cannot use --playback with --make-smoke");
            return 0;
        }
    } else if (dbg->smokeTestMode == SMOKE_TEST_MODE_COMPARE ||
               dbg->smokeTestMode == SMOKE_TEST_MODE_REMAKE) {
        if (dbg->recordPath[0] || dbg->playbackPath[0]) {
            debug_error("%s: cannot combine with --record or --playback",
                        dbg->smokeTestMode == SMOKE_TEST_MODE_REMAKE ? "remake-smoke" : "smoke-test");
            return 0;
        }
    }
    smoke_test_setFolder(dbg->smokeTestPath);
    smoke_test_setMode((smoke_test_mode_t)dbg->smokeTestMode);
    smoke_test_setOpenOnFail(dbg->smokeTestOpenOnFail);
    if (!smoke_test_init()) {
        return 0;
    }
    char path[PATH_MAX];
    if (smoke_test_getRecordPath(path, sizeof(path))) {
        if (dbg->smokeTestMode == SMOKE_TEST_MODE_RECORD) {
            strutil_strlcpy(dbg->recordPath, sizeof(dbg->recordPath), path);
        } else if (dbg->smokeTestMode == SMOKE_TEST_MODE_COMPARE ||
                   dbg->smokeTestMode == SMOKE_TEST_MODE_REMAKE) {
            strutil_strlcpy(dbg->playbackPath, sizeof(dbg->playbackPath), path);
        }
    }
    return 1;
}

int
smoke_test_getExitCode(const struct e9k_debugger *dbg)
{
    if (!dbg) {
        return -1;
    }
    return dbg->smokeTestExitCode;
}

void
smoke_test_cleanup(void)
{
    smoke_test_shutdown();
}

static int
smoke_test_writeDiffImage(uint64_t frame, const uint8_t *data, int width, int height, size_t pitch,
                          char *outPath, size_t cap)
{
    char name[64];
    snprintf(name, sizeof(name), "diff-%llu.png", (unsigned long long)frame);
    char path[PATH_MAX];
    if (!debugger_platform_pathJoin(path, sizeof(path), smoke_test_folder, name)) {
        return 0;
    }
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        (void *)data, width, height, 32, (int)pitch, SDL_PIXELFORMAT_XRGB8888);
    if (!surface) {
        return 0;
    }
    if (IMG_SavePNG(surface, path) == 0 && outPath && cap > 0) {
        strncpy(outPath, path, cap - 1);
        outPath[cap - 1] = '\0';
    }
    SDL_FreeSurface(surface);
    return 1;
}

static int
smoke_test_writeDiffScript(uint64_t frame, const char *refPath, char *outMontage, size_t cap)
{
    if (!refPath || !*refPath) {
        return 0;
    }
    char testName[64];
    snprintf(testName, sizeof(testName), "diff-%llu.png", (unsigned long long)frame);
    char testPath[PATH_MAX];
    if (!debugger_platform_pathJoin(testPath, sizeof(testPath), smoke_test_folder, testName)) {
        return 0;
    }
    char compareName[64];
    snprintf(compareName, sizeof(compareName), "diff-%llu-compare.png", (unsigned long long)frame);
    char comparePath[PATH_MAX];
    if (!debugger_platform_pathJoin(comparePath, sizeof(comparePath), smoke_test_folder, compareName)) {
        return 0;
    }
    char montageName[64];
    snprintf(montageName, sizeof(montageName), "diff-%llu-triple.png", (unsigned long long)frame);
    char montagePath[PATH_MAX];
    if (!debugger_platform_pathJoin(montagePath, sizeof(montagePath), smoke_test_folder, montageName)) {
        return 0;
    }
    char scriptName[64];
    snprintf(scriptName, sizeof(scriptName), "diff-%llu.cmd", (unsigned long long)frame);
    char scriptPath[PATH_MAX];
    if (!debugger_platform_pathJoin(scriptPath, sizeof(scriptPath), smoke_test_folder, scriptName)) {
        return 0;
    }
    FILE *fp = fopen(scriptPath, "w");
    if (!fp) {
        return 0;
    }
    fprintf(fp, "magick compare -metric AE \"%s\" \"%s\" \"%s\"\n",
            refPath, testPath, comparePath);
    fprintf(fp, "magick montage \"%s\" \"%s\" \"%s\" -tile 3x1 -geometry +0+0 \"%s\"\n",
            refPath, testPath, comparePath, montagePath);
    fclose(fp);
    char cmdCompare[PATH_MAX * 3];
    char cmdMontage[PATH_MAX * 4];
    snprintf(cmdCompare, sizeof(cmdCompare),
             "magick compare -metric AE \"%s\" \"%s\" \"%s\"",
             refPath, testPath, comparePath);
    snprintf(cmdMontage, sizeof(cmdMontage),
             "magick montage \"%s\" \"%s\" \"%s\" -tile 3x1 -geometry +0+0 \"%s\"",
             refPath, testPath, comparePath, montagePath);
    int ignored = system(cmdCompare);
    ignored = system(cmdMontage);
    (void)ignored;
    if (outMontage && cap > 0) {
        strncpy(outMontage, montagePath, cap - 1);
        outMontage[cap - 1] = '\0';
    }
    return 1;
}

static void
smoke_test_openImage(const char *path)
{
    if (!path || !*path) {
        return;
    }
    char url[PATH_MAX + 16];
    snprintf(url, sizeof(url), "file://%s", path);
    if (SDL_OpenURL(url) != 0) {
        SDL_OpenURL(path);
    }
}

static int
smoke_test_compareFrame(uint64_t frame, const uint8_t *data, int width, int height, size_t pitch)
{
    char diffPath[PATH_MAX];
    char montagePath[PATH_MAX];
    diffPath[0] = '\0';
    montagePath[0] = '\0';
    char name[64];
    snprintf(name, sizeof(name), "%llu.png", (unsigned long long)frame);
    char path[PATH_MAX];
    if (!debugger_platform_pathJoin(path, sizeof(path), smoke_test_folder, name)) {
        smoke_test_writeDiffImage(frame, data, width, height, pitch, diffPath, sizeof(diffPath));
        smoke_test_writeDiffScript(frame, path, montagePath, sizeof(montagePath));
        if (smoke_test_openOnFail && montagePath[0]) {
            smoke_test_openImage(montagePath);
        }
        debug_printf("Smoke test failed at frame #%llu (%s)", (unsigned long long)frame,
                     montagePath[0] ? montagePath : (diffPath[0] ? diffPath : "diff unavailable"));
        return 1;
    }
    struct stat st;
    if (stat(path, &st) != 0) {
        if (errno == ENOENT) {
            return 2;
        }
        debug_error("smoke-test: stat failed for %s", path);
        smoke_test_writeDiffImage(frame, data, width, height, pitch, diffPath, sizeof(diffPath));
        smoke_test_writeDiffScript(frame, path, montagePath, sizeof(montagePath));
        if (smoke_test_openOnFail && montagePath[0]) {
            smoke_test_openImage(montagePath);
        }
        debug_printf("Smoke test failed at frame #%llu (%s)", (unsigned long long)frame,
                     montagePath[0] ? montagePath : (diffPath[0] ? diffPath : "diff unavailable"));
        return 1;
    }
    SDL_Surface *loaded = IMG_Load(path);
    if (!loaded) {
        debug_error("smoke-test: IMG_Load failed: %s", IMG_GetError());
        smoke_test_writeDiffImage(frame, data, width, height, pitch, diffPath, sizeof(diffPath));
        smoke_test_writeDiffScript(frame, path, montagePath, sizeof(montagePath));
        if (smoke_test_openOnFail && montagePath[0]) {
            smoke_test_openImage(montagePath);
        }
        debug_printf("Smoke test failed at frame #%llu (%s)", (unsigned long long)frame,
                     montagePath[0] ? montagePath : (diffPath[0] ? diffPath : "diff unavailable"));
        return 1;
    }
    SDL_Surface *converted = loaded;
    if (loaded->format->format != SDL_PIXELFORMAT_XRGB8888) {
        converted = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_XRGB8888, 0);
        SDL_FreeSurface(loaded);
    }
    if (!converted) {
        debug_error("smoke-test: SDL_ConvertSurfaceFormat failed: %s", SDL_GetError());
        smoke_test_writeDiffImage(frame, data, width, height, pitch, diffPath, sizeof(diffPath));
        smoke_test_writeDiffScript(frame, path, montagePath, sizeof(montagePath));
        if (smoke_test_openOnFail && montagePath[0]) {
            smoke_test_openImage(montagePath);
        }
        debug_printf("Smoke test failed at frame #%llu (%s)", (unsigned long long)frame,
                     montagePath[0] ? montagePath : (diffPath[0] ? diffPath : "diff unavailable"));
        return 1;
    }
    int fail = 0;
            if (SDL_MUSTLOCK(converted)) {
            if (SDL_LockSurface(converted) != 0) {
                debug_error("smoke-test: SDL_LockSurface failed: %s", SDL_GetError());
                smoke_test_writeDiffImage(frame, data, width, height, pitch, diffPath, sizeof(diffPath));
                smoke_test_writeDiffScript(frame, path, montagePath, sizeof(montagePath));
                if (smoke_test_openOnFail && montagePath[0]) {
                    smoke_test_openImage(montagePath);
                }
                debug_printf("Smoke test failed at frame #%llu (%s)", (unsigned long long)frame,
                             montagePath[0] ? montagePath : (diffPath[0] ? diffPath : "diff unavailable"));
                SDL_FreeSurface(converted);
                return 1;
            }
        }
    if (converted->w != width || converted->h != height) {
        fail = 1;
    } else {
        const uint8_t *src = (const uint8_t *)converted->pixels;
        const uint32_t mask = 0x00ffffffu;
        for (int y = 0; y < height && !fail; ++y) {
            const uint8_t *row_a = src + (size_t)y * (size_t)converted->pitch;
            const uint8_t *row_b = data + (size_t)y * pitch;
            for (int x = 0; x < width; ++x) {
                const uint32_t *pa = (const uint32_t *)(row_a + (size_t)x * 4);
                const uint32_t *pb = (const uint32_t *)(row_b + (size_t)x * 4);
                if (((*pa) & mask) != ((*pb) & mask)) {
                    fail = 1;
                    break;
                }
            }
        }
    }
    if (SDL_MUSTLOCK(converted)) {
        SDL_UnlockSurface(converted);
    }
    SDL_FreeSurface(converted);
    if (fail) {
        smoke_test_writeDiffImage(frame, data, width, height, pitch, diffPath, sizeof(diffPath));
        smoke_test_writeDiffScript(frame, path, montagePath, sizeof(montagePath));
        if (smoke_test_openOnFail && montagePath[0]) {
            smoke_test_openImage(montagePath);
        }
        debug_printf("Smoke test failed at frame #%llu (%s)", (unsigned long long)frame,
                     montagePath[0] ? montagePath : (diffPath[0] ? diffPath : "diff unavailable"));
        return 1;
    }
    return 0;
}

int
smoke_test_captureFrame(uint64_t frame)
{
    if (!smoke_test_enabled) {
        return 0;
    }
    const uint8_t *data = NULL;
    int width = 0;
    int height = 0;
    size_t pitch = 0;
    if (!libretro_host_getFrame(&data, &width, &height, &pitch)) {
        return 0;
    }
    if (smoke_test_mode == SMOKE_TEST_MODE_COMPARE) {
        return smoke_test_compareFrame(frame, data, width, height, pitch);
    }
    char name[64];
    snprintf(name, sizeof(name), "%llu.png", (unsigned long long)frame);
    char path[PATH_MAX];
    if (!debugger_platform_pathJoin(path, sizeof(path), smoke_test_folder, name)) {
        return 0;
    }
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        (void *)data, width, height, 32, (int)pitch, SDL_PIXELFORMAT_XRGB8888);
    if (!surface) {
        debug_error("smoke-test: SDL_CreateRGBSurfaceWithFormatFrom failed: %s", SDL_GetError());
        return 0;
    }
    if (IMG_SavePNG(surface, path) != 0) {
        debug_error("smoke-test: IMG_SavePNG failed: %s", IMG_GetError());
    }
    SDL_FreeSurface(surface);
    return 0;
}
