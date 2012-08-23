
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/nbd.h>

/* always call with the tx_lock held */
static int swt_send_req(struct nbd_device *nbd, struct request *req)
{
  return 0;
}
