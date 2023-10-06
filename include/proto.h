#ifndef PROTO_H_
#define PROTO_H_

#define DEFAULT_MGROUP   "224.2.2.2"
#define DEFAULT_RCVPORT  "1989"

#define CHANNUM           100
#define LISTENCHANID      0
#define MINCHANID         1
#define MAXCHANID         (MINCHANID + CHANNUM - 1)

#define MSG_CHAN_MAX      (65536 - 20 - 8)
#define MAX_DATA          (MSG_CHAN_MAX - sizeof(chanid_t)) 

#define MSG_LIST_MAX      (65536 - 20 - 8)
#define MAX_LISTITEM      (MSG_LIST_MAX - sizeof(chanid_t))
#include <stdint.h>
#include"site_type.h"
struct msg_chan_struct{
	chanid_t chanid;
	uint8_t data[1];
}__attribute__((packed));

struct msg_listitem_struct{
	chanid_t chanid;
	uint16_t len;
	uint8_t description[1];
}__attribute__((packed));

struct msg_list_struct{
	chanid_t chanid;
	struct msg_listitem_struct category[1];
}__attribute__((packed));




















#endif
