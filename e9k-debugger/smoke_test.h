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

typedef enum smoke_test_mode {
    SMOKE_TEST_MODE_NONE = 0,
    SMOKE_TEST_MODE_RECORD = 1,
    SMOKE_TEST_MODE_COMPARE = 2,
    SMOKE_TEST_MODE_REMAKE = 3
} smoke_test_mode_t;

struct e9k_debugger;

void
smoke_test_setFolder(const char *path);

void
smoke_test_setMode(smoke_test_mode_t mode);

smoke_test_mode_t
smoke_test_getMode(void);

void
smoke_test_setOpenOnFail(int enable);

void
smoke_test_setThreshold(int threshold);

int
smoke_test_init(void);

void
smoke_test_shutdown(void);

int
smoke_test_isEnabled(void);

void
smoke_test_setStartOnWrite(int enabled);

int
smoke_test_isWaitingForStart(void);

int
smoke_test_hasStarted(void);

int
smoke_test_startFromFrame(uint64_t frame);

int
smoke_test_getRecordPath(char *out, size_t cap);

void
smoke_test_reset(struct e9k_debugger *dbg);

int
smoke_test_bootstrap(struct e9k_debugger *dbg);

int
smoke_test_getExitCode(const struct e9k_debugger *dbg);

void
smoke_test_cleanup(void);

int
smoke_test_captureFrame(uint64_t frame);

int
smoke_test_finishScreenCompare(void);

void
smoke_test_setAudioFormat(int sampleRate, int channels);

int
smoke_test_captureAudio(const int16_t *data, size_t frames);

int
smoke_test_finishAudioCompare(void);
