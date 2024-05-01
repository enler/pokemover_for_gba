#ifndef GUARD_TEXT_EXT_H
#define GUARD_TEXT_EXT_H

void InitBppConvTable();
void DrawText(u8 windowId, const u8 *str, u8 left, u8 top, const u8 *colors, bool8 drawImmediately);
void DrawMessage(const u8 *message, u8 speed);
bool8 HandleMessage();

#endif