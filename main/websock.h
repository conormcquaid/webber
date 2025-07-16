#ifndef __WEBSOCK_H__
#define __WEBSOCK_H__

#define CONFIG_WEBSOCKET_URI "ws://tv-remote.local"
#define NO_DATA_TIMEOUT_SECS 10

void websocket_task(void* nada);

#endif /* __WEBSOCK_H__ */