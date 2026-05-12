/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#pragma once

#include <stdint.h>

typedef int (*console_cmd_handler_t)(int argc, char **argv);
typedef int (*console_cmd_complete_t)(const char *prefix, char ***outList, int *outCount);

int
console_cmd_parseHex(const char *s, uint32_t *out);

int
console_cmd_parseHexStrict(const char *s, uint64_t *out, int *outDigits);

int
console_cmd_parseU32Strict(const char *s, uint32_t *out);

int
console_cmd_parseU32Auto(const char *s, uint32_t *out);

int
console_cmd_parseU64Dec(const char *s, uint64_t *out);

int
console_cmd_parseSizeBitsOpt(const char *tok, uint32_t *outSizeBits);

int
console_cmd_base_command(int argc, char **argv);

int
console_cmd_break_command(int argc, char **argv);

int
console_cmd_break_complete(const char *prefix, char ***outList, int *outCount);

int
console_cmd_cls_command(int argc, char **argv);

int
console_cmd_continue_command(int argc, char **argv);

int
console_cmd_print_command(int argc, char **argv);

int
console_cmd_print_complete(const char *prefix, char ***outList, int *outCount);

int
console_cmd_symbols_command(int argc, char **argv);

int
console_cmd_next_command(int argc, char **argv);

int
console_cmd_finish_command(int argc, char **argv);

int
console_cmd_step_command(int argc, char **argv);

int
console_cmd_stepi_command(int argc, char **argv);

int
console_cmd_transition_command(int argc, char **argv);

int
console_cmd_transition_complete(const char *prefix, char ***outList, int *outCount);

const char *
console_cmd_watch_usage(void);

int
console_cmd_watch_command(int argc, char **argv);

int
console_cmd_train_command(int argc, char **argv);

int
console_cmd_loop_command(int argc, char **argv);

int
console_cmd_protect_command(int argc, char **argv);

int
console_cmd_diff_command(int argc, char **argv);

int
console_cmd_write_command(int argc, char **argv);

int
console_cmd_write_complete(const char *prefix, char ***outList, int *outCount);
