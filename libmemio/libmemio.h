#include "bdbm_drv.h"
#include "umemory.h" /* bdbm_malloc */
#include "uthread.h" /* bdbm_thread_nanosleep */
#include "devices.h" /* bdbm_dm_get_inf */
#include "debug.h" /* bdbm_msg */
#include <queue>
enum _dma_type{
	DMA_READ_BUF, DMA_WRITE_BUF
};
typedef struct memio_q_req{
	void *req;
	int type;
	uint32_t lba;
	uint64_t len;
	uint8_t *data;
	uint8_t q_path_type;
	int async;
	int dma_tag;
	void *__hash_node;
}memio_q_req;

typedef struct memio {
	bdbm_drv_info_t bdi;
	bdbm_llm_req_t* rr;
	/*kukania*/
	uint8_t req_flag;

	int nr_punits;
	int nr_tags;
	uint64_t io_size; /* bytes */
	uint64_t trim_size;
	uint64_t trim_lbas;
	std::queue<int>* tagQ;

	bdbm_mutex_t req_mutex;
	bdbm_mutex_t tagQMutex;
	bdbm_cond_t  tagQCond;

} memio_t;

typedef struct async_bdbm_req{
	uint32_t type;
	void (*end_req)(struct async_bdbm_req *);
	void *private_data;
}async_bdbm_req;

memio_t* memio_open ();
void memio_close (memio_t* mio);

/*
   for IO requests
		[read/write]
		the "len" of read/write should be 8192 which is size of physical page size.
		the "async" should be true. the sync mode which the async is false is not available.

		[trim]
		the "len" of memio_trim should be 16384*8192 which is size of segment.
		the "lba" of memio_trim should be aligned by 16384 which is # of page per segment.
		the "lba" of memio_trim_a_block should be aligend by 256 which is # of page per block.
 */
int memio_read (memio_t* mio, uint32_t lba, uint64_t len, uint8_t* data, int async, void *req, int dmaTag);
int memio_write (memio_t* mio, uint32_t lba, uint64_t len, uint8_t* data, int async, void *req, int dmaTag);
int memio_trim (memio_t* mio, uint32_t lba, uint64_t len, void (*end_req)(uint64_t,uint8_t));
int memio_trim_a_block (memio_t* mio, uint32_t lba);

/*wait until all requests are done.*/
void memio_wait (memio_t* mio);

/*for device status*/
int memio_get_ack_done();
bool memio_is_clean(memio_t *mio);

/*for DMA allocation*/
int memio_alloc_dma (int type, char** buf);
void memio_free_dma (int type, int dmaTag);

/*for lsmtree acceleration*/
int memio_do_merge(unsigned int ht_num, unsigned lt_num, unsigned int *kt_num, unsigned int *inv_num, uint32_t dma_ppa);
int memio_do_hw_read (memio_t* mio, uint32_t lba, char *key,uint16_t key_len, uint8_t* data, int async, void *req,int dmatag);
int memio_release_merge(int, uint32_t *);
