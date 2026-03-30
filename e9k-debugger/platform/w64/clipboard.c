/*
 * COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
 *
 * https://github.com/alpine9000/engine9000-public
 *
 * See COPYING for license details
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#include "clipboard.h"

static void
clipboard_logWin32Error(const char *what)
{
    DWORD err = GetLastError();
    char msg[256] = {0};
    DWORD got = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0, msg, sizeof(msg), NULL);
    if (got == 0) {
        fprintf(stderr, "clipboard: %s failed with error %lu\n", what, (unsigned long)err);
        return;
    }
    for (DWORD i = got; i > 0; --i) {
        char c = msg[i - 1];
        if (c != '\r' && c != '\n') {
            break;
        }
        msg[i - 1] = '\0';
    }
    fprintf(stderr, "clipboard: %s failed with error %lu: %s\n", what, (unsigned long)err, msg);
}

static HGLOBAL
clipboard_makeDibV5Handle(const unsigned char *buffer, const png_image *img, size_t rowbytes, size_t totalBytes)
{
    if (!buffer || !img) {
        return NULL;
    }
    size_t dibSize = sizeof(BITMAPV5HEADER) + totalBytes;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dibSize);
    if (!hMem) {
        clipboard_logWin32Error("GlobalAlloc");
        return NULL;
    }
    void *ptr = GlobalLock(hMem);
    if (!ptr) {
        clipboard_logWin32Error("GlobalLock");
        GlobalFree(hMem);
        return NULL;
    }
    BITMAPV5HEADER *bih = (BITMAPV5HEADER *)ptr;
    ZeroMemory(bih, sizeof(*bih));
    bih->bV5Size = sizeof(BITMAPV5HEADER);
    bih->bV5Width = (LONG)img->width;
    bih->bV5Height = -((LONG)img->height);
    bih->bV5Planes = 1;
    bih->bV5BitCount = 32;
    bih->bV5Compression = BI_BITFIELDS;
    bih->bV5SizeImage = (DWORD)totalBytes;
    bih->bV5RedMask = 0x00FF0000;
    bih->bV5GreenMask = 0x0000FF00;
    bih->bV5BlueMask = 0x000000FF;
    bih->bV5AlphaMask = 0xFF000000;
    bih->bV5CSType = 0x73524742;
    bih->bV5Intent = LCS_GM_IMAGES;
    unsigned char *dst = (unsigned char *)(bih + 1);
    memcpy(dst, buffer, rowbytes * img->height);
    GlobalUnlock(hMem);
    return hMem;
}

static HGLOBAL
clipboard_makeDibHandle(const unsigned char *buffer, const png_image *img, size_t rowbytes, size_t totalBytes)
{
    if (!buffer || !img) {
        return NULL;
    }
    size_t dibSize = sizeof(BITMAPINFOHEADER) + totalBytes;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dibSize);
    if (!hMem) {
        clipboard_logWin32Error("GlobalAlloc");
        return NULL;
    }
    void *ptr = GlobalLock(hMem);
    if (!ptr) {
        clipboard_logWin32Error("GlobalLock");
        GlobalFree(hMem);
        return NULL;
    }
    BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)ptr;
    ZeroMemory(bih, sizeof(*bih));
    bih->biSize = sizeof(*bih);
    bih->biWidth = (LONG)img->width;
    bih->biHeight = (LONG)img->height;
    bih->biPlanes = 1;
    bih->biBitCount = 32;
    bih->biCompression = BI_RGB;
    bih->biSizeImage = (DWORD)totalBytes;

    unsigned char *dst = (unsigned char *)(bih + 1);
    for (png_uint_32 y = 0; y < img->height; ++y) {
        const unsigned char *srcRow = buffer + ((size_t)(img->height - 1u - y) * rowbytes);
        unsigned char *dstRow = dst + ((size_t)y * rowbytes);
        memcpy(dstRow, srcRow, rowbytes);
    }
    GlobalUnlock(hMem);
    return hMem;
}

int
clipboard_setPng(const void *png_data, size_t png_size)
{
    if (!png_data || png_size == 0) {
        return 0;
    }
    png_image img;
    memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&img, png_data, png_size)) {
        fprintf(stderr, "clipboard: libpng read error: %s\n", img.message);
        return 0;
    }
    img.format = PNG_FORMAT_BGRA;
    size_t rowbytes = PNG_IMAGE_ROW_STRIDE(img);
    size_t total_bytes = rowbytes * img.height;
    unsigned char *buffer = (unsigned char *)malloc(total_bytes);
    if (!buffer) {
        png_image_free(&img);
        return 0;
    }
    if (!png_image_finish_read(&img, NULL, buffer, rowbytes, NULL)) {
        fprintf(stderr, "clipboard: libpng finish read error: %s\n", img.message);
        png_image_free(&img);
        free(buffer);
        return 0;
    }
    HGLOBAL hMemDibV5 = clipboard_makeDibV5Handle(buffer, &img, rowbytes, total_bytes);
    HGLOBAL hMemDib = clipboard_makeDibHandle(buffer, &img, rowbytes, total_bytes);
    free(buffer);
    if (!hMemDibV5 && !hMemDib) {
        return 0;
    }
    if (!OpenClipboard(NULL)) {
        clipboard_logWin32Error("OpenClipboard");
        if (hMemDibV5) {
            GlobalFree(hMemDibV5);
        }
        if (hMemDib) {
            GlobalFree(hMemDib);
        }
        return 0;
    }
    if (!EmptyClipboard()) {
        clipboard_logWin32Error("EmptyClipboard");
        CloseClipboard();
        if (hMemDibV5) {
            GlobalFree(hMemDibV5);
        }
        if (hMemDib) {
            GlobalFree(hMemDib);
        }
        return 0;
    }

    int ok = 0;
    if (hMemDib && SetClipboardData(CF_DIB, hMemDib)) {
        ok = 1;
        hMemDib = NULL;
    } else if (hMemDib) {
        clipboard_logWin32Error("SetClipboardData(CF_DIB)");
    }

    if (hMemDibV5 && SetClipboardData(CF_DIBV5, hMemDibV5)) {
        ok = 1;
        hMemDibV5 = NULL;
    } else if (hMemDibV5) {
        clipboard_logWin32Error("SetClipboardData(CF_DIBV5)");
    }

    CloseClipboard();
    if (hMemDibV5) {
        GlobalFree(hMemDibV5);
    }
    if (hMemDib) {
        GlobalFree(hMemDib);
    }
    return ok;
}
