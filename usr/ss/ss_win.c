/*
 * TacOS Source Code
 *    Tokuyama kousen Advanced educational Computer.
 *
 * Copyright (C) 2019 by
 *                      Dept. of Computer Science and Electronic Engineering,
 *                      Tokuyama College of Technology, JAPAN
 *
 *   上記著作権者は，Free Software Foundation によって公開されている GNU 一般公
 * 衆利用許諾契約書バージョン２に記述されている条件を満たす場合に限り，本ソース
 * コード(本ソースコードを改変したものを含む．以下同様)を使用・複製・改変・再配
 * 布することを無償で許諾する．
 *
 *   本ソースコードは＊全くの無保証＊で提供されるものである。上記著作権者および
 * 関連機関・個人は本ソースコードに関して，その適用可能性も含めて，いかなる保証
 * も行わない．また，本ソースコードの利用により直接的または間接的に生じたいかな
 * る損害に関しても，その責任を負わない．
 *
 *
 */

 /*
  * ss/ss.cmm : ファイル送信プログラム
  *
  * 2019.10.21 : 完成
  * 2019.01.28 : 新規作成
  *
  * $Id$
  *
  */

//edit for windows

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> //sirial header
#include <sys/stat.h> //file size
#include<fcntl.h> //open
#include<sys/types.h> //open
#include <string.h> //strlen
#include <unistd.h> //read

#define BLOCK_SIZE 60
#define BUFFER_SIZE 128
#define ACK '!'

char *PORT = "/dev/com3";

int filesize[2];
char buffer[BUFFER_SIZE];
int fd; //デバイス

//ファイルサイズ取得 size[0]に上位16,[1]に下位16
int fsize(char *path, int *size) {
  struct stat statmp;                                    // stat の結果を格納する
  int e = stat(path, &statmp);                   // stat システムコール
  if (e<0) return 1;                            // stat エラーなら終了
  size[0] = statmp.st_size>>16;
  size[1] = statmp.st_size;
  return 0;
}

// Ackを待つ
void waitForAck(){
  fprintf(stderr, ".");
  fflush(stderr);
  char ack;
  for (;;) {
    if(read(fd, &ack, 1) == -1) fprintf(stderr, "read err\n");
    if (ack == ACK) break;
  }
}

int main(int argc, char *argv[]){
  if (argc < 2) {                                // ファイル名がない
    //fprintf(stderr,
    //        "Usage: %s <filename> [<as filename>]\n",
    //        argv[0]);
    fprintf(stderr,
            "Usage: %s <filename> [port]\n",
            argv[0]);
    return 1;
  }

  if (argc>=3)  PORT = argv[2];

  if (fsize(argv[1], filesize)){                 // ファイル長を取得
    perror(argv[1]);
    return 1;
  }
  if (filesize[0]>0 || filesize[1]>0xFFFF) {     // 64KiBより大きなファイル
    fprintf(stderr, "%s : Too large file\n", argv[1]);
    return 1;
  }

  FILE *fp;
  fp = fopen(argv[1], "r");                 // 送信するファイル
  if (fp == NULL){
    perror(argv[1]);
    return 1;
  }

  //シリアル通信設定
  fd = open(PORT, O_RDWR | O_NOCTTY);     // デバイスをオープン,受信書込ON,制御無し
  if (fd < 0) {
      printf("%s open error\n",PORT);
      return -1;
  }
  struct termios tio;                 // シリアル通信設定
  tio.c_cflag = CS8 | CLOCAL | CREAD; //8bit, モデム制御なし,受信有効
  tio.c_oflag = 0; //raw
  tio.c_lflag = ICANON; //カノニカル無効
  tio.c_cc[VTIME] = 0;// タイマを使わない
  tio.c_cc[VMIN] = 1;// 1文字来るまでreadをブロック

  cfsetispeed( &tio, B9600 );
  cfsetospeed( &tio, B9600 );
  cfmakeraw(&tio); //raw mode
  tcsetattr(fd,TCSANOW,&tio);

  char write_data[30];


  fprintf(stderr,"sr\n");
  write(fd, "sr\n", 3);                                // TaC側のプログラム起動
  char *filename = argv[1];
  //if (argc>=3) filename = argv[2];
  fprintf(stderr,"%s\n", filename);                      // ファイル名を出力
  fprintf(stderr,"%d\n", filesize[1]);                   // ファイル長の下位32bit
  fprintf(stderr,"%d\n", BLOCK_SIZE);                    // ブロックサイズ
  
  sprintf(write_data, "%s\n", filename);      //ファイル名
  write(fd, write_data, strlen(write_data));
  sprintf(write_data, "%d\n", filesize[1]);   //ファイル長
  write(fd, write_data, strlen(write_data));
  sprintf(write_data, "%d\n", BLOCK_SIZE);    //ブロックサイズ
  write(fd, write_data, strlen(write_data));

  waitForAck();                                  // 相手の準備完了を待つ

  int length = 0;
  for (int i=filesize[1]; i>0; i=i-1){           // ファイルの長さ繰り返す
    char c = fgetc(fp);
    write(fd, &c, 1);
    length = length + 1;
    if (length == BLOCK_SIZE){
      waitForAck();
      length = 0;
    }
  }
  waitForAck();                                  // 相手が終了した
  fclose(fp); //close
  close(fd);
  fprintf(stderr, "\n");
  return 0;
}
