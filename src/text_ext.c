#include "global.h"
#include "characters.h"
#include "main.h"
#include "text.h"
#include "window.h"

#define GLYPH_HEIGHT 16

static u32 gBppConvTable[256];

static const u8 gCHSFont1bpp[] = INCBIN_U8("graphics/fonts/gba_chs_font_11x11.bin");
static const u8 gCHSFont1bppExt[] __attribute__((section(".vramfont"))) = INCBIN_U8("graphics/fonts/gba_chs_font_11x11_tail.bin");
static const u8 gLatinFont1bpp[] = INCBIN_U8("graphics/fonts/gba_latin_font_8x16.bin");
static const u8 gLatinFont1bppWidthTable[] = INCBIN_U8("graphics/fonts/gba_latin_font_8x16_width_table.bin");
static const u8 gJPNFont1bpp[] = INCBIN_U8("graphics/fonts/gba_jpn_font_8x16.bin");
static const u8 sDownArrowTiles[] = INCBIN_U8("graphics/fonts/down_arrow.4bpp");
static const u8 sDownArrowYCoords[] = { 0, 1, 2, 1 };
static const u8 sTextDelays[] = {0, 8, 4, 1};
static const u8 sScrollSpeeds[] = {GLYPH_HEIGHT, 1, 2, 4};


struct TextDrawingContext {
    const u8 *currChar;
    u8 state;
    u8 windowId;
    u8 origLeft;
    u8 origTop;
    u8 left;
    u8 top;
    u8 bgColor;
    u8 fgColor;
    u8 shadowColor;
    u8 textSpeed;
    u8 delayCounter;
    u8 downArrowYCoordsIndex;
    bool8 isJpnChar;
    const u8 *currNarrowFont;
};

static struct TextDrawingContext sTextDrawingContext;

// colors bg fg shadow
static u8 DrawGlyph(u8 windowId, const u8 *glyph, u8 left, u8 top, u8 width, const u8 *colors) {
    struct Window *win = &gWindows[windowId];
    const u8 *shadow = glyph + 16;
    if (width > 8)
        width = 8;
    width = min(win->window.width * 8 - left, width);
    u32 mask = (1 << width * 4) - 1;
    for (int i = 0; i < GLYPH_HEIGHT && i + top < win->window.height * 8; i++) {
        u32 pixelValue = (gBppConvTable[glyph[i]] * colors[1]) | (gBppConvTable[shadow[i]] * colors[2]) |
                         (gBppConvTable[~(glyph[i] | shadow[i]) & 0xFF] * colors[0]);
        pixelValue &= mask;
        u32 *tilePtr = (u32 *)(win->tileData + win->window.width * TILE_SIZE_4BPP * ((top + i) / 8) + (left / 8) * TILE_SIZE_4BPP + (top + i) % 8 * 4);
        u32 shift = (left % 8) * 4;
        *tilePtr = (*tilePtr & ~(mask << shift)) | (pixelValue << shift);
        if (8 - left % 8 >= width) continue;
        tilePtr += TILE_HEIGHT;
        *tilePtr = (*tilePtr & ~(mask >> (32 - shift))) | (pixelValue >> (32 - shift));
    }
}

// size of dest must be 32 bytes
static void ConvertNarrowGlyph(const u8 *glyphBuff, u8 *dest) {
    memset(dest, 0, 32);
    for (int i = 0; i < GLYPH_HEIGHT; i++)
    {
        dest[i] = glyphBuff[i];
    }
    u8 *shadowBuff = dest + 16;
    glyphBuff = dest;
    u8 line = glyphBuff[0];
    u8 lineNextShadow = 0;
    for (int i = 0; i < GLYPH_HEIGHT - 1; i++)
    {
        u8 lineNext = glyphBuff[i + 1];
        u8 lineShadow = (line >> 1) | line;
        u8 shadow = ((lineShadow ^ line) | lineNextShadow);
        shadowBuff[i] = shadow;
        lineNextShadow = (lineShadow | lineNext) ^ lineNext;
        line = lineNext;
    }
    shadowBuff[GLYPH_HEIGHT - 1] = lineNextShadow;
}

// size of dest must be 64 bytes
static void ConvertWideGlyph(const u8 *source, u8 *dest, int glyphBoxWidth, int glyphBoxHeight, int xOffset)
{
  int index = 0;
  int remainBitCount = 8;
  int bitOffset = 0;
  u32 temp = 0;
  memset(dest, 0, 64);

  // step 1 convert 11x11-glyph to 16x16-glyph
  while (index < glyphBoxHeight)
  {
    int rightShift = remainBitCount - (glyphBoxWidth - bitOffset) > 0 ? remainBitCount - (glyphBoxWidth - bitOffset) : 0;
    int bitsLength = (remainBitCount - rightShift);
    int leftShift = 16 - bitOffset - bitsLength;
    temp = (*source >> rightShift << leftShift) | temp;
    bitOffset += bitsLength;
    remainBitCount -= bitsLength;
    if (remainBitCount == 0)
    {
      remainBitCount = 8;
      source++;
    }
    if (bitOffset == glyphBoxWidth)
    {
      //*(dest + index * 2) = (u8)((temp >> 8) & 0xFF);
      //*(dest + index * 2 + 1) = (u8)(temp & 0xFF);
      int hiOffset = index + xOffset;
      int loOffset = hiOffset + 16;
      *(dest + hiOffset) = (u8)((temp >> 8) & 0xFF);
      *(dest + loOffset) = (u8)(temp & 0xFF);
      index++;
      bitOffset = 0;
      temp = 0;
    }
  }

  // step 2 draw shadow
  const u8 *glyphBuff = dest;
  u8 *shadowBuff = dest + 32;
  u16 line = (glyphBuff[0] << 8) | glyphBuff[16];
  u16 lineNextShadow = 0;
  for (int i = 0; i < GLYPH_HEIGHT - 1; i++)
  {
    int hiOffset = i;
    int loOffset = hiOffset + 16;
    u16 lineNext = (glyphBuff[hiOffset + 1] << 8) | glyphBuff[loOffset + 1];
    u16 lineShadow = (line >> 1) | line;
    u16 shadow = ((lineShadow ^ line) | lineNextShadow);
    shadowBuff[hiOffset] = (u8)(shadow >> 8);
    shadowBuff[loOffset] = (u8)shadow;
    lineNextShadow = (lineShadow | lineNext) ^ lineNext;
    line = lineNext;
  }
  shadowBuff[15] = (u8)(lineNextShadow >> 8);
  shadowBuff[31] = (u8)lineNextShadow;

  // step 3 reorder
  for (int i = 0; i < 4; i++)
  {
    temp = *(u32 *)&glyphBuff[16 + i * 4];
    *(u32 *)&glyphBuff[16 + i * 4] = *(u32 *)&shadowBuff[i * 4];
    *(u32 *)&shadowBuff[i * 4] = temp;
  }
}

static void DrawDownArrowAlt() {
    if (sTextDrawingContext.delayCounter != 0)
    {
        --sTextDrawingContext.delayCounter;
    }
    else
    {
        FillWindowPixelRect(sTextDrawingContext.windowId, PIXEL_FILL(sTextDrawingContext.bgColor), sTextDrawingContext.left, sTextDrawingContext.top, 0x8, 0x10);
        BlitBitmapRectToWindow(sTextDrawingContext.windowId, sDownArrowTiles, 0, sDownArrowYCoords[sTextDrawingContext.downArrowYCoordsIndex & 3], 8, 16, sTextDrawingContext.left, sTextDrawingContext.top, 8, 16);
        CopyWindowToVram(sTextDrawingContext.windowId, COPYWIN_GFX);
        sTextDrawingContext.delayCounter = 8;
        sTextDrawingContext.downArrowYCoordsIndex++;
    }
}

static bool8 HandleDrawingText() {
    u8 currChar, loChar, width, glyphBuff[64];
    const u8 *glyphPtr;
    u16 glyphIdx;

    switch (sTextDrawingContext.state) {
        case RENDER_STATE_HANDLE_CHAR:
            currChar = *sTextDrawingContext.currChar;
            if (currChar == EOS)
                return TRUE;
            sTextDrawingContext.currChar++;
            switch (currChar)
            {
            case CHAR_NEWLINE:
                sTextDrawingContext.left = sTextDrawingContext.origLeft;
                sTextDrawingContext.top += GLYPH_HEIGHT;
                break;
            case CHAR_PROMPT_SCROLL:
                if (sTextDrawingContext.textSpeed > 0) {
                    sTextDrawingContext.delayCounter = 0;
                    sTextDrawingContext.downArrowYCoordsIndex = 0;
                    sTextDrawingContext.state = RENDER_STATE_SCROLL_START;
                }
                else {
                    sTextDrawingContext.delayCounter = GLYPH_HEIGHT;
                    sTextDrawingContext.state = RENDER_STATE_SCROLL;
                }
                break;
            case CHAR_PROMPT_CLEAR:
                sTextDrawingContext.delayCounter = 0;
                sTextDrawingContext.downArrowYCoordsIndex = 0;
                sTextDrawingContext.state = RENDER_STATE_CLEAR;
                break;
            case EXT_CTRL_CODE_BEGIN:
                currChar = *sTextDrawingContext.currChar++;
                switch (currChar)
                {
                case EXT_CTRL_CODE_COLOR:
                    sTextDrawingContext.fgColor = *sTextDrawingContext.currChar++;
                    break;
                case EXT_CTRL_CODE_JPN:
                    sTextDrawingContext.isJpnChar = TRUE;
                    sTextDrawingContext.currNarrowFont = gJPNFont1bpp;
                    break;
                case EXT_CTRL_CODE_ENG:
                    sTextDrawingContext.isJpnChar = FALSE;
                    sTextDrawingContext.currNarrowFont = gLatinFont1bpp;
                    break;
            }
            break;
            case 0x01 ... 0x05:
            case 0x07 ... 0x1A:
            case 0x1C ... 0x1E:
                if (!sTextDrawingContext.isJpnChar)
                {
                    loChar = *sTextDrawingContext.currChar;
                    if (loChar >= 0x00 && loChar <= 0xF6)
                    {
                        sTextDrawingContext.currChar++;
                        if (currChar >= 0x07)
                            currChar--;
                        if (currChar >= 0x1B)
                            currChar--;
                        currChar--;
                        glyphIdx = currChar * 0xF7 + loChar;
                        if (glyphIdx < sizeof(gCHSFont1bpp) / 16)
                            glyphPtr = &gCHSFont1bpp[glyphIdx *16];
                        else
                            glyphPtr = &gCHSFont1bppExt[(glyphIdx - sizeof(gCHSFont1bpp) / 16) * 16];
                        width = glyphPtr[15] & 0xF;
                        ConvertWideGlyph(glyphPtr, glyphBuff, 11, 11, 2);
                        if (width > 8)
                        {
                            DrawGlyph(sTextDrawingContext.windowId, glyphBuff, sTextDrawingContext.left, sTextDrawingContext.top, 8, &sTextDrawingContext.bgColor);
                            DrawGlyph(sTextDrawingContext.windowId, &glyphBuff[32], sTextDrawingContext.left + 8, sTextDrawingContext.top, width - 8, &sTextDrawingContext.bgColor);
                        }
                        else
                        {
                            DrawGlyph(sTextDrawingContext.windowId, glyphBuff, sTextDrawingContext.left, sTextDrawingContext.top, width, &sTextDrawingContext.bgColor);
                        }
                        sTextDrawingContext.left += width;
                        break;
                    }
                }
            default:
                glyphPtr = sTextDrawingContext.currNarrowFont + currChar * 16;
                ConvertNarrowGlyph(glyphPtr, glyphBuff);
                if (sTextDrawingContext.isJpnChar) {
                    width = 8;
                }
                else {
                    width = gLatinFont1bppWidthTable[currChar];
                }
                DrawGlyph(sTextDrawingContext.windowId, glyphBuff, sTextDrawingContext.left, sTextDrawingContext.top, width, &sTextDrawingContext.bgColor);
                sTextDrawingContext.left += width;
                break;
            }
            break;
        case RENDER_STATE_SCROLL_START:
            DrawDownArrowAlt();
            if (JOY_NEW(A_BUTTON | B_BUTTON))
            {
                FillWindowPixelRect(sTextDrawingContext.windowId, PIXEL_FILL(sTextDrawingContext.bgColor), sTextDrawingContext.left, sTextDrawingContext.top, 0x8, 0x10);
                sTextDrawingContext.delayCounter = GLYPH_HEIGHT;
                sTextDrawingContext.state = RENDER_STATE_SCROLL;
            }
            break;
        case RENDER_STATE_SCROLL:
            if (sTextDrawingContext.delayCounter != 0) {
                ScrollWindow(sTextDrawingContext.windowId, 0, sScrollSpeeds[sTextDrawingContext.textSpeed], PIXEL_FILL(sTextDrawingContext.bgColor));
                sTextDrawingContext.delayCounter -= sScrollSpeeds[sTextDrawingContext.textSpeed];
                sTextDrawingContext.left = sTextDrawingContext.origLeft;
            }
            else {
                sTextDrawingContext.state = RENDER_STATE_HANDLE_CHAR;
            }
            break;
        case RENDER_STATE_CLEAR:
            DrawDownArrowAlt();
            if (JOY_NEW(A_BUTTON | B_BUTTON))
            {
                FillWindowPixelBuffer(sTextDrawingContext.windowId, PIXEL_FILL(sTextDrawingContext.bgColor));
                sTextDrawingContext.left = sTextDrawingContext.origLeft;
                sTextDrawingContext.top = sTextDrawingContext.origTop;
                sTextDrawingContext.state = RENDER_STATE_HANDLE_CHAR;
                sTextDrawingContext.delayCounter = 0;
            }
            break;
    }
    return FALSE;
}

void InitBppConvTable() {
    u32 value;
    for (u32 i = 0; i < 256; i++) {
        value = 0;
        for (u32 j = 0; j < 8; j++) {
            if ((i & (0x80 >> j)) != 0) {
                value = value | (1 << (4 * j));
            }
        }
        gBppConvTable[i] = value;
    }
}

void DrawText(u8 windowId, const u8 * str, u8 left, u8 top, const u8 * colors, bool8 drawImmediately) {
    memset(&sTextDrawingContext, 0, sizeof(sTextDrawingContext));
    sTextDrawingContext.windowId = windowId;
    sTextDrawingContext.currNarrowFont = gLatinFont1bpp;
    sTextDrawingContext.currChar = str;
    sTextDrawingContext.left = sTextDrawingContext.origLeft = left;
    sTextDrawingContext.top = sTextDrawingContext.origTop = top;
    if (colors) {
        sTextDrawingContext.bgColor = colors[0];
        sTextDrawingContext.fgColor = colors[1];
        sTextDrawingContext.shadowColor = colors[2];
    }
    else {
        sTextDrawingContext.bgColor = TEXT_COLOR_WHITE;
        sTextDrawingContext.fgColor = TEXT_COLOR_DARK_GRAY;
        sTextDrawingContext.shadowColor = TEXT_COLOR_LIGHT_GRAY;
    }
    sTextDrawingContext.state = RENDER_STATE_HANDLE_CHAR;
    while (!HandleDrawingText())
        ;
    if (drawImmediately) {
        CopyWindowToVram(sTextDrawingContext.windowId, COPYWIN_GFX);
    }
}

void DrawMessage(const u8 * message, u8 speed) {
    memset(&sTextDrawingContext, 0, sizeof(sTextDrawingContext));
    sTextDrawingContext.currNarrowFont = gLatinFont1bpp;
    sTextDrawingContext.currChar = message;
    sTextDrawingContext.bgColor = TEXT_COLOR_WHITE;
    sTextDrawingContext.fgColor = TEXT_COLOR_DARK_GRAY;
    sTextDrawingContext.shadowColor = TEXT_COLOR_LIGHT_GRAY;
    sTextDrawingContext.state = RENDER_STATE_HANDLE_CHAR;
    sTextDrawingContext.textSpeed = speed;
}

bool8 HandleMessage() {
    if (sTextDrawingContext.state == RENDER_STATE_HANDLE_CHAR) {
        if (*sTextDrawingContext.currChar == EOS)
            return TRUE;
        if (sTextDrawingContext.delayCounter != 0) {
            sTextDrawingContext.delayCounter--;
            return FALSE;
        }
    }
    HandleDrawingText();
    if (sTextDrawingContext.state == RENDER_STATE_HANDLE_CHAR) {
        CopyWindowToVram(sTextDrawingContext.windowId, COPYWIN_GFX);
        if (JOY_HELD(A_BUTTON | B_BUTTON)) 
            sTextDrawingContext.delayCounter = sTextDelays[0];
        else
            sTextDrawingContext.delayCounter = sTextDelays[sTextDrawingContext.textSpeed];
    }
    return FALSE;
}

s32 GetStringWidth(u8 fontId, const u8 *str, s16 letterSpacing) {
    bool8 isJapanese;
    u32 lineWidth;
    const u8 *glyphPtr;
    int glyphWidth;
    s32 width;
    u32 glyphIdx;
    u8 hiChar;

    isJapanese = 0;

    width = 0;
    lineWidth = 0;

    while (*str != EOS)
    {
        switch (*str)
        {
        case CHAR_NEWLINE:
            if (lineWidth > width)
                width = lineWidth;
            lineWidth = 0;
            break;
        case EXT_CTRL_CODE_BEGIN:
            switch (*++str)
            {
            case EXT_CTRL_CODE_COLOR:
                ++str;
                break;
            case EXT_CTRL_CODE_JPN:
                isJapanese = 1;
                break;
            case EXT_CTRL_CODE_ENG:
                isJapanese = 0;
                break;
            }
            break;
        case CHAR_PROMPT_SCROLL:
        case CHAR_PROMPT_CLEAR:
            break;
        case 0x01 ... 0x05:
        case 0x07 ... 0x1A:
        case 0x1C ... 0x1E:
            if (!isJapanese) {
                if (str[1] >= 0x00 && str[1] <= 0xF6) {
                    hiChar = str[0];
                    if (hiChar >= 0x07)
                        hiChar--;
                    if (hiChar >= 0x1B)
                        hiChar--;
                    hiChar--;
                    glyphIdx = hiChar * 0xF7 + str[1];
                    if (glyphIdx < sizeof(gCHSFont1bpp) / 16)
                        glyphPtr = &gCHSFont1bpp[glyphIdx * 16];
                    else
                        glyphPtr = &gCHSFont1bppExt[(glyphIdx - sizeof(gCHSFont1bpp) / 16) * 16];
                    glyphWidth = glyphPtr[15] & 0xF;
                    lineWidth += glyphWidth;
                    str++;
                    break;
                }
            }
        default:
            if (isJapanese)
                lineWidth += 8;
            else 
                lineWidth += gLatinFont1bppWidthTable[str[0]];
            break;
        }
        ++str;
    }

    if (lineWidth > width)
        return lineWidth;
    return width;
}