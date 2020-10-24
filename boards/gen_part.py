#
# Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
# Copyright (C) 2015 - 2020, Jaume Olive Petrus (jolive@whitecatboard.org)
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of the <organization> nor the
#      names of its contributors may be used to endorse or promote products
#      derived from this software without specific prior written permission.
#    * The WHITECAT logotype cannot be changed, you can remove it, but you
#      cannot change it in any way. The WHITECAT logotype is:
#
#          /\       /\
#         /  \_____/  \
#        /_____________\
#        W H I T E C A T
#
#    * Redistributions in binary form must retain all copyright notices printed
#      to any local or remote output device. This include any reference to
#      Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
#      appear in the future.
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
# Lua RTOS build system
#
import sys, argparse

SPIFFS_PART = 0x40
LFS_PART = 0x41

def make_part(label, type, subtype, offset, size):
  new_offset = offset + size
  f.write("%s,%s,%s,0x%x,0x%x\n" % (label, type, subtype, offset, size))
    
  return new_offset

# Process arguments
parser = argparse.ArgumentParser()
parser.add_argument("-LUA_RTOS_PART_STORAGE_SIZE")
parser.add_argument("-LUA_RTOS_USE_FAT")
parser.add_argument("-LUA_RTOS_USE_SPIFFS")
parser.add_argument("-LUA_RTOS_USE_LFS")
parser.add_argument("-LUA_RTOS_PARTION_CSV")
parser.add_argument("-LUA_RTOS_FLASH_SIZE")
parser.add_argument("-LUA_RTOS_PARTITION_TABLE_OFFSET")
parser.add_argument("-LUA_RTOS_USE_OTA")
parser.add_argument("-LUA_RTOS_USE_FACTORY_PARTITION")
parser.add_argument("-LUA_RTOS_PHY_INIT_DATA_IN_PARTITION")
parser.add_argument("-LUA_RTOS_PART_NVS_SIZE")
args = parser.parse_args()

# Get the flash size
flash_size = int(args.LUA_RTOS_FLASH_SIZE)

# Get the partition table offset
part_table_offset = int(args.LUA_RTOS_PARTITION_TABLE_OFFSET, 16)

# Check if read-write partition is required
use_spiffs = (args.LUA_RTOS_USE_SPIFFS == 'y');
use_lfs = (args.LUA_RTOS_USE_LFS == 'y');

storage_partition = (use_spiffs or use_lfs)

# Chek if OTA is enabled in build
with_ota = (args.LUA_RTOS_USE_OTA == 'y')

# Checl if factory partition is required
with_ota_factory = with_ota and (args.LUA_RTOS_USE_FACTORY_PARTITION == 'y')

# Check if PHY calibration partition is required
with_phy_init = (args.LUA_RTOS_PHY_INIT_DATA_IN_PARTITION == 'y')

nvs_size = int(args.LUA_RTOS_PART_NVS_SIZE)

phy_init_size = 0x1000
with_nvs = 1

# Get how many app partitions are required
app_partitions = 1

if with_ota:
  app_partitions = app_partitions + 1

  if with_ota_factory:
    app_partitions = app_partitions + 1

# Get the offset for other partitions
start = part_table_offset + 0x1000
offset = start

print ""
print "Partition      Offset         Size    Size"
print "------------------------------------------"

nvs_offset = offset
if with_nvs:
  print "nvs        0x%08x\t0x%08x  % 5dK" % (offset, nvs_size, nvs_size / 1024)
  offset = offset + nvs_size

storage_offset = offset
if storage_partition:    
  print "storage    0x%08x\t0x%08x  % 5dK" % (offset, int(args.LUA_RTOS_PART_STORAGE_SIZE), int(args.LUA_RTOS_PART_STORAGE_SIZE) / 1024)
  offset = offset + int(args.LUA_RTOS_PART_STORAGE_SIZE)

phy_init_offset = offset
if with_phy_init:
  print "phy_init   0x%08x\t0x%08x" % (offset, 0x1000)
  offset = offset + 0x1000

ota_offset = offset
if with_ota:
  print "otadata    0x%08x\t0x%08x  % 5dK" % (offset, 0x2000, 0x2000 / 1024)
  offset = offset + 0x2000

max_app_size = (flash_size - offset) // app_partitions

if ((max_app_size & 0xffff) != 0):
  # Not aligned, so align
  max_app_size = (max_app_size & ~0xffff) 

# App partitions starts at app_offset
app_offset = flash_size - (max_app_size * app_partitions)

unused =  app_offset - offset
if unused:
  print "unused     0x%08x\t0x%08x  % 5dK" % (offset, unused, unused / 1024)

# Generate the partition table
f = open(args.LUA_RTOS_PARTION_CSV,"w") 

f.write("# Lua RTOS Partition Table\n")
f.write("# Name,Type,SubType,Offset,Size\n")

if with_nvs:
  make_part("nvs","data","nvs",nvs_offset,nvs_size)

if storage_partition:
  if use_spiffs:
    storage_subtype = SPIFFS_PART
  elif use_lfs:
    storage_subtype = LFS_PART
  else:
    sys.stderr.write("invalid storage partition subtype")
    exit(1)

  make_part("storage","data",str(storage_subtype),storage_offset,int(args.LUA_RTOS_PART_STORAGE_SIZE))

if with_phy_init:
  make_part("phy_init","data","phy",phy_init_offset,phy_init_size)

if (with_ota):
  offset = make_part("otadata","data","ota",ota_offset,0x2000)

offset = app_offset
if with_ota:
  if with_ota_factory:
    print "factory    0x%08x\t0x%08x  % 5dK" % (offset, max_app_size, max_app_size / 1024)
    offset = make_part("factory","0","0",offset,max_app_size)
      
  print "ota_0      0x%08x\t0x%08x  % 5dK" % (offset, max_app_size, max_app_size / 1024)
  offset = make_part("ota_0","0","ota_0",offset,max_app_size)

  print "ota_1      0x%08x\t0x%08x  % 5dK" % (offset, max_app_size, max_app_size / 1024)
  offset = make_part("ota_1","0","ota_1",offset,max_app_size)
else:
  print "factory    0x%08x\t0x%08x  % 5dK" % (offset, max_app_size, max_app_size / 1024)
  offset = make_part("factory","0","0",offset,max_app_size)
    
f.close()

print ""