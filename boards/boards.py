import json
import sys

i = 1
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
    
for row in d:
    if ((board == 0) or (board == i)):
        if (board == 0):
            sys.stdout.write(`i`.rjust(2) + ": ")
            sys.stdout.write(row[prop])
        else:
            sys.stdout.write(row[prop])
        
        if (board == 0):
             sys.stdout.write("\\")
        
    i = i + 1
    
sys.stdout.flush()