#include "ApplicationCommon.h"

MESSAGE CreateMessage(int code, const char* byte_stream, uint length)
{
	if (code > MC_ERROR || code < MC_ENCRYPT)
		return NULL;
	MESSAGE m;
	if (byte_stream == NULL || length == 0) {
		length = 0;
		m = Clone("", 1, CODE_LENGTH + SIZE_LENGTH);
	}
	else {
		m = Clone(byte_stream, length, CODE_LENGTH + SIZE_LENGTH);
	}

	ulong be_length = ToNetworkByteOrder((ulong)length);

	memcpy_s(m, CODE_LENGTH, &code, CODE_LENGTH);
	memcpy_s(m + CODE_LENGTH, SIZE_LENGTH, &be_length, sizeof(be_length));
	return m;
}

MESSAGE CreateMessage(int code, uint value)
{
	if (code > MC_ERROR || code < MC_ENCRYPT)
		return NULL;

	int length = sizeof(value);
	MESSAGE m = (MESSAGE)malloc(CODE_LENGTH + SIZE_LENGTH + length);
	if (m != NULL) {
		ulong be_value_length = ToNetworkByteOrder((ulong)length);
		ulong be_value = ToNetworkByteOrder((ulong)value);

		memcpy_s(m, CODE_LENGTH, &code, CODE_LENGTH);
		memcpy_s(m + CODE_LENGTH, SIZE_LENGTH, &be_value_length, SIZE_LENGTH);
		memcpy_s(m + CODE_LENGTH + SIZE_LENGTH, length, &be_value, length);
	}
	return m;
}

int ExtractMessage(const MESSAGE message, char** obyte_stream, uint* olength)
{
	int _code;
	uint _length;
	char* _byte_stream = NULL;

	uint mlen = strlen(message);
	if (mlen > CODE_LENGTH + SIZE_LENGTH) {
		_code = *(unsigned char*)(message);
		if (_code <= MC_ERROR && _code >= MC_ENCRYPT) {

			_length = ToHostByteOrder(*(ulong*)(message + CODE_LENGTH));

			if (_code != MC_ERROR && _length > 0) {
				_byte_stream = Clone(message + CODE_LENGTH + SIZE_LENGTH, _length);
			}

			if (olength != NULL)
				*olength = _length;
			if (obyte_stream != NULL)
				*obyte_stream = _byte_stream;
			return _code;
		}
	}
	return MC_INVALID;
}

void DestroyMessage(MESSAGE m)
{
	free(m);
}

uint ToUnsignedInt(const char* value)
{
	return htonl(*(ulong*)(value));
}
