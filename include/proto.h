#ifndef PROTO_H_
#define PROTO_H_

#define DEFAULT_MGROUP   "224.2.2.2" //多播组的地址，可以更改
#define DEFAULT_RCVPORT  "1989"

#define CHANNUM           100  // 最大频道数量
#define LISTENCHANID      0    // 该0号频道用于发送节目单
#define MINCHANID         1
#define MAXCHANID         (MINCHANID + CHANNUM - 1)

#define MSG_CHAN_MAX      (65536 - 20 - 8)//频道发送包最大尺寸
#define MAX_DATA          (MSG_CHAN_MAX - sizeof(chanid_t)) //除去标识信息的数据量最大值

#define MSG_LIST_MAX      (65536 - 20 - 8)
#define MAX_LISTITEM      (MSG_LIST_MAX - sizeof(chanid_t))
#include <stdint.h>
#include"site_type.h"
struct msg_chan_struct{//频道信息包，客户端和服务器端都按照这种格式，发包
	chanid_t chanid;   //频道号信息
	uint8_t data[1];   //数据
}__attribute__((packed));

struct msg_listitem_struct{//每个节目的信息，包括ID、长度和节目的描述信息
	chanid_t chanid;
	uint16_t len;
	uint8_t description[1];
}__attribute__((packed));

struct msg_list_struct{//节目列表，里面有各个节目的信息，chanid为0
	chanid_t chanid;
	struct msg_listitem_struct category[1];
}__attribute__((packed));




















#endif
