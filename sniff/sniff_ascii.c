#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "sniff.h"

#define NB_DATA_LINE (16)
#define LINE_SIZE_MAX  ((t_u32)65536)

t_u8 buffer_dst[(LINE_SIZE_MAX * 3) + 2]; /* Temporary ASCII Hex data */

static t_u32 bin_to_ascii_hex(t_u8* in_buffer, t_u16 in_buffer_size, t_u8* out_buffer)
{
	t_u32 i;
	t_u8* pt_src;
	t_u32 src_len;
	t_u8* pt_dst;
	t_ascii_hex val_ascii;

	src_len = in_buffer_size;
	pt_src = in_buffer;
	pt_dst = out_buffer;

	for (i = 0; i<src_len; i++)
	{
		pt_dst[0] = ' ';
		val_ascii = BinHextoAscii(pt_src[i]);
		pt_dst[1] = val_ascii.msb; /* MSB */
		pt_dst[2] = val_ascii.lsb; /* LSB */
		pt_dst += 3;
	}

	return (pt_dst - out_buffer);
}

static t_u32 bin_to_ascii_txt(t_u8* in_buffer, t_u16 in_buffer_size, t_u8* out_buffer)
{
	t_u8* pt_src;
	t_u32 src_len;
	t_u8* pt_dst;
	t_u32 i;

	src_len = in_buffer_size;
	pt_src = in_buffer;
	pt_dst = out_buffer;

	for (i = 0; i<src_len; i++)
	{
		*pt_dst = ascii_table[pt_src[i]];
		pt_dst++;
	}

	return (pt_dst - out_buffer);
}

void sniff_frame_print_ascii(FILE* out_file, t_sniff_frame* in_frame)
{
	int i;
	int nb_packet;
	int last_data;
	int data_idx;
	size_t len;
/*
    fprintf(out_file, "\n%3us %3ums %3uus\n",
        in_frame->start_of_frame_time.sec, in_frame->start_of_frame_time.msec, in_frame->start_of_frame_time.timestamp);
*/
/*
	fprintf(out_file, "< 0x%02X 0x%02X > : %s\n",
        in_frame->sap.id_8b.id0, in_frame->sap.id_8b.id1,
        trace_sap_api_str[in_frame->sap.id_8b.id0]);
*/
	nb_packet = in_frame->data_size / NB_DATA_LINE;
	data_idx = 0;
	for (i = 0; i < nb_packet; i++)
	{

		/* Print ASCII HEX Data */
		len = bin_to_ascii_hex(&in_frame->data[data_idx], NB_DATA_LINE, buffer_dst);
		if (fwrite(buffer_dst, 1, len, out_file) != len)
		{
			fprintf(stderr, "fwrite error\n");
		}

		fprintf(out_file, "  ");

		/* Print ASCII Text Data */
		len = bin_to_ascii_txt(&in_frame->data[data_idx], NB_DATA_LINE, buffer_dst);
		if (fwrite(buffer_dst, 1, len, out_file) != len)
		{
			fprintf(stderr, "fwrite error\n");
		}

		fprintf(out_file, "\n");
		data_idx += NB_DATA_LINE;
	}

	last_data = in_frame->data_size - (nb_packet * NB_DATA_LINE);
	if (last_data > 0)
	{
		/* Print ASCII HEX Data */
		len = bin_to_ascii_hex(&in_frame->data[data_idx], last_data, buffer_dst);
		/* Add padding space at end */
		for (i = 0; i < ((NB_DATA_LINE - last_data) * 3); i++)
			buffer_dst[len+i] = ' ';

		len = NB_DATA_LINE * 3;
		if (fwrite(buffer_dst, 1, len, out_file) != len)
		{
			fprintf(stderr, "fwrite error\n");
		}

		fprintf(out_file, "  ");

		/* Print ASCII Text Data */
        bin_to_ascii_txt(&in_frame->data[data_idx], last_data, buffer_dst);
        len = fwrite(buffer_dst, 1, last_data, out_file);
        if (len != (size_t)last_data)
		{
			fprintf(stderr, "fwrite error\n");
		}

		fprintf(out_file, "\n");
	}else
		fprintf(out_file, "\n");
}

/*
void sniff_to_ascii(FILE* infile, char* outfile)
{
    t_sniff_frame frame;
	char file[256];
	FILE* outfp;

	strcpy(file, outfile);
	strcat(file, ".txt");
	outfp = fopen(file, "wb");
	if (outfp != NULL)
	{
		fprintf(stdout, "ASCII trace file created OK %s\n", file);
	}
	else
	{
		fprintf(stderr, "Error to create file %s\n", file);
		return;
	}

    while(t2_trc_frame_decode(infile, &frame) == T2_TRC_FRAME_INIT_DECODE)
    {
		switch (frame.sap.id)
		{
			case FID_CPU_LOAD:
				t2_trc_cpuload_decode(&frame, &cpuload);
				t2_trc_cpuload_print_ascii(outfp, &cpuload);
			break;

			case FID_SYS_INFO:
				t2_trc_sysinfo_decode(&frame, &sysinfo);
				t2_trc_sysinfo_print_ascii(outfp, &sysinfo);
			break;

			case PID_MANAGER_CHAN0:

			break;

			case PID_MANAGER_CHAN1:

			break;

			case PID_MANAGER_CHAN2:

			break;

			case PID_MANAGER_CHAN3:

			break;

			default:
				t2_trc_frame_print_ascii(outfp, &frame);
			break;
		}
	}
	fclose(outfp);
}
*/
