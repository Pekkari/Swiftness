
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/swt.h>

extern struct nbd_device;

/* always call with the tx_lock held */
int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	return 0;
}

struct request *swt_read_stat(struct nbd_device *nbd)
{
	struct request result;
	return (struct request *)&result;
}
