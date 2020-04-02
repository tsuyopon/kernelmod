/*
 * A sample, extra-simple block driver. Updated for kernel 2.6.31.
 *
 * (C) 2003 Eklektix, Inc.
 * (C) 2010 Pat Patterson <pat at superpat dot com>
 * Redistributable under the terms of the GNU GPL.
 */
// See: https://wiki.bit-hive.com/north/pg/%E3%82%B7%E3%83%B3%E3%83%97%E3%83%AB%E3%83%96%E3%83%AD%E3%83%83%E3%82%AF%E3%83%87%E3%83%90%E3%82%A4%E3%82%B9%E3%83%89%E3%83%A9%E3%82%A4%E3%83%90

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

MODULE_LICENSE("Dual BSD/GPL");
static char *Version = "1.4";

// module_paramはカーネルにパラメータを指定できる仕組み
//   See: https://qiita.com/koara-local/items/a3d8dc438f630c295456
static int major_num = 0;
module_param(major_num, int, 0);
static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
static int nsectors = 1024; /* How big the drive is */
module_param(nsectors, int, 0);

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE 512

/*
 * Our request queue.
 */
static struct request_queue *Queue;

/*
 * The internal representation of our device.
 */
static struct sbd_device {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} Device;

/*
 * Handle an I/O request.
 * sbd_transfer()では、実デバイスとやり取りをブロック単位で行います。
 */
static void sbd_transfer(struct sbd_device *dev, sector_t sector,
		unsigned long nsect, char *buffer, int write) {
	unsigned long offset = sector * logical_block_size;
	unsigned long nbytes = nsect * logical_block_size;

	if ((offset + nbytes) > dev->size) {
		printk (KERN_NOTICE "sbd: Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}

	// dev->dataのセクタ位置をオフセットとして、req->bufferと読み書きします
	// dev->dataは本サンプルに特化したもので、実際はデバイス依存のデバイスとのやり取りとなります。
	if (write)
		memcpy(dev->data + offset, buffer, nbytes);   // 書き込み操作の場合
	else
		memcpy(buffer, dev->data + offset, nbytes);   // 読み込み操作の場合
}

// request_queueのrequstのbio単位でデバイスバッファとやり取りを行います。
static void sbd_request(struct request_queue *q) {
	struct request *req;

	// blk_fetch_requestによりrequest_queueからrequestを取得する
	req = blk_fetch_request(q);
	while (req != NULL) {
		// blk_fs_request() was removed in 2.6.36 - many thanks to
		// Christian Paro for the heads up and fix...
		//if (!blk_fs_request(req)) {

		//  req->cmd_type != REQ_TYPE_FS はrequestがファイル参照のリクエストかどうかを表す
		//  この判定が存在するのは、requestにはデバイスの制御コマンドとしてのコード、シャットダウンコマンド等の他のリクエストも有しているからです。サンプルでは未処理です。
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {    // req->cmd_type != REQ_TYPE_FS はrequestがファイル参照のリクエストかどうかを表す
			printk (KERN_NOTICE "Skip non-CMD request\n");
			// __blk_end_request_all()をコールすることで、そのrequestを削除し、かかるセクタ数を更新し、request_queueの先頭に次のrequestが接続されます。
			// 故に、関数内の変数reqは、次にrequestということで、改めてblk_fetch_request()をコールすることなく、次にbioを取得できるわけです。
			__blk_end_request_all(req, -EIO);
			continue;
		}
		// sbd_transferでは実デバイスとやり取りをブロック単位で行います。
		// 指定している引数は以下の情報を表しています
		//   blk_rq_pos(req)はrequestの先頭セクタ位置
		//   blk_rq_cur_sectors()はrequestの先頭のbioのセクタ数
		//   rq_data_dir()はread/writeフラグ
		sbd_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
				req->buffer, rq_data_dir(req));

		// やり取りが終了すると、__blk_end_request_cur()をコールします。これはrequest先頭(読み書きした)のbioのみを削除します。
		// この時requestの先頭セクタ位置/セクタ数等も更新されます。返り値がfalseの場合、requestにはこれ以上のbioはありません。
		// 従って次のrequestのbioがデバイスが読み書きとなり、blk_fetch_request()で改めてrequestを取得することになります。
		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}
	}
}

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
/*
 * デバイスコールバックの.getgeoは、ioctrlの HDIO_GETGEOでコールすることで、blkdev_ioctl()と、そして disk->fops->getgeo()とすることで、
 * シリンダ数/ヘッド数/セクタ数/先頭位置のstruct hd_geometryが取得でき、fdisk等で無地なデバイスを設定する際のデバイス情報が取得できるようになっています。
 * サンプルはfdiskでパーティションを作ることも可能です。
 */
int sbd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

	/* We have no real geometry, of course, so make something up. */
	/*
	 * このようなハード構造を持たないデバイスにも、シリンダ/ヘッド/セクタを割り当てる必要があります。
	 * 要はシリンダ×ヘッド×セクタが全セクタ数になればいいわけで、(size & ~0x3f) >> 6で、上位6ビットをシリンダ数、ヘッドを４、セクタを16とします。
	 * ４×１６=2^6ですから、size & ~0x3fが全セクタ数となるわけです。なお、この実装ではsize & 0x3fのセクタ数は無視されることになります。
 	 */
	size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations sbd_ops = {
		.owner  = THIS_MODULE,
		.getgeo = sbd_getgeo
};

static int __init sbd_init(void) {
	/*
	 * Set up our internal device.
	 */
	Device.size = nsectors * logical_block_size;
	spin_lock_init(&Device.lock);
	Device.data = vmalloc(Device.size);
	if (Device.data == NULL)
		return -ENOMEM;
	/*
	 * blk_init_queue()でrequest_queueを取得します。
	 */
	// これは肝となる関数でブロック読み書き時のデバイス実参照時に、sbd_request()がコールされるように、
	// Device.gd->queueのrequest_fnに設定し、同時にrequest_queueのmake_request_fnに、エレベータ関数のopコールバックも設定されます。
	Queue = blk_init_queue(sbd_request, &Device.lock);
	if (Queue == NULL)
		goto out;

	// blk_queue_logical_block_size()は容量を取得してセットしている
	blk_queue_logical_block_size(Queue, logical_block_size);
	/*
	 * デバイスドライバは初期化時にregister_blkdev()でデバイスをカーネルに登録します。引数にはメジャー番号とデバイス名を指定します。
	 */
	major_num = register_blkdev(major_num, "sbd");
	if (major_num < 0) {
		printk(KERN_WARNING "sbd: unable to get major number\n");
		goto out;
	}
	/*
	 * alloc_disk関数でstruct gendisk構造体を取得し、そこにブロックデバイスの情報(メジャー番号、マイナー番号、ディスク名、セクタサイズ、セクタ数等)を設定します。
	 */
	// alloc_diskの引数16は、そのディスクのパーティション(マイナー番号)を16個まで持つことができるようにgendiskを作成するということです。
	Device.gd = alloc_disk(16);
	if (!Device.gd)                          // gendisk構造体の割り当てに失敗したらエラー
		goto out_unregister;
	Device.gd->major = major_num;            // メジャー番号
	Device.gd->first_minor = 0;              // 最初のマイナー番号
	Device.gd->fops = &sbd_ops;              // ハンドラ群のポインタを格納したstruct block_device_operationsへのポインタ
	Device.gd->private_data = &Device;       // このフィールドはデバイスドライバのために予約され、ブロックサブシステムでは使われない。通常はドライバ特有のデータ構造をポイントする
	strcpy(Device.gd->disk_name, "sbd0");    // デバイス名

	// セクタ数を登録する
	set_capacity(Device.gd, nsectors);

	// queueに登録する
	//   参照ブロックはbioに設定され、submit_io()関数をコールすることで、ブロックデバイス下のDevice.gd->queue->make_request_fnのエレベータと称するコールバック関数が、
	//   デバイスのrequest_queにリストされているrequestにリストされます。
	Device.gd->queue = Queue;

	// 仮想ディスクを登録する
	add_disk(Device.gd);

	return 0;

out_unregister:
	unregister_blkdev(major_num, "sbd");
out:
	vfree(Device.data);
	return -ENOMEM;
}

static void __exit sbd_exit(void)
{
	del_gendisk(Device.gd);
	put_disk(Device.gd);
	unregister_blkdev(major_num, "sbd");
	blk_cleanup_queue(Queue);
	vfree(Device.data);
}

module_init(sbd_init);
module_exit(sbd_exit);
