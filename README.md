#A counter of page-cache faults per file per page in Linux (ReFault_Counter)
###What does mean by refauls?
For improving the performance of I/O, most OS caches the file contents which have been accessed.
Due to limited memory-size or memory-pressure, some of them might be evicted from memory.
After some time, some of these evicted pages again requested. If these pages would have been in memory
then page-fault would have not occured. These kinds of faults has been termed as 'Refaults'.

###What can be the possible applications of the refault_counter?
In virtualized environments, one can find the suitable caching-policy in their guest-OS which might can improve the performance of running application inside guest-OS/Container.
e.g.
Consider a situation in which, One Virtual-Machine(Or Container) is running database applicatoin and other is running web-server.
It can be possible that LRU gives best performance for one while for other, LRU might not be good enough.

One can compare the performance of cahcing policy in Linux(if there is a way to do so) by observing the stats of refault counter.
