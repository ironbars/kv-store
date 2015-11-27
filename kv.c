#include "kv.h"

const int BUF_SIZE = KEYLEN + (sizeof(uint32_t) * 2);
const char *PORT = "3245";

unsigned char* serialize_int(unsigned char *buffer, uint32_t value)
{
	buffer[0] = value >> 24;
	buffer[1] = value >> 16;
	buffer[2] = value >> 8;
	buffer[3] = value;

	return buffer + 4;
}


unsigned char* serialize_char(unsigned char *buffer, char value)
{
	buffer[0] = value;

	return buffer + 1;
}


unsigned char* serialize_string(unsigned char *buffer, char *value)
{
	char *p = value;
	unsigned char *buf = buffer;

	while(*p != '\0') {
		buf = serialize_char(buf, *p);
		p++;
	}

	*buf = '\0';

	return buf + 1;
}


unsigned char* serialize_kv_status(unsigned char *buffer, kv_status value)
{
	// Since complilers treat enums as ints, and we don't know how one side
	// will treat ints, force the status value into a standard size (32 bits in
	// this case).
	unsigned char *ptr = serialize_int(buffer, (uint32_t) value);
	return ptr;
}


unsigned char* serialize_kv_message(unsigned char *buffer, kv_args *msg)
{
	buffer = serialize_string(buffer, msg->c_key);
	buffer = serialize_int(buffer, msg->i_value);
	buffer = serialize_kv_status(buffer, msg->status);

	return buffer;
}


unsigned char* deserialize_int(unsigned char *buffer, uint32_t *value)
{
	*value = 0;
	int i;

	for(i = 0; i < 4; i++) {
		*value += buffer[i] << (24 - (8 * i));
	}

	return buffer + 4;
}


unsigned char* deserialize_char(unsigned char *buffer, char *value)
{
	*value = buffer[0];

	return buffer + 1;
}


unsigned char* deserialize_string(unsigned char *buffer, char *value)
{
	if(value == NULL) {
		char c[32];
		value = &c[0];
	}

	char *v = value;
	unsigned char *buf = buffer;

	while(*buf != '\0') {
		buf = deserialize_char(buf, v);
		v++;
	}

	// buf should now be pointing to the null terminator of the string.
	// Use it to terminate the value
	*v = *buf;

	return buf + 1;
}


unsigned char* deserialize_kv_status(unsigned char *buffer, kv_status *value)
{
	uint32_t v;
	buffer = deserialize_int(buffer, &v);
	*value = (kv_status) v;

	return buffer;
}


unsigned char* deserialize_kv_message(unsigned char *buffer, kv_args *msg)
{
	buffer = deserialize_string(buffer, msg->c_key);
	buffer = deserialize_int(buffer, &(msg->i_value));
	buffer = deserialize_kv_status(buffer, &(msg->status));

	return buffer;
}


uint32_t hashkey(char *key)
{
	uint32_t hashval = 0; //our hash
	int i = 0;

	/* Convert our string to an integer */
	while(hashval < ULONG_MAX && i < strlen(key)) {
		hashval = hashval << 8;
		hashval += key[i];
		i++;
	}

	return hashval;
}
