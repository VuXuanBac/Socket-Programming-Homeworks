#include "Crypto.h"

char* EncryptShiftCipher(uint key, const char* data, uint length)
{
	char* encrypt_data = (char*)malloc(length);
	if (encrypt_data != NULL) {
		for (uint i = 0; i < length; ++i) {
			encrypt_data[i] = (data[i] + key) % SHIFT_KEY_SPACE;
		}
	}
	return encrypt_data;
}
char* DecryptShiftCipher(uint key, const char* data, uint length)
{
	char* decrypt_data = (char*)malloc(length);
	if (decrypt_data != NULL) {
		for (uint i = 0; i < length; ++i) {
			decrypt_data[i] = (data[i] - key) % SHIFT_KEY_SPACE;
		}
	}
	return decrypt_data;
}