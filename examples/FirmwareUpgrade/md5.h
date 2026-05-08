#ifndef _MD5_H_
#define _MD5_H_

#include <stdint.h>

#define E_OK 0
#define E_FAILURE -1

#define HASH_VALUE_NUM 16

typedef struct _MD5_CTX_T
{
	uint32_t Total[2];
	uint32_t State[4];
	uint8_t  Buffer[64];
} MD5_CTX_T;

int MD5Start(MD5_CTX_T *Ctx);
int MD5Update(MD5_CTX_T *Ctx, const uint8_t *Data, uint32_t Bytes);
int MD5Finish(MD5_CTX_T *Ctx, uint8_t *Digest);
int MD5(const uint8_t *Data, uint32_t Bytes, uint8_t *Digest);

#endif

