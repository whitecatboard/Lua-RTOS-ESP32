import sys
import re

exp = sys.argv[1]
text = sys.argv[2]

if re.match(exp, text):
  print 1
else:
  print 0