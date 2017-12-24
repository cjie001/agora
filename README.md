# agora
Agora is a log collection system based on Libevent. Now it only support TCP connection, and it will support other protocols later.

## prerequisites:
* libevent 2x
* libmemcached 

## architeture
mainly consist of three components:

* main worker and receiver worker
* memory queue
* queue worker

to add