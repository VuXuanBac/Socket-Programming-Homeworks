#pragma once

#include "Utilities.h"

#define SHIFT_KEY_SPACE 256

/// <summary>
/// Encrypt a byte stream using Shift Cipher.
/// </summary>
/// <param name="key">The key in Shift Cipher algorithm</param>
/// <param name="data">The byte stream want to encrypt</param>
/// <param name="length">The length of the byte stream</param>
/// <returns>The encrypted stream. NULL if fail to allocate memory</returns>
stream EncryptShiftCipher(uint key, const stream data, uint length);

/// <summary>
/// Decrypt a byte stream using Shift Cipher.
/// </summary>
/// <param name="key">The key in Shift Cipher algorithm</param>
/// <param name="data">The byte stream want to decrypt</param>
/// <param name="length">The length of the byte stream</param>
/// <returns>The decrypted stream. NULL if fail to allocate memory</returns>
stream DecryptShiftCipher(uint key, const stream data, uint length);