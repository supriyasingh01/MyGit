 #!/usr/bin/python
 from socket import *
 import sys, time
 from thread import start_new_thread, get_ident
  start = time.time()
 threads = {}
 sock = socket(AF_INET, SOCK_DGRAM)
 def request(n):
         sock.sendto("%s [%d]" % (sys.argv[2],n),(sys.argv[1], int(sys.argv[3])))
         messin, server = sock.recvfrom(255)
         print "Received:", messin
         del threads[get_ident()]
  for n in range(20):
         id = start_new_thread(request, (n,))
         threads[id] = None
         #print id,
 while threads: time.sleep(.1)
 sock.close()
 print "%.2f seconds" % (time.time()-start)
