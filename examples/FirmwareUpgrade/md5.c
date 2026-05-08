#include <string.h>
#include "md5.h"

#define BYTE_0(x) ((uint8_t)( (x)          & 0xff ))
#define BYTE_1(x) ((uint8_t)(((x) >> 8 ) & 0xff ))
#define BYTE_2(x) ((uint8_t)(((x) >> 16) & 0xff ))
#define BYTE_3(x) ((uint8_t)(((x) >> 24) & 0xff ))

#define GET_UINT32_LE(Data, Offset)                   \
(                                                     \
		  ((uint32_t) (Data)[(Offset)    ]      )     \
		| ((uint32_t) (Data)[(Offset) + 1] <<  8)     \
		| ((uint32_t) (Data)[(Offset) + 2] << 16)     \
		| ((uint32_t) (Data)[(Offset) + 3] << 24)     \
)

#define PUT_UINT32_LE( n, data, offset )          \
{                                                 \
	(data)[(offset)    ] = BYTE_0(n);             \
	(data)[(offset) + 1] = BYTE_1(n);             \
	(data)[(offset) + 2] = BYTE_2(n);             \
	(data)[(offset) + 3] = BYTE_3(n);             \
}

static int MD5Process(MD5_CTX_T *Ctx, const uint8_t *Data)
{
	struct {
		uint32_t X[16];
		uint32_t A;
		uint32_t B;
		uint32_t C;
		uint32_t D;
	} Local;

	Local.X[ 0] = GET_UINT32_LE(Data,  0);
	Local.X[ 1] = GET_UINT32_LE(Data,  4);
	Local.X[ 2] = GET_UINT32_LE(Data,  8);
	Local.X[ 3] = GET_UINT32_LE(Data, 12);
	Local.X[ 4] = GET_UINT32_LE(Data, 16);
	Local.X[ 5] = GET_UINT32_LE(Data, 20);
	Local.X[ 6] = GET_UINT32_LE(Data, 24);
	Local.X[ 7] = GET_UINT32_LE(Data, 28);
	Local.X[ 8] = GET_UINT32_LE(Data, 32);
	Local.X[ 9] = GET_UINT32_LE(Data, 36);
	Local.X[10] = GET_UINT32_LE(Data, 40);
	Local.X[11] = GET_UINT32_LE(Data, 44);
	Local.X[12] = GET_UINT32_LE(Data, 48);
	Local.X[13] = GET_UINT32_LE(Data, 52);
	Local.X[14] = GET_UINT32_LE(Data, 56);
	Local.X[15] = GET_UINT32_LE(Data, 60);

#define S(x,n) (((x) << (n)) | (((x) & 0xFFFFFFFF) >> (32 - (n))))

#define P(a,b,c,d,k,s,t)                                                \
		do																\
		{																\
			(a) += F((b),(c),(d)) + Local.X[(k)] + (t); 				\
			(a) = S((a),(s)) + (b); 									\
		} while( 0 )


	Local.A = Ctx->State[0];
	Local.B = Ctx->State[1];
	Local.C = Ctx->State[2];
	Local.D = Ctx->State[3];

#define F(x,y,z) ((z) ^ ((x) & ((y) ^ (z))))

	P( Local.A, Local.B, Local.C, Local.D,  0,  7, 0xD76AA478 );
	P( Local.D, Local.A, Local.B, Local.C,  1, 12, 0xE8C7B756 );
	P( Local.C, Local.D, Local.A, Local.B,  2, 17, 0x242070DB );
	P( Local.B, Local.C, Local.D, Local.A,  3, 22, 0xC1BDCEEE );
	P( Local.A, Local.B, Local.C, Local.D,  4,  7, 0xF57C0FAF );
	P( Local.D, Local.A, Local.B, Local.C,  5, 12, 0x4787C62A );
	P( Local.C, Local.D, Local.A, Local.B,  6, 17, 0xA8304613 );
	P( Local.B, Local.C, Local.D, Local.A,  7, 22, 0xFD469501 );
	P( Local.A, Local.B, Local.C, Local.D,  8,  7, 0x698098D8 );
	P( Local.D, Local.A, Local.B, Local.C,  9, 12, 0x8B44F7AF );
	P( Local.C, Local.D, Local.A, Local.B, 10, 17, 0xFFFF5BB1 );
	P( Local.B, Local.C, Local.D, Local.A, 11, 22, 0x895CD7BE );
	P( Local.A, Local.B, Local.C, Local.D, 12,  7, 0x6B901122 );
	P( Local.D, Local.A, Local.B, Local.C, 13, 12, 0xFD987193 );
	P( Local.C, Local.D, Local.A, Local.B, 14, 17, 0xA679438E );
	P( Local.B, Local.C, Local.D, Local.A, 15, 22, 0x49B40821 );

#undef F

#define F(x,y,z) ((y) ^ ((z) & ((x) ^ (y))))

	P( Local.A, Local.B, Local.C, Local.D,  1,  5, 0xF61E2562 );
	P( Local.D, Local.A, Local.B, Local.C,  6,  9, 0xC040B340 );
	P( Local.C, Local.D, Local.A, Local.B, 11, 14, 0x265E5A51 );
	P( Local.B, Local.C, Local.D, Local.A,  0, 20, 0xE9B6C7AA );
	P( Local.A, Local.B, Local.C, Local.D,  5,  5, 0xD62F105D );
	P( Local.D, Local.A, Local.B, Local.C, 10,  9, 0x02441453 );
	P( Local.C, Local.D, Local.A, Local.B, 15, 14, 0xD8A1E681 );
	P( Local.B, Local.C, Local.D, Local.A,  4, 20, 0xE7D3FBC8 );
	P( Local.A, Local.B, Local.C, Local.D,  9,  5, 0x21E1CDE6 );
	P( Local.D, Local.A, Local.B, Local.C, 14,  9, 0xC33707D6 );
	P( Local.C, Local.D, Local.A, Local.B,  3, 14, 0xF4D50D87 );
	P( Local.B, Local.C, Local.D, Local.A,  8, 20, 0x455A14ED );
	P( Local.A, Local.B, Local.C, Local.D, 13,  5, 0xA9E3E905 );
	P( Local.D, Local.A, Local.B, Local.C,  2,  9, 0xFCEFA3F8 );
	P( Local.C, Local.D, Local.A, Local.B,  7, 14, 0x676F02D9 );
	P( Local.B, Local.C, Local.D, Local.A, 12, 20, 0x8D2A4C8A );

#undef F

#define F(x,y,z) ((x) ^ (y) ^ (z))

	P( Local.A, Local.B, Local.C, Local.D,  5,  4, 0xFFFA3942 );
	P( Local.D, Local.A, Local.B, Local.C,  8, 11, 0x8771F681 );
	P( Local.C, Local.D, Local.A, Local.B, 11, 16, 0x6D9D6122 );
	P( Local.B, Local.C, Local.D, Local.A, 14, 23, 0xFDE5380C );
	P( Local.A, Local.B, Local.C, Local.D,  1,  4, 0xA4BEEA44 );
	P( Local.D, Local.A, Local.B, Local.C,  4, 11, 0x4BDECFA9 );
	P( Local.C, Local.D, Local.A, Local.B,  7, 16, 0xF6BB4B60 );
	P( Local.B, Local.C, Local.D, Local.A, 10, 23, 0xBEBFBC70 );
	P( Local.A, Local.B, Local.C, Local.D, 13,  4, 0x289B7EC6 );
	P( Local.D, Local.A, Local.B, Local.C,  0, 11, 0xEAA127FA );
	P( Local.C, Local.D, Local.A, Local.B,  3, 16, 0xD4EF3085 );
	P( Local.B, Local.C, Local.D, Local.A,  6, 23, 0x04881D05 );
	P( Local.A, Local.B, Local.C, Local.D,  9,  4, 0xD9D4D039 );
	P( Local.D, Local.A, Local.B, Local.C, 12, 11, 0xE6DB99E5 );
	P( Local.C, Local.D, Local.A, Local.B, 15, 16, 0x1FA27CF8 );
	P( Local.B, Local.C, Local.D, Local.A,  2, 23, 0xC4AC5665 );

#undef F

#define F(x,y,z) ((y) ^ ((x) | ~(z)))

	P( Local.A, Local.B, Local.C, Local.D,  0,  6, 0xF4292244 );
	P( Local.D, Local.A, Local.B, Local.C,  7, 10, 0x432AFF97 );
	P( Local.C, Local.D, Local.A, Local.B, 14, 15, 0xAB9423A7 );
	P( Local.B, Local.C, Local.D, Local.A,  5, 21, 0xFC93A039 );
	P( Local.A, Local.B, Local.C, Local.D, 12,  6, 0x655B59C3 );
	P( Local.D, Local.A, Local.B, Local.C,  3, 10, 0x8F0CCC92 );
	P( Local.C, Local.D, Local.A, Local.B, 10, 15, 0xFFEFF47D );
	P( Local.B, Local.C, Local.D, Local.A,  1, 21, 0x85845DD1 );
	P( Local.A, Local.B, Local.C, Local.D,  8,  6, 0x6FA87E4F );
	P( Local.D, Local.A, Local.B, Local.C, 15, 10, 0xFE2CE6E0 );
	P( Local.C, Local.D, Local.A, Local.B,  6, 15, 0xA3014314 );
	P( Local.B, Local.C, Local.D, Local.A, 13, 21, 0x4E0811A1 );
	P( Local.A, Local.B, Local.C, Local.D,  4,  6, 0xF7537E82 );
	P( Local.D, Local.A, Local.B, Local.C, 11, 10, 0xBD3AF235 );
	P( Local.C, Local.D, Local.A, Local.B,  2, 15, 0x2AD7D2BB );
	P( Local.B, Local.C, Local.D, Local.A,  9, 21, 0xEB86D391 );

#undef F

	Ctx->State[0] += Local.A;
	Ctx->State[1] += Local.B;
	Ctx->State[2] += Local.C;
	Ctx->State[3] += Local.D;

	/* clear the tmp variable for security - may be removed for performance */
	memset(&Local, 0, sizeof(Local));

	return E_OK;
}


int MD5Start(MD5_CTX_T *Ctx)
{
	if (Ctx == NULL)
	{
		return E_FAILURE;
	}

	Ctx->Total[0] = 0;
	Ctx->Total[1] = 0;

	Ctx->State[0] = 0x67452301;
	Ctx->State[1] = 0xEFCDAB89;
	Ctx->State[2] = 0x98BADCFE;
	Ctx->State[3] = 0x10325476;

	memset(Ctx->Buffer, 0, sizeof(Ctx->Buffer));

	return E_OK;
}

int MD5Update(MD5_CTX_T *Ctx, const uint8_t *Data, uint32_t Bytes)
{
	int Retv;
	uint32_t Fill;
	uint32_t Left;

	if(Bytes == 0)
	{
		return E_FAILURE;
	}

	Left = Ctx->Total[0] & 0x3F;
	Fill = 64 - Left;

	Ctx->Total[0] += Bytes;
	Ctx->Total[0] &= 0xFFFFFFFF;

	if(Ctx->Total[0] < Bytes)
	{
		Ctx->Total[1]++;
	}

	/* Handle the non-64B-align case */
	if( Left && Bytes >= Fill )
	{
		memcpy(Ctx->Buffer+Left, Data, Fill);

		Retv = MD5Process( Ctx, Ctx->Buffer);
		if (E_OK != Retv)
		{
			return Retv;
		}

		Data  += Fill;
		Bytes -= Fill;
		Left = 0;
	}

	/* Handle the rest 64B blocks */
	while(Bytes >= 64)
	{
		Retv = MD5Process(Ctx, Data);
		if (E_OK != Retv)
		{
			return Retv;
		}

		Data  += 64;
		Bytes -= 64;
	}

	/* Handle the remaining bytes */
	if(Bytes > 0)
	{
		memcpy(Ctx->Buffer+Left, Data, Bytes);
	}

	return E_OK;
}

int MD5Finish(MD5_CTX_T *Ctx, uint8_t *Digest)
{
	int Retv;
	uint32_t Used;
	uint32_t High, Low;

	Used = Ctx->Total[0] & 0x3F;
	Ctx->Buffer[Used++] = 0x80;

	if(Used <= 56)
	{
		memset(Ctx->Buffer+Used, 0, 56-Used);
	}
	else
	{
		memset(Ctx->Buffer+Used, 0, 64-Used);

		Retv = MD5Process(Ctx, Ctx->Buffer);
		if (E_OK != Retv)
		{
			return Retv;
		}

		memset(Ctx->Buffer, 0, 56);
	}

	High = (Ctx->Total[0] >> 29) | (Ctx->Total[1] << 3);
	Low  = (Ctx->Total[0] << 3);

	/* Last 8B for message length */
	PUT_UINT32_LE(Low,  Ctx->Buffer, 56 );
	PUT_UINT32_LE(High, Ctx->Buffer, 60 );

	Retv = MD5Process(Ctx, Ctx->Buffer);
	if (E_OK != Retv)
	{
		return Retv;
	}

	/* output digest */
	PUT_UINT32_LE(Ctx->State[0], Digest,  0);
	PUT_UINT32_LE(Ctx->State[1], Digest,  4);
	PUT_UINT32_LE(Ctx->State[2], Digest,  8);
	PUT_UINT32_LE(Ctx->State[3], Digest, 12);

	return E_OK;
}

int MD5(const uint8_t *Data, uint32_t Bytes, uint8_t *Digest)
{
	int Retv;
	MD5_CTX_T Ctx;

	if ((NULL == Data) || (NULL == Digest) || (0 == Bytes))
	{
		return E_FAILURE;
	}

	Retv = MD5Start(&Ctx);

	if (E_OK == Retv)
	{
		Retv = MD5Update(&Ctx, Data, Bytes);

		if (E_OK == Retv)
		{
			Retv = MD5Finish(&Ctx, Digest);
		}
	}

	/* clear the tmp variable for security - may be removed for performance */
	memset(&Ctx, 0, sizeof(MD5_CTX_T));

	return Retv;
}

