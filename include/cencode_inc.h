/*
cencode.h - c header for a base64 encoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
* Modified for ESP32-Radiola 2019 SinglWolf (https://serverdoma.ru)
*/

#ifndef BASE64_CENCODE_H
#define BASE64_CENCODE_H

typedef enum
{
	step_A, step_B, step_C
} base64_encodestep_e;

typedef struct
{
	base64_encodestep_e step;
	char result;
	int stepcount;
} base64_encodestate_s;

void base64_init_encodestate(base64_encodestate_s* state_in);

char base64_encode_value(char value_in);

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate_s* state_in);

int base64_encode_blockend(char* code_out, base64_encodestate_s* state_in);

#endif /* BASE64_CENCODE_H */
