# 概要
シンプルなブロックデバイスドライバについて

# 使い方

ビルドとモジュールのインストール
```
$ make
$ sudo insmod simpleblock.ko
```

指定したブロックデバイスが作成されていることの確認
```
$ lsmod | grep -i simple
simpleblock            12977  0 
$ ls -li /dev/sbd*
250986 brw-rw----. 1 root disk 252, 0 Apr  2 22:25 /dev/sbd0
```

ブロックデバイスのフォーマット
```
$ sudo mkfs.ext2 /dev/sbd0
mke2fs 1.42.9 (28-Dec-2013)
Filesystem label=
OS type: Linux
Block size=1024 (log=0)
Fragment size=1024 (log=0)
Stride=0 blocks, Stripe width=0 blocks
64 inodes, 512 blocks
25 blocks (4.88%) reserved for the super user
First data block=1
Maximum filesystem blocks=524288
1 block group
8192 blocks per group, 8192 fragments per group
64 inodes per group

Allocating group tables: done                            
Writing inode tables: done                            
Writing superblocks and filesystem accounting information: done
```

マウントする
```
$ mkdir /tmp/hoge
$ sudo mount /dev/sbd0 /tmp/hoge
$ mount -v | grep hoge
/dev/sbd0 on /tmp/hoge type ext2 (rw,relatime,seclabel)
```

書き込む
```
$ sudo bash -c 'echo "hogehoge" >  /tmp/hoge/zzz'
```

終わったらumountとrmmodしておく
``
$ sudo umount /tmp/hoge
$ sudo rmmod simpleblock
``

# 参考URL
- lwn.net: Enhanced simple block driver example
  - https://lwn.net/Articles/31513/
- https://changewath.wixsite.com/denverfullhd/single-post/2016/01/12/Simple-Block-Device-Driver-Example
- A Simple Block Driver for Linux Kernel 2.6.31
  - https://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/
- https://wiki.bit-hive.com/north/pg/%E3%82%B7%E3%83%B3%E3%83%97%E3%83%AB%E3%83%96%E3%83%AD%E3%83%83%E3%82%AF%E3%83%87%E3%83%90%E3%82%A4%E3%82%B9%E3%83%89%E3%83%A9%E3%82%A4%E3%83%90
