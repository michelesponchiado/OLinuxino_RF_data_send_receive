ASAC Zigbee network commands


PC  --<ETHERNET/SOCKET>-- ASAC GATEWAY --<SERIAL PORT>-- ZIGBEE COORDINATOR --(((RADIO)))-- ZIGBEE END DEVICE --<SERIAL PORT>-- ASAC GATEWAY --<SOCKET>-- ANDROID END DEVICE


Command types:
- network interface
  * set/get introduce myself
  * set PAN ID
  * set CHANNEL NUMBER
  * set timeout on message transmit (vedi rpcWaitMqClientMsg(1500); in dataSendRcv.c)
  * get num of active connections
  * get stats
  * reset the network connection
  * let me now who is connected
  * read ZIGBEE framework version
- outside 
  * send a message
  * send a message and wait for a reply
  * read for incoming messages
- administrator commands
  * firmware update?

Socket message structure
* a key, e.g.: "ASAC" as N chars string?
* the command structure version as 32bit integer
* the command type as 32bit unsigned integer
* the command body which depends upon the command type

Introduce myself
When a device connects, it announces its own name.