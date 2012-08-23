
#ifndef _SWT_H
#define _SWT_H

#ifdef CONFIG_NBD_SWT
static int swt_send_req(struct nbd_device *nbd, struct request *req);
#else
static int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	return -ENOSYS;
}
#endif /* CONFIG_NBD_SWT */

#endif