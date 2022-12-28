#include "Crypto.h"

stream EncryptShiftCipher(uint key, const stream data, uint length)
{
	stream encrypt_data = CreateStream(length);
	if (encrypt_data != NULL) {
		for (uint i = 0; i < length; ++i) {
			encrypt_data[i] = (data[i] + key) % SHIFT_KEY_SPACE;
		}
	}
	return encrypt_data;
}
stream DecryptShiftCipher(uint key, const stream data, uint length)
{
	stream decrypt_data = CreateStream(length);
	if (decrypt_data != NULL) {
		for (uint i = 0; i < length; ++i) {
			decrypt_data[i] = (data[i] - key) % SHIFT_KEY_SPACE;
		}
	}
	return decrypt_data;
}