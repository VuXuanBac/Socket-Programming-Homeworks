#pragma once

#include "Utilities.h"

#define SHIFT_KEY_SPACE 256

char* EncryptShiftCipher(uint key, const char* data, uint length);
char* DecryptShiftCipher(uint key, const char* data, uint length);