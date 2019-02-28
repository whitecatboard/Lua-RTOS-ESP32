#
# Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
# Copyright (C) 2015 - 2018, Jaume Olive Petrus (jolive@whitecatboard.org)
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    # Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    # Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    # Neither the name of the <organization> nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#    # The WHITECAT logotype cannot be changed, you can remove it, but you
#       cannot change it in any way. The WHITECAT logotype is:
#
#          /\       /\
#         /  \_____/  \
#        /_____________\
#        W H I T E C A T
#
#    # Redistributions in binary form must retain all copyright notices printed
#       to any local or remote output device. This include any reference to
#       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
#       appear in the future.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import json
import sys
import os

query = 0
query_firm = ""
query_prop = ""

if len(sys.argv) == 3:
    query = 1
    query_firm = sys.argv[1]
    query_prop = sys.argv[2]
    
# Open supported_boards.json file
with open('boards/supported_boards.json') as json_data:
    boards = json.load(json_data)
    json_data.close()
    
if query:
    for i, board in enumerate(boards, 1):
        for j, firmware in enumerate(board["firmwares"], 1):
            if firmware["id"] == query_firm:
                sys.stdout.write(board[query_prop])
                exit(0)
                
# Board selection
sys.stderr.write("Please, select a board:\r\n\r\n")

boardn = 0
for i, board in enumerate(boards, 1):
    boardn = boardn + 1
    sys.stderr.write("  %2d: %s\r\n" % (i, board["description"]))

sys.stderr.write("\r\nSelected board: ")    
sboard = raw_input()

if not sboard.isdigit():
    sys.stderr.write("Invalid board selection\r\n")
    exit(1)
    
sboard = int(sboard)

if sboard < 1 or sboard > boardn:
    sys.stderr.write("Invalid board selection\r\n")
    exit(1)
    
# Get board object
board = boards[sboard - 1]
        
# Firmware selection
sys.stderr.write("\r\n\r\nPlease, select a firmware for %s:\r\n\r\n" % (board["description"]))

firmn = 0
for j, firmware in enumerate(board["firmwares"], 1):
    firmn = firmn + 1
    sys.stderr.write("  %2d: %s\r\n" % (j, firmware["description"]))
      
sys.stderr.write("\r\nSelected firmware: ")    
firm = raw_input()
  
if not firm.isdigit():
    sys.stderr.write("Invalid board firmware\r\n")
    exit(1)
    
firm = int(firm)

if firm < 1 or firm > firmn:
    sys.stderr.write("Invalid board firmware\r\n")
    exit(1)

firm = board["firmwares"][firm - 1]

sys.stdout.write(firm["id"])

exit(1)