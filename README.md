kernelmod
=========

kernel module examples

以下の順番で見ていくと良いかもしれない
- helloworld:   カーネルモジュール最初の第一歩としてのプログラム
- getparams:    カーネルパラメータを取得するプログラム
- procfs:       procファイルシステムを利用するプログラム
- chardriver:   キャラクタデバイスを作成するプログラム
- charfromproc: キャラクタデバイスを読み込みprocファイルシステムから取得するプログラム 


# ビルド環境準備作業
カーネルと関連モジュールのバージョンが一致していないとビルドできないことがあるのでインストールしておくこと
```
$ rpm -qa | grep -i kernel
kernel-headers-3.10.0-957.27.2.el7.x86_64
kernel-tools-libs-3.10.0-957.el7.x86_64
kernel-tools-3.10.0-957.el7.x86_64
kernel-devel-3.10.0-957.el7.x86_64
kernel-3.10.0-957.el7.x86_64
```

以下のコマンドでビルドに必要となるパッケージはインストールできるはず
```
sudo yum -y install kernel-devel-$(uname -r) kernel-headers-$(uname -r)
sudo yum -y install gcc
```
