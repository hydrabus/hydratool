#ifndef __SNIFF_H__
#define __SNIFF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SNIFF_HEADER_SIZE (4)

typedef signed char t_i8;
typedef unsigned char t_u8;
typedef short t_i16;
typedef unsigned short t_u16;
typedef int t_i32;
typedef unsigned int t_u32;
typedef unsigned long long t_u64;

#define BIT0    (1<<0)
#define BIT1    (1<<1)
#define BIT2    (1<<2)
#define BIT3    (1<<3)
#define BIT4    (1<<4)
#define BIT5    (1<<5)
#define BIT6    (1<<6)
#define BIT7    (1<<7)

#define PROTOCOL_MODULATION_UNKNOWN (0)
#define PROTOCOL_MODULATION_TYPEA_MILLER_MODIFIED_106KBPS (1) // MILLER MODIFIED = PCD / Readed
#define PROTOCOL_MODULATION_TYPEA_MANCHESTER_106KBPS (2) // MILLER MODIFIED = PICC / Tag

#define PROTOCOL_OPTIONS_VERSION (0) /* Versions reserved on 3bits BIT0 to BIT2 */
#define PROTOCOL_OPTIONS_START_OF_FRAME_TIMESTAMP (BIT3) /* Include 32bits start of frame timestamp(1/168MHz increment) at start of each frame */
#define PROTOCOL_OPTIONS_END_OF_FRAME_TIMESTAMP (BIT4) /* Include 32bits end of frame timestamp(1/168MHz increment) at end of each frame */
#define PROTOCOL_OPTIONS_PARITY (BIT5) /* Include an additional byte with parity(0 or 1) after each data byte */
#define PROTOCOL_OPTIONS_RAW (BIT6) /* Raw Data (modulation not decoded) */
/*
 For option PROTOCOL_OPTIONS_RAW
 - Each output byte(8bits) shall represent 1 bit data TypeA or TypeB @106kbps with following Modulation & Bit Coding:
   - PCD to PICC TypeA => Modulation 100% ASK, Bit Coding Modified Miller
   - PICC to PCD TypeA => Modulation OOK, Bit Coding Manchester
   - PCD to PICC TypeB => Modulation 10% ASK, Bit Coding NRZ
   - PICC to PCD TypeB => Modulation BPSK, Bit Coding NRZ - L
 - Note when this option is set PROTOCOL_OPTIONS_PARITY is ignored as in raw mode we have everything ...
*/

typedef struct
{
    t_u32 timestamp;
} sniff_frame_timestamp;

typedef struct
{
    t_u16 data_hdr_size; /* Header Raw Data size */
    t_u8 data_hdr[SNIFF_HEADER_SIZE]; /* Header Raw Data */
    /* Header Data */
    t_u8 protocol_options; /* See define PROTOCOL_OPTIONS_XXX */
    t_u8 protocol_modulation; /* See define PROTOCOL_MODULATION_XXX */
	t_u16 data_size; /* Data data_size */
    /* Optional start_of_frame_timestamp (depending on protocol_options flags) */
    t_u32 start_of_frame_timestamp; /* Data data_size */
    /* Optional end_of_frame_timestamp (depending on protocol_options flags) */
    t_u32 end_of_frame_timestamp; /* frame time */

    /* Optional start_of_frame_timestamp (depending on protocol_options flags) */
    sniff_frame_timestamp start_of_frame_time; /* frame time */
    /* Optional end_of_frame_timestamp (depending on protocol_options flags) */
    sniff_frame_timestamp end_of_frame_time; /* frame time */

	t_u8 data[65536];
} t_sniff_frame;

typedef enum
{
    SNIFF_FRAME_INIT_DECODE,
    SNIFF_FRAME_HEADER_DECODE,
    SNIFF_FRAME_START_TIMESTAMP_DECODE,
    SNIFF_FRAME_DATA_DECODE,
    SNIFF_FRAME_END_TIMESTAMP_DECODE
} t_sniff_frame_state;

#define SNIFF_FRAME_HEADER_DECODE_NB_BYTES (4)
#define SNIFF_FRAME_DATA_SIZE_NB_BYTES (2)
#define SNIFF_FRAME_PROTOCOL_OPTIONS_NB_BYTES (1)
#define SNIFF_FRAME_PROTOCOL_MODULATION_NB_BYTES (1)

#define SNIFF_FRAME_TIMESTAMP_DECODE_NB_BYTES (4)

typedef struct
{
	char msb;
	char lsb;
} t_ascii_hex;

extern const unsigned char ascii_table[256];

t_ascii_hex BinHextoAscii(t_u8 val);

t_u16 swap_u16(t_u16 val);
t_u32 swap_u32(t_u32 val);

/* Streaming frame decode */
t_sniff_frame_state sniff_frame_header_decode(const t_u8 in_len_u8[SNIFF_FRAME_HEADER_DECODE_NB_BYTES], t_sniff_frame* in_out_frame);
t_sniff_frame_state sniff_frame_timestamp_decode(const t_u8 in_timestamp_u8[SNIFF_FRAME_TIMESTAMP_DECODE_NB_BYTES], t_sniff_frame* in_out_frame, bool start_of_frame);
t_sniff_frame_state sniff_frame_data_decode(t_u8* in_data, t_sniff_frame* out_frame);
bool is_frame_contains_end_timestamp(const t_sniff_frame* in_frame);

int is_frame_contains_binary_data(t_sniff_frame* in_frame);

#ifdef __cplusplus
}
#endif

#endif   // __SNIFF_H__
