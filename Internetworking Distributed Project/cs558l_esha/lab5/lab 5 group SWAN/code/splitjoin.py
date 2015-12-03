#!/usr/bin/python
""" FileSplitter - Simple Python file split/concat module.

    What it does
    -==========-
    
    1. Split a text/binary file into equal sized chunks
       and save them separately. 

    2. Concat existing chunks and recreate
       original file.

    Author: Anand Pillai
    Copyright : None, (Public Domain)
"""

import os, sys

class FileSplitterException(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return str(self.value)

def usage():
    return """\nUsage: FileSplitter.py -i <inputfile> -n <chunksize> [option]\n
    Options:\n
    -s, --split  Split file into chunks
    -j, --join   Join chunks back to file.
    """

class FileSplitter:
    """ File splitter class """

    def __init__(self):

        # cache filename
        self.__filename = ''
        # number of equal sized chunks
        self.__numchunks = 5
        # Size of each chunk
        self.__chunksize = 0
        # Optional postfix string for the chunk filename
        self.__postfix = ''
        # Program name
        self.__progname = "FileSplitter.py"
        # Action
        self.__action = 0 # split

	#fsp.myArgs(sys.argv[3],sys.argv[2])
	#echo "./splitjoin.py $file $numhosts &">>server
    def myArgs(self,count,SorJ ):

        import getopt

        #try:
         #   optlist, arglist = getopt.getopt(args, 'sji:n:', ["split=", "join="])
        #except getopt.GetoptError, e:
         #   print e
          #  return None

        
	print "This is count ",count            
	print "This is SofJ", SorJ
        self.__filename = sys.argv[1]
        self.__numchunks = int(count)
	SorJ = int(SorJ)

	if SorJ==0:
		self.__action = 0 # split
	if SorJ==1: 
		self.__action = 1 # combine
        if not self.__filename:
            sys.exit("Error: filename not given")
        
    def do_work(self):
        if self.__action==0:
            self.split()
        elif self.__action==1:
            self.combine()
        else:
            return None
        
    def split(self):
        """ Split the file and save chunks
        to separate files """

        print 'Splitting file', self.__filename
        print 'Number of chunks', self.__numchunks, '\n'
        
        try:
            f = open(self.__filename, 'rb')
        except (OSError, IOError), e:
            raise FileSplitterException, str(e)

        bname = (os.path.split(self.__filename))[1]
        # Get the file size
        fsize = os.path.getsize(self.__filename)
        # Get size of each chunk
        self.__chunksize = int(float(fsize)/float(self.__numchunks))

        chunksz = self.__chunksize
        total_bytes = 0

        for x in range(self.__numchunks):
            chunkfilename = bname + '-' + str(x+1) + self.__postfix

            # if reading the last section, calculate correct
            # chunk size.
            if x == self.__numchunks - 1:
                chunksz = fsize - total_bytes

            try:
                print 'Writing file',chunkfilename
                data = f.read(chunksz)
                total_bytes += len(data)
                chunkf = file(chunkfilename, 'wb')
                chunkf.write(data)
                chunkf.close()
            except (OSError, IOError), e:
                print e
                continue
            except EOFError, e:
                print e
                break

        print 'Done.'

    def sort_index(self, f1, f2):

        index1 = f1.rfind('-')
        index2 = f2.rfind('-')
        
        if index1 != -1 and index2 != -1:
            i1 = int(f1[index1:len(f1)])
            i2 = int(f2[index2:len(f2)])
            return i2 - i1
        
    def combine(self):
        """ Combine existing chunks to recreate the file.
        The chunks must be present in the cwd. The new file
        will be written to cwd. """

        import re
        print 'Creating file', self.__filename
        
        bname = (os.path.split(self.__filename))[1]
        bname2 = bname
	print " This is bname ", bname
        
        # bugfix: if file contains characters like +,.,[]
        # properly escape them, otherwise re will fail to match.
        for a, b in zip(['+', '.', '[', ']','$', '(', ')'],
                        ['\+','\.','\[','\]','\$', '\(', '\)']):
            bname2 = bname2.replace(a, b)
            
        chunkre = re.compile(bname2 + '-' + '[0-9]+')
        
        chunkfiles = []
        for f in os.listdir("."):
            print f
            if chunkre.match(f):
                chunkfiles.append(f)


        print 'Number of chunks', len(chunkfiles), '\n'
        chunkfiles.sort(self.sort_index)

        data=''
        for f in chunkfiles:

            try:
                print 'Appending chunk', os.path.join(".", f)
                data += open(f, 'rb').read()
            except (OSError, IOError, EOFError), e:
                print e
                continue

        try:
	    #bname = 'files/myoutfile.out'		    
            f = open(bname, 'wb+')
            f.write(data)
            f.close()
        except (OSError, IOError, EOFError), e:
            raise FileSplitterException, str(e)

        print 'Wrote file', bname

def main():
    import sys

    if len(sys.argv)<1:
        sys.exit(usage())
    fsp = FileSplitter()
    fsp.myArgs(sys.argv[2],0)
    fsp.do_work()

if __name__=="__main__":
    main()
