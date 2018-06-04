import json
import sys

board = 0
prop = "description"

if (len(sys.argv) == 2):
    board = int(sys.argv[1])
    prop = "id"

if (len(sys.argv) == 3):
    board = int(sys.argv[1])
    prop = sys.argv[2]
    
with open('boards/boards.json') as json_data:
    d = json.load(json_data)
    json_data.close()
    
for i, row in enumerate(d, 1):
    if ((board == 0) or (board == i)):
        if (board == 0):
            sys.stdout.write('%2d: %s' % (i, row[prop]))
        else:
            sys.stdout.write(row[prop])
        
        if (board == 0):
             sys.stdout.write("\\")
    
sys.stdout.flush()

