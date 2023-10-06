#ifndef CLIENT_H_
#define CLIENT_H_ 

#define DEFAULT_PLAYERCMD  "/usr/local/bin/mpg123  > /dev/null" 
struct client_conf_structure{
	char* rcvport;
	char* mgroup;
	char* player_cmd;
};

extern struct client_conf_structure client_conf;




#endif
