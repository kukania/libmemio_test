/*
The MIT License (MIT)

Copyright (c) 2014-2015 CSAIL, MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "libmemio.h"

typedef struct dma_info{
	uint32_t tag;
	uint32_t dma_type;
	char *data;
}dma_info;

memio_t *mio;
void end_req(async_bdbm_req *req){
	dma_info *temp_dma=(dma_info*)req->private_data;
	switch(req->type){
		case REQTYPE_IO_READ:
			/*do something after read req*/

			/*reclaim dma*/
			memio_free_dma(DMA_READ_BUF, temp_dma->tag);
			break;
		case REQTYPE_IO_WRITE:
			/*do something after write req*/

			/*reclaim dma*/
			memio_free_dma(DMA_WRITE_BUF, temp_dma->tag);
			break;
		default:
			break;
	}
	free(temp_dma);
	free(req);
}

void erase_end_req(uint64_t seg_num, uint8_t isbad){
	/*managing block mapping when "isbad" set to 1 which mean the segments has some bad blocks*/
}

int main (int argc, char** argv)
{
	if ((mio = memio_open()) == NULL) {
		printf("could not open memio\n");
		return -1;
	}

	char temp[8192]={0,};
	/*allocation write dma*/
	dma_info *dma=(dma_info*)malloc(sizeof(dma_info));
	dma->tag=memio_alloc_dma(DMA_WRITE_BUF, &dma->data);
	memcpy(dma->data, temp, 8192);

	async_bdbm_req *temp_req=(async_bdbm_req*)malloc(sizeof(async_bdbm_req));
	temp_req->type=REQTYPE_IO_WRITE;
	temp_req->private_data=(void*)dma;
	temp_req->end_req=end_req; //when the requset ends, the "end_req" is called

	memio_write(mio, 0, 8192, (uint8_t*)dma->data, true, (void*)temp_req, dma->tag);

	/*allocation read dma*/
	dma=(dma_info*)malloc(sizeof(dma_info));
	dma->tag=memio_alloc_dma(DMA_READ_BUF, &dma->data);

	temp_req=(async_bdbm_req*)malloc(sizeof(async_bdbm_req));
	temp_req->type=REQTYPE_IO_READ;
	temp_req->end_req=end_req;
	temp_req->private_data=(void*)dma;
	memio_read(mio, 0, 8192, (uint8_t*)dma->data, true, (void*)temp_req, dma->tag);

	/*trim for badblock checking*/
	memio_trim(mio, 0, 16384*8192, erase_end_req);

	/*trim for erase data*/
	memio_trim(mio, 0, 16384*8192, NULL);

	memio_close(mio);
	return 0;
}
