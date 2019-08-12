import re
import os
import os.path
import sys

path = sys.argv[1] + '/'

file_r = open(os.path.abspath(path + 'version.h'), 'r')
file_w = open(os.path.abspath(path + 'version.h.new'), 'w')

pattern = re.compile(r'^\s*\#define[\s]+BUILD_NUMBER\s+(?P<number>\d+)\s*$')

for line in file_r:
  match = re.match(pattern, line)
  if match is not None:
    file_w.write("#define BUILD_NUMBER %8d\n" % (int(match.group('number')) + 1))
    print "Build number in version.h updated to %7d." % (int(match.group('number')) + 1)
  else:
    file_w.write(line)
    
file_r.close()
file_w.close()

os.remove(os.path.abspath(path + 'version.h'))
os.rename(os.path.abspath(path + 'version.h.new'), os.path.abspath(path + 'version.h'))
