import json
import sys

i = 1

if (len(sys.argv) == 2):
    board = int(sys.argv[1])
else:
    board = 0
    
with open('boards/boards.json') as json_data:
    d = json.load(json_data)
    json_data.close()
    
for row in d:
    if ((board == 0) or (board == i)):
        if (board == 0):
            sys.stdout.write(`i`.rjust(2) + ": ")
            sys.stdout.write(row['description'])
        else:
            sys.stdout.write(row['id'])
        
        if (board == 0):
             sys.stdout.write("\\")
        
    i = i + 1
    
sys.stdout.flush()