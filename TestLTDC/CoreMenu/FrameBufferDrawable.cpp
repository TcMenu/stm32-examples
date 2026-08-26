/**
* Adafruit based FrameBuffer support class. This class provides the minimal wrappers for high performance
 * drawing extending from our frame buffer class. The base class implements all operations using either
 * draw pixel or slightly optimised line drawing. This version accelerates the most common methods using
 * DMA2D techniques.
 *
 * It is provided to you as a starting point, and is based on the StmCube BSP packages for the board. So,
 * unless you have the exact same hardware configuration it is likely to require modification. For example,
 * if you use the same processor but a different configuration of the hardware, you may need to modify this
 * class or the BSP package to suit your needs.
 *
 * Words of caution, this is a high performance driver that is intended for senior developers with enough
 * experience to understand the limitations of the hardware and the trade-offs of the acceleration techniques.
 *
 * Round-trip/Local filesystem notes: It will not be overwritten by designer in local files. If you want to go
 * back to the original then delete this file and regenerate it from the designer.
 */


#include "FrameBufferDrawable.h"
#include "stm32f4xx_hal.h"
#include <algorithm>

// Tell the compiler that we have a DMA2d handle available to us
extern DMA2D_HandleTypeDef hdma2d;

void StmDMA2dAdafruitFrameBuffer16::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color) {
    if (w <= 0 || h <= 0) return;

    Coord start, end;
    if (!this->correctDimensions(x, y, start) ||
        !this->correctDimensions(x + w - 1, y + h - 1, end)) {
        return;
    }

    // Use DMA2d to fill the rectangle, wait for it to be available if needed
    HAL_DMA2D_PollForTransfer(&hdma2d, 100);

    int16_t x_min = std::min(start.x, end.x);
    int16_t x_max = std::max(start.x, end.x);
    int16_t y_min = std::min(start.y, end.y);
    int16_t y_max = std::max(start.y, end.y);

    uint32_t width = (x_max - x_min) + 1;
    uint32_t height = (y_max - y_min) + 1;
    auto dstAddress = reinterpret_cast<uint32_t>(&this->buffer[x_min + (y_min * this->WIDTH)]);
    // Prepare 16-bit RGB565 color for DMA2D R2M mode
    uint32_t dmaColor = static_cast<uint32_t>((color & 0xF800) << 8) |
        static_cast<uint32_t>((color & 0x07E0) << 5) |
        static_cast<uint32_t>((color & 0x001F) << 3);

    hdma2d.Init.Mode = DMA2D_R2M;
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
    hdma2d.Init.OutputOffset = WIDTH - width;

    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK) {
        if (HAL_DMA2D_Start(&hdma2d, dmaColor, dstAddress, width, height) == HAL_OK) {
            HAL_DMA2D_PollForTransfer(&hdma2d, 100);
        }
    }
}

void StmDMA2dAdafruitFrameBuffer16::writeFastVLine(int16_t x, int16_t y, int16_t h, color_t color) {
    writeFillRect(x, y, 1, h, color);
}

void StmDMA2dAdafruitFrameBuffer16::writeFastHLine(int16_t x, int16_t y, int16_t w, color_t color) {
    writeFillRect(x, y, w, 1, color);
}

// ------------------------------------------------------------------
// The drawable follows, it maps the framebuffer to the menu library

void TcAdafruitFrameDrawable::drawBitmap(const Coord& where, const DrawableIcon* icon, bool selected) {
    if (icon->getIconType() == DrawableIcon::ICON_XBITMAP) {
        graphics->fillRect(where.x, where.y, icon->getDimensions().x, icon->getDimensions().y, backgroundColor);
        graphics->drawXBitmap(where.x, where.y, icon->getIcon(selected), icon->getDimensions().x,
                              icon->getDimensions().y, drawColor);
    }
    else if (icon->getIconType() == DrawableIcon::ICON_NATIVE) {
        graphics->drawRGBBitmap(where.x, where.y, (const uint16_t*)icon->getIcon(selected), icon->getDimensions().x,
                                icon->getDimensions().y);
    }
    else if (icon->getIconType() == DrawableIcon::ICON_MONO) {
        graphics->drawBitmap(where.x, where.y, icon->getIcon(selected), icon->getDimensions().x,
                             icon->getDimensions().y, drawColor, backgroundColor);
    }
    else if (icon->getPalette() != nullptr) {
        auto bpp = icon->getIconType() == tcgfx::DrawableIcon::ICON_PALLETE_2BPP ? 2 : 4;
        drawBitmapNbpp(where, icon->getIcon(selected), icon->getDimensions(), bpp, icon->getPalette());
    }
}

void TcAdafruitFrameDrawable::drawXBitmap(const Coord& where, const Coord& size, const uint8_t* data) {
    graphics->fillRect(where.x, where.y, size.x, size.y, backgroundColor);
    graphics->drawXBitmap(where.x, where.y, data, size.x, size.y, drawColor);
}

void TcAdafruitFrameDrawable::drawBitmapNbpp(const Coord& where, const uint8_t* data, const Coord& size, int bpp,
                                             const color_t* palette) {
    graphics->drawBitmapNBpp(where, data, size, bpp, palette);
}

void TcAdafruitFrameDrawable::drawBox(const Coord& where, const Coord& size, bool filled) {
    if (filled) {
        graphics->fillRect(where.x, where.y, size.x, size.y, drawColor);
    }
    else {
        graphics->drawRect(where.x, where.y, size.x, size.y, drawColor);
    }
}

void TcAdafruitFrameDrawable::drawRoundRect(const Coord& where, const Coord& size, int radius, bool filled) {
    if (filled) {
        graphics->fillRoundRect(where.x, where.y, size.x, size.y, static_cast<int16_t>(radius), drawColor);
    }
    else {
        graphics->drawRoundRect(where.x, where.y, size.x, size.y, static_cast<int16_t>(radius), drawColor);
    }
}

void TcAdafruitFrameDrawable::drawCircle(const Coord& where, int radius, bool filled) {
    if (filled) {
        graphics->fillCircle(where.x, where.y, static_cast<int16_t>(radius), drawColor);
    }
    else {
        graphics->drawCircle(where.x, where.y, static_cast<int16_t>(radius), drawColor);
    }
}

void TcAdafruitFrameDrawable::drawPolygon(const Coord points[], int numPoints, bool filled) {
    // not implemented
}

void TcAdafruitFrameDrawable::drawPixel(uint16_t x, uint16_t y) {
    graphics->drawPixel(x, y, drawColor);
}




void TcAdafruitFrameDrawable::internalDrawText(const Coord &where, const void *font, int mag, const char *sz) {
    graphics->setTextWrap(false);
    int baseline=0;
    Coord exts = textExtents(font, mag, "(;y", &baseline);
    int yCursor = font ? (where.y + (exts.y - baseline)) : where.y;
    graphics->setCursor(where.x, yCursor);
    graphics->setTextColor(drawColor);
    graphics->print(sz);
}

Coord TcAdafruitFrameDrawable::internalTextExtents(const void *f, int mag, const char *text, int *baseline) {
    if(mag == 0) mag = 1; // never allow 0 magnification

    graphics->setFont(static_cast<const GFXfont *>(f));
    graphics->setTextSize(mag);
    auto* font = (GFXfont *) f;
    int16_t x1, y1;
    uint16_t w, h;
    graphics->getTextBounds((char*)text, 3, font?30:2, &x1, &y1, &w, &h);

    if(font == nullptr) {
        // for the default font, the starting offset is 0, and we calculate the height.
        if(baseline) *baseline = 0;
        return Coord(w, h);
    }
    else {
        computeBaselineIfNeeded(font);
        if(baseline) *baseline = (computedBaseline * mag);
        return Coord(int(w), (computedHeight * mag));
    }
}

void TcAdafruitFrameDrawable::computeBaselineIfNeeded(const GFXfont* font) {
    // we cache the last baseline, if the font is unchanged, don't calculate again
    if(computedFont == font && computedBaseline > 0) return;

    // we need to work out the biggest glyph and maximum extent beyond the baseline, we use 4 chars 'Agj(' for this
    const char sz[] = "Agj(";
    int height = 0;
    int bl = 0;
    const char* current = sz;
    auto fontLast = pgm_read_word(&font->last);
    auto fontFirst = pgm_read_word(&font->first);
    while(*current && (*current < fontLast)) {
        size_t glIdx = *current - fontFirst;
        auto allGlyphs = (GFXglyph*)pgm_read_ptr(&font->glyph);
        int glyphHeight = int(pgm_read_byte(&allGlyphs[glIdx].height));
        if (glyphHeight > height) height = glyphHeight;
        auto yOffset = int8_t(pgm_read_byte(&allGlyphs[glIdx].yOffset));
        bl += glyphHeight + yOffset;
        current++;
    }
    computedFont = font;
    computedBaseline = bl / 4;
    computedHeight = height;
}


