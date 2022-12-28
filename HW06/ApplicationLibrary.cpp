#include "ApplicationLibrary.h"

#pragma region MESSAGE object

MESSAGE CreateMessage(int code, const stream byte_stream, uint length, uint* omessage_len)
{
	if (code > MC_ERROR || code < MC_ENCRYPT || omessage_len == NULL)
		return NULL;

	code += '0'; // to digit.
	MESSAGE m;
	if (byte_stream == NULL || length == 0) {
		length = 0;
		m = CreateStream(MESSAGE_HEADER_SIZE);
	}
	else {
		m = Clone(byte_stream, length, MESSAGE_HEADER_SIZE);

	}
	if (m != NULL) {
		uint be_length = ToNetworkByteOrder(length);

		memcpy_s(m, MESSAGE_HEADER_CODE_SIZE, &code, MESSAGE_HEADER_CODE_SIZE);
		memcpy_s(m + MESSAGE_HEADER_CODE_SIZE, MESSAGE_HEADER_LENGTH_SIZE, &be_length, MESSAGE_HEADER_LENGTH_SIZE);
		*omessage_len = length + MESSAGE_HEADER_SIZE;
	}
	return m;
}

MESSAGE CreateMessage(int code, uint value, uint* omessage_len)
{
	if (code > MC_ERROR || code < MC_ENCRYPT || omessage_len == NULL)
		return NULL;

	code += '0'; // to digit.
	uint length = sizeof(value);
	MESSAGE m = (MESSAGE)CreateStream(MESSAGE_HEADER_SIZE + length);
	if (m != NULL) {
		uint be_value_length = ToNetworkByteOrder(length);
		uint be_value = ToNetworkByteOrder(value);

		memcpy_s(m, MESSAGE_HEADER_CODE_SIZE, &code, MESSAGE_HEADER_CODE_SIZE);
		memcpy_s(m + MESSAGE_HEADER_CODE_SIZE, MESSAGE_HEADER_LENGTH_SIZE, &be_value_length, MESSAGE_HEADER_LENGTH_SIZE);
		memcpy_s(m + MESSAGE_HEADER_SIZE, length, &be_value, length);
		*omessage_len = length + MESSAGE_HEADER_SIZE;
	}
	return m;
}

int ExtractMessage(const MESSAGE message, uint message_len, stream* opayload, uint* olength)
{
	if (opayload == NULL || olength == NULL)
		return INVALID_ARGUMENTS;
	*opayload = NULL;

	stream _payload = NULL;

	if (message_len >= MESSAGE_HEADER_SIZE) {
		int _code = *(unsigned char*)(message)-'0'; // first byte
		if (_code <= MC_ERROR && _code >= MC_ENCRYPT) {

			uint _length = ToHostByteOrder(ToUnsignedInt(message + MESSAGE_HEADER_CODE_SIZE));
			if (_code != MC_ERROR && _length <= MESSAGE_PAYLOAD_MAX_SIZE)
				_payload = Clone(message + MESSAGE_HEADER_SIZE, _length);

			if (_payload != NULL) {
				*olength = _length;
				*opayload = _payload;
				return _code;
			}
		}
	}
	return MC_INVALID;
}

void DestroyMessage(MESSAGE m)
{
	free(m);
}

#pragma endregion

#pragma region Non-Overlapped IO

int SendEncryptDecryptMessage(SOCKET sender, int request_type, int key)
{
	int status = FAIL;
	uint message_len;
	MESSAGE message = CreateMessage(request_type == RT_ENCRYPT ? MC_ENCRYPT : MC_DECRYPT, key, &message_len);
	if (message != NULL) {
		status = SendSegment(sender, 1, message, message_len);
	}
	DestroyMessage(message);
	return status;
}

int SendDataMessage(SOCKET sender, const stream content, uint content_len)
{
	int status = FAIL;
	uint message_len;
	MESSAGE message = CreateMessage(MC_DATA, content, content_len, &message_len);
	if (message != NULL) {
		status = SendSegment(sender, 1, message, message_len);
	}
	DestroyMessage(message);
	return status;
}

int ReceiveMessage(SOCKET receiver, int* ocode, stream* opayload, uint* olength)
{
	if (ocode == NULL || opayload == NULL || olength == NULL)
		return INVALID_ARGUMENTS;

	*opayload = NULL;
	MESSAGE message;
	uint message_len;
	int ret = ReceiveSegment(receiver, 1, &message, &message_len);
	if (ret == FATAL_ERROR)
		return ret;
	if (ret == SUCCESS) {
		stream content;
		uint content_len;
		*ocode = ExtractMessage(message, message_len, &content, &content_len);
		if (*ocode != MC_INVALID) {
			*opayload = content;
			*olength = content_len;
		}
		else
			ret = FAIL;
	}
	DestroyMessage(message);
	return ret;
}

int ReceiveACK(SOCKET receiver)
{
	stream read;

	// read segment content size
	int ret = Receive(receiver, 1, ACK_PACKET_SIZE, &read);
	if (ret == SUCCESS) {
		ret = (ToUnsignedInt(read) == 0 ? SUCCESS : FAIL);
	}
	DestroyStream(read);

	return ret;
}

int SendACK(SOCKET sender)
{
	char ack[ACK_PACKET_SIZE];
	uint value = 0;
	memcpy_s(ack, ACK_PACKET_SIZE, &value, ACK_PACKET_SIZE);
	return Send(sender, 1, ACK_PACKET_SIZE, ack);
}

#pragma endregion

#pragma region Overlapped IO

int SendDataMessage(SOCKETEX* sender, const stream content, uint content_len)
{
	uint message_len;
	int status = FAIL;

	MESSAGE message = CreateMessage(MC_DATA, content, content_len, &message_len);
	if (message != NULL) {
		status = SendSegment(sender, message, message_len);
	}
	DestroyMessage(message);
	return status;
}

int SendACK(SOCKETEX* sender)
{
	PrepareBuffer(sender, ACK_PACKET_SIZE);
	memset(sender->buffer.buf, 0, ACK_PACKET_SIZE);

	return Send(sender);
}

int ReceiveACK(SOCKETEX* receiver)
{
	PrepareBuffer(receiver, ACK_PACKET_SIZE);
	return Receive(receiver);
}

#pragma endregion
