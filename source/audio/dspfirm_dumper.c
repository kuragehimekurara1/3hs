/* This file is part of 3hs
 * Copyright (C) 2021-2022 hShop developer team
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE /* for memmem */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>

#define FIRM_PATH "/3ds/dspfirm.cdc"


static int lzss_decompress_buffer(u8* compressed, u32 compressedsize, u8* decompressed, u32 decompressedsize);
static u8 *lzss_decompress(u8 *orig, size_t siz, size_t *nsiz)
{
	size_t dsize = siz + * (u32 *) (&orig[siz - 4]);

	u8 *ret = malloc(dsize);
	if(!ret) return NULL;

	if(lzss_decompress_buffer(orig, siz, ret, dsize) != 0)
	{
		free(ret);
		return NULL;
	}

	*nsiz = dsize;
	return ret;
}

static int try_dump_from_title_id(u64 tid, FS_MediaType media)
{
	u32 archivepath[] = { tid & 0xFFFFFFFF, (tid >> 32) & 0xFFFFFFFF, media, 0 /* ExeFS */ };
	u32 filepath[] = { 0 /* NCCH data */, 0 /* TMD content index */, 2 /* ExeFS */, 0x646F632E /* swap(".cod") */, 0x00000065 /* swap("e\0\0\0") */ };
	FS_Path archive = { PATH_BINARY, sizeof(archivepath), archivepath }, file = { PATH_BINARY, sizeof(filepath), filepath };

	Handle dotcode;
	if(R_FAILED(FSUSER_OpenFileDirectly(&dotcode, ARCHIVE_SAVEDATA_AND_CONTENT, archive, file, FS_OPEN_READ, 0)))
		return 0;
	int res = 0;

	u64 fsize;
	u8 *dataptr = NULL, *dsp1, *decompressed;
	u32 readb, dsp1size;
	size_t size;
	FILE *f = NULL;
	if(R_FAILED(FSFILE_GetSize(dotcode, &fsize)))
		goto fail;
	if(!(dataptr = malloc(fsize)))
		goto fail;
	if(R_FAILED(FSFILE_Read(dotcode, &readb, 0, dataptr, fsize)) || readb != fsize)
		goto fail;

	decompressed = lzss_decompress(dataptr, fsize, &size);
	if(!decompressed) goto fail;
	free(dataptr); dataptr = decompressed;

	dsp1 = dataptr;
	do {
		dsp1 = memmem(dsp1, size - ((dsp1 + 1) - dataptr), "DSP1", 4);
		if(!dsp1) goto fail;
		dsp1size = * (u32 *) (dsp1 + 4);
		/* small checks to test if this is really the firmware */
		if(* (u64 *) (&dsp1[0x18]) == 0 && (dsp1[0xE] <= 10 && dsp1[0xE] >= 1) && dsp1[0xD] <= 2 && dsp1size <= size - (dsp1 - dataptr) + 0x100)
			break;
		++dsp1;
	} while(1);

	/* signature is the first 0x100 bytes */
	dsp1 -= 0x100;
	f = fopen(FIRM_PATH, "w");
	if(!f) goto fail;
	if(fwrite(dsp1, dsp1size, 1, f) != 1)
		goto fail;

	res = 1;
fail:
	FSFILE_Close(dotcode);
	free(dataptr);
	fclose(f);
	return res;
}

void ensure_dspfirm_is_available()
{
	if(access(FIRM_PATH, F_OK) == 0)
		return;

	static const struct scantitle {
		u64 tid;
		FS_MediaType media;
	} titles[] =
	{
		{ 0x0004003000008F02, MEDIATYPE_NAND }, /* HOME menu USA */
		{ 0x0004003000008202, MEDIATYPE_NAND }, /* HOME menu JPN */
		{ 0x0004003000009802, MEDIATYPE_NAND }, /* HOME menu EUR/AUS */
		{ 0x000400300000A102, MEDIATYPE_NAND }, /* HOME menu CHN */
		{ 0x000400300000A902, MEDIATYPE_NAND }, /* HOME menu KOR */
		{ 0x000400300000B102, MEDIATYPE_NAND }, /* HOME menu TWN */
		/* note that the eShop also has 2 (!) firms but HMM is more likely to exist */
	};

	static const int titlesSize = sizeof(titles) / sizeof(struct scantitle);
	for(int i = 0; i < titlesSize; ++i)
		if(try_dump_from_title_id(titles[i].tid, titles[i].media))
			break;
}


/* from ctrtool:
MIT License

Copyright (c) 2012 neimod
Copyright (c) 2014 3DSGuy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
static int lzss_decompress_buffer(u8* compressed, u32 compressedsize, u8* decompressed, u32 decompressedsize)
{
	u8* footer = compressed + compressedsize - 8;
	u32 buffertopandbottom = * (uint32_t *) (footer+0);
	//u32 originalbottom = getle32(footer+4);
	u32 i, j;
	u32 out = decompressedsize;
	u32 index = compressedsize - ((buffertopandbottom>>24)&0xFF);
	u32 segmentoffset;
	u32 segmentsize;
	u8 control;
	u32 stopindex = compressedsize - (buffertopandbottom&0xFFFFFF);

	memset(decompressed, 0, decompressedsize);
	memcpy(decompressed, compressed, compressedsize);

	while(index > stopindex)
	{
		control = compressed[--index];

		for(i=0; i<8; i++)
		{
			if (index <= stopindex)
				break;

			if (index <= 0)
				break;

			if (out <= 0)
				break;

			if (control & 0x80)
			{
				if (index < 2)
					goto clean;

				index -= 2;

				segmentoffset = compressed[index] | (compressed[index+1]<<8);
				segmentsize = ((segmentoffset >> 12)&15)+3;
				segmentoffset &= 0x0FFF;
				segmentoffset += 2;

				if (out < segmentsize)
					goto clean;

				for(j=0; j<segmentsize; j++)
				{
					u8 data;

					if (out+segmentoffset >= decompressedsize)
						goto clean;

					data  = decompressed[out+segmentoffset];
					decompressed[--out] = data;
				}
			}
			else
			{
				if (out < 1)
					goto clean;

				decompressed[--out] = compressed[--index];
			}

			control <<= 1;
		}
	}

	return 0;

clean:
	return -1;
}

