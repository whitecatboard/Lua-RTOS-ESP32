/*
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 * Copyright (C) 2015 - 2026, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2026, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Lua RTOS, EEPROM flash emulation driver
 */
 
 /*
  * OVERVIEW:
  *
  * This driver emulates EEPROM functionality over NOR Flash. Since Flash bits 
  * cannot be toggled from 0 to 1 without a sector erase, this driver uses a 
  * log-structured approach to minimize destructive erase cycles and extend 
  * flash longevity.
  *
  * DESIGN PRINCIPLES:
  *
  * - Dual-Bank Strategy: Storage is divided into two physical banks: 'Active' 
  * and 'Spare'. Only one bank holds the current "Source of Truth" at any time.
  *
  * - Pointer/Cursor Logic:
  * - Read Cursor: The physical address of the first 'Active' page, used as the 
  * optimized entry point for logical-to-physical searches.
  * - Write Cursor: The address of the next 'Free' slot for data allocation.
  *
  * - Read-Modify-Write (RMW): Updates are handled by reconstructing a logical 
  * page in RAM (merging old data with the new chunk) and appending the 
  * result to the current Write Cursor.
  *
  * TRANSACTIONAL SHADOW-PAGING:
  *
  * To ensure atomicity during multi-page writes or bank exhaustion, the driver 
  * employs a shadow-paging mechanism:
  * 1. Workspace Redirection: If a write requires more space than available, or 
  * crosses a bank boundary, the 'Spare' bank is prepared as a "Shadow Bank".
  * 2. Concurrent Banks: During this phase, the 'Active' bank remains the 
  * read-only source, while all new data is directed to the 'Spare' bank.
  * 3. Atomic Commit: The transaction is finalized only when the banks are 
  * swapped, promoting the shadow workspace to 'Active' status in a single 
  * metadata update.
  *
  * GARBAGE COLLECTION (GC):
  *
  * The GC is triggered automatically when the Write Cursor reaches the end of 
  * the bank or when a large write transaction is pending.
  *
  * - Selective Migration: Only valid 'Active' pages are moved to the Spare bank.
  * - Ignore Range Optimization: The GC accepts a range of logical indices to 
  * exclude from migration. This prevents writing "old" data that is about 
  * to be overwritten by the current transaction, reducing flash wear and 
  * improving performance.
  *
  * POWER-FAIL SAFETY:
  *
  * - State Transitions: Banks use intermediate states (Active, Swapping, Free). 
  * If power is lost during a GC, the 'eeprom_init' function detects the 
  * 'Swapping' state and resolves the conflict using wear-leveling counters.
  *
  * - Pre-Commencement Capacity Check: Before any multi-page write starts, the 
  * driver calculates total space requirements. If the bank cannot fit the 
  * entire transaction, a GC is triggered *before* a single byte is written. 
  * This prevents "split-bank" scenarios where data is divided between banks.
  *
  * - Write Verification: Every physical write is immediately read back and 
  * verified against the source buffer to ensure hardware integrity and 
  * detect flash exhaustion early.
  */  
#include "eeprom_flash.h"
#include "driver.h"
#include "esp_err.h"
#include "esp_partition.h"
#include "spi_flash_mmap.h"
#include "syslog.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Partition subtype used by Lua RTOS to identify EEPROM partitions
#define LUA_RTOS_EEPROM_PART 0x42

// Unique signature to validate EEPROM storage format and integrity
#define EEPROM_SIGNATURE_H 0x121fd05827bd40c8
#define EEPROM_SIGNATURE_L 0xb4d78ef421f8eadc

// Bank size
#if CONFIG_LUA_RTOS_EEPROM_SIZE_1K
#define EEPROM_PART_SIZE 8192
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_2K
#define EEPROM_PART_SIZE 8192
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_4K
#define EEPROM_PART_SIZE 8192
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_8K
#define EEPROM_PART_SIZE 8192
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_16K
#define EEPROM_PART_SIZE 8192
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_32K
#define EEPROM_PART_SIZE 16384
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_64K
#define EEPROM_PART_SIZE 24576
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_128K
#define EEPROM_PART_SIZE 40960
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_256K
#define EEPROM_PART_SIZE 81920
#elif CONFIG_LUA_RTOS_EEPROM_SIZE_512K
#define EEPROM_PART_SIZE 163840
#endif

#define EEPROM_BANK_SIZE (EEPROM_PART_SIZE >> 1)

// Base offset in partition for the first bank
#define EEPROM_BANK0_OFFSET (0)

// Base offset in partition for the second bank
#define EEPROM_BANK1_OFFSET (EEPROM_BANK0_OFFSET + EEPROM_BANK_SIZE)

// Get the page mask
#if EEPROM_PAGE_SIZE == 4
#define EEPROM_PAGE_BITS 2
#elif EEPROM_PAGE_SIZE == 8
#define EEPROM_PAGE_BITS 3
#elif EEPROM_PAGE_SIZE == 16
#define EEPROM_PAGE_BITS 4
#elif EEPROM_PAGE_SIZE == 32
#define EEPROM_PAGE_BITS 5
#elif EEPROM_PAGE_SIZE == 64
#define EEPROM_PAGE_BITS 6
#elif EEPROM_PAGE_SIZE == 128
#define EEPROM_PAGE_BITS 7
#elif EEPROM_PAGE_SIZE == 256
#define EEPROM_PAGE_BITS 8
#endif

// Special physical address to represent a not found page
#define EEPROM_PAGE_NOT_FOUND 0xffffffff
#define EEPROM_PAGE_ID_NONE   0xffff

// Register driver and messages
DRIVER_REGISTER_BEGIN(EEPROM,eeprom,0,NULL,NULL);
	DRIVER_REGISTER_ERROR(EEPROM, eeprom, InvalidPartition, "invalid partition", EEPROM_ERR_INVALID_PARTITION);
	DRIVER_REGISTER_ERROR(EEPROM, eeprom, NotEnoughtMemory, "not enough memory", EEPROM_ERR_NO_MEM);
	DRIVER_REGISTER_ERROR(EEPROM, eeprom, FlashAccessError, "flash access error", EEPROM_ERR_FLASH_ACCESS_ERROR);
	DRIVER_REGISTER_ERROR(EEPROM, eeprom, FlashIntegrityError, "flash integrity error", EEPROM_ERR_FLASH_INTEGRITY_ERROR);
	DRIVER_REGISTER_ERROR(EEPROM, eeprom, NoSpaceLeft, "no space left", EEPROM_ERR_NO_SPACE_LEFT);
DRIVER_REGISTER_END(EEPROM,eeprom,0,NULL,NULL);

typedef enum {
	EEPROMOk = 0,
	EEPROMFLASHEraseError, 	
	EEPROMFLASHWriteError, 	
	EEPROMFLASHReadError, 	
	EEPROMFLASHIntegrityError, 	
	EEPROMFLASHNoLeftSpace,
	EEPROMBankStateError,
} eeprom_error_t;

typedef enum {
    EEPROMBankFree = 0xffffffff,
    EEPROMBankActive = 0x00000000,
    EEPROMBankSwapping = 0x55555555,
} eeprom_bank_state_t;

typedef enum {
    EEPROMPageFree = 0xffff,
    EEPROMPageActive = 0x5555,
    EEPROMPageInactive = 0x0000,
} eeprom_page_state_t;

typedef uint16_t eeprom_page_idx_t;

typedef struct __attribute__((__packed__)) {
	uint32_t state;
	uint64_t signature_h;
	uint64_t signature_l;
	uint32_t erase_count;			
} eeprom_bank_header_t;

typedef struct __attribute__((__packed__)) {
	uint16_t state;
	eeprom_page_idx_t page_idx;
} eeprom_page_header_t;

typedef struct __attribute__((__packed__)) {
	eeprom_page_header_t header;
	uint8_t data[EEPROM_PAGE_SIZE];
} eeprom_page_t;

typedef struct {
	const esp_partition_t *partition; // EEPROM partition
	
	size_t bank_size;      // Bank size in bytes	
	uint32_t active_bank;  // Physical start address of the active bank
	uint32_t spare_bank;   // Physical start address of the spare bank
	uint32_t read_cursor;  // Starting point for data retrieval
	
	uint32_t write_bank;
	uint32_t write_cursor; // Destination for the next write operation
	
	uint8_t *allocated_bits;
	uint32_t allocated_bits_size;
	
	uint32_t last_lookup;
	eeprom_page_idx_t last_lookup_idx;
} eeprom_ctx_t;

static eeprom_ctx_t eeprom_ctx = {0};

/*
 * Helper functions
 */

 static driver_error_t *_get_error(eeprom_error_t error) {
  	switch (error) {
  		case EEPROMFLASHEraseError:
  			return driver_error(EEPROM_DRIVER, EEPROM_ERR_FLASH_ACCESS_ERROR, "erase");
  		
  		case EEPROMFLASHIntegrityError:
  			return driver_error(EEPROM_DRIVER, EEPROM_ERR_FLASH_INTEGRITY_ERROR, NULL);
  			
  		case EEPROMFLASHReadError:
  			return driver_error(EEPROM_DRIVER, EEPROM_ERR_FLASH_ACCESS_ERROR, "read");

  		case EEPROMFLASHWriteError:
  			return driver_error(EEPROM_DRIVER, EEPROM_ERR_FLASH_ACCESS_ERROR, "write");
 			
 		case EEPROMFLASHNoLeftSpace:
 			return driver_error(EEPROM_DRIVER, EEPROM_ERR_NO_SPACE_LEFT, NULL);
  		
  		default: break;
  	}
  	
  	return NULL;								
  }

static inline void _set_page_allocated(eeprom_page_idx_t page_id) {
	eeprom_ctx.allocated_bits[page_id >> 3] |=  (1 << (page_id & 7));
 }
 
static inline bool _is_page_allocated(uint32_t page_id) {
	return eeprom_ctx.allocated_bits[page_id >> 3] &   (1 << (page_id & 7));
}

/**
 * @brief Establishes entry points for the Read and Write engines during boot.
 *
 * Identifies the first 'Active' page (scan start) and first 'Free' page 
 * (allocation start) during the initial bank scan.
 *
 * @param[in] page_h    Pointer to the header of the page being scanned.
 * @param[in] phys_addr Physical address of said page.
 */
 static void _update_cursors(const eeprom_page_header_t *page_h, uint32_t phys_addr) {
	 // If an 'Active' page is found and the read cursor is not yet established, 
	 // set this address as the starting point for logical searches.
	 if ((page_h->state == EEPROMPageActive) && (eeprom_ctx.read_cursor == EEPROM_PAGE_NOT_FOUND)) {
         eeprom_ctx.read_cursor = phys_addr;
     }   

	 // If a 'Free' page is found and the write cursor is not yet established, 
	 // set this address as the next available slot for data allocation.
	 if ((page_h->state == EEPROMPageFree) && (eeprom_ctx.write_cursor == EEPROM_PAGE_NOT_FOUND)) {
         eeprom_ctx.write_cursor = phys_addr;
     }   
 }
 
/**
 * @brief Reads a data block from the physical flash partition.
 *
 * @param[in]  addr Physical offset within the EEPROM partition.
 * @param[out] dst  Pointer to the destination buffer in RAM.
 * @param[in]  size Number of bytes to retrieve.
 *
 * @return EEPROMOk on success, or EEPROMFLASHReadError if the hardware 
 * interface fails.
 */
static eeprom_error_t _flash_read(uint32_t addr, void *dst, size_t size) {
	esp_err_t ret = esp_partition_read(eeprom_ctx.partition, addr, dst, size);
	if (ret != ESP_OK) {
		return EEPROMFLASHReadError;
	}
		
	return EEPROMOk;	
}

/**
 * @brief Performs a verified write operation to the physical flash partition.
 *
 * This function wraps the hardware-specific write command with an immediate 
 * Read-Back Verification pass. If the data read from flash does not 
 * perfectly match the source buffer, an integrity error is returned.
 *
 * @param[in] addr The relative offset within the flash partition.
 * @param[in] src  Pointer to the data buffer to be persisted.
 * @param[in] size The number of bytes to write and verify.
 *
 * @return EEPROMOk on success, or a specific flash error code on failure.
 */
static eeprom_error_t _flash_write(uint32_t addr, const void *src, size_t size) {
    // Perform the physical write to the partition
    esp_err_t ret = esp_partition_write(eeprom_ctx.partition, addr, src, size);
    if (ret != ESP_OK) {
        return EEPROMFLASHWriteError;
    }

    // verify the write integrity by reading the data back from flash.
	size_t chunk_size;
    uint8_t mirror[64];
    size_t verified = 0;

    while (verified < size) {
        chunk_size = (size - verified > sizeof(mirror)) ? sizeof(mirror) : (size - verified);
        
        if (esp_partition_read(eeprom_ctx.partition, addr + verified, mirror, chunk_size) != ESP_OK) {
            return EEPROMFLASHReadError;
        }

        // Bitwise comparison between source and flash-mirror
        for (size_t i = 0; i < chunk_size; i++) {
            if (((const uint8_t *)src)[verified + i] != mirror[i]) {
                return EEPROMFLASHIntegrityError;
            }
        }
        verified += chunk_size;
    }

    return EEPROMOk;
}

static eeprom_error_t _flash_erase(uint32_t addr, size_t size) {	
	esp_err_t ret = esp_partition_erase_range(eeprom_ctx.partition, addr, size);
	#if 0
	if (ret == ESP_OK) {
		uint8_t mirror[128];
		size_t chunk_size;
		
		while (size > 0) {
			chunk_size = ((size <= sizeof(mirror))?size:sizeof(mirror));
			
			ret = esp_partition_read(eeprom_ctx.partition, addr, mirror, chunk_size);
			if (ret == ESP_OK) {
				for(int offset = 0;offset < size;offset++) {
					if (mirror[offset] ^ 0xff) {
						return EEPROMFLASHIntegrityError;
					}					
				}
			} else {
				return EEPROMFLASHReadError;
			}
			
			size -= chunk_size;
			addr += chunk_size;
		}		
	} else {
		return EEPROMFLASHEraseError;
	}
	#endif
	
	return EEPROMOk;
}

/**
 * @brief Performs delta validation to prevent unnecessary flash writes.
 *
 * Compares the existing "base data" (already in flash) against the 
 * "delta data" (the new subset to be written). If the delta is already 
 * reflected in the base, the update is considered redundant.
 *
 * @param[in] base_data   Pointer to the current full page data.
 * @param[in] offset      The starting position of the change within the page.
 * @param[in] delta       Pointer to the new data subset.
 * @param[in] delta_size  The length of the incoming data subset.
 *
 * @return true if the delta is already identical to the base data at that offset.
 */
static bool _is_delta_redundant(const uint8_t *base_data, uint16_t offset, const uint8_t *delta, size_t delta_size) {
    const uint8_t *base_segment = &base_data[offset];
    
    for (size_t i = 0; i < delta_size; i++) {
        if (base_segment[i] != delta[i]) {
            // Data has changed, a physical write is required
            return false;
        }
    }
    
    // Delta matches the base, no physical operation needed
    return true;
}

/**
 * @brief Formats a physical flash bank and initializes its metadata header.
 *
 * This function performs a hardware erase of the specified bank and writes a 
 * new management header. The header includes the system signatures for 
 * integrity validation, an updated erase cycle count for wear-leveling 
 * tracking, and the target bank state.
 *
 * @param[in] bank_phys_addr The physical start address of the bank to format.
 * @param[in] cnt            The erase cycle count.
 * @param[in] state          The initial lifecycle state (typically EEPROMBankFree).
 *
 * @return EEPROMOk on success, or a flash-specific error code on failure.
 */
static eeprom_error_t _bank_format(uint32_t bank_phys_addr, uint32_t cnt, eeprom_bank_state_t state) {	
	eeprom_error_t ret;
	
	// Erase bank	
	if ((ret = _flash_erase(bank_phys_addr, EEPROM_BANK_SIZE)) != EEPROMOk) {
		return ret;
	}
	
	// Initialize the metadata header for the new bank lifecycle
	eeprom_bank_header_t header;

	// Write copy info, and set state 
	header.state       = state;
	header.signature_h = EEPROM_SIGNATURE_H;
	header.signature_l = EEPROM_SIGNATURE_L;
	
	// Maintain wear-leveling integrity by tracking cumulative erase cycles
	header.erase_count = cnt;
	
	// Persist management metadata to the physical start of the bank
	if ((ret = _flash_write(bank_phys_addr, &header, sizeof(eeprom_bank_header_t))) != EEPROMOk) {
		return ret;
	}
	
	return EEPROMOk;
}

/**
 * @brief Transitions the state of a physical flash bank.
 *
 * Persists a new bank state to the flash header. This operation is 
 * fundamental to the driver's power-fail safety, as it allows the 
 * initialization logic to detect and recover from interrupted bank swaps.
 *
 * @note This function assumes the flash hardware/driver supports partial 
 * page programming or that the state transition only involves 
 * clearing bits (1 to 0).
 *
 * @param[in] bank_phys_addr The physical start address of the bank header.
 * @param[in] state          The new bank state to be committed.
 *
 * @return EEPROMOk on success, or a flash-specific error code on failure.
 */
static eeprom_error_t _bank_set_state(uint32_t bank_phys_addr, eeprom_bank_state_t state) {
	eeprom_error_t ret;
	
	// Read bank header
	eeprom_bank_header_t header;
  
	if ((ret = _flash_read(bank_phys_addr, &header, sizeof(eeprom_bank_header_t))) != EEPROMOk) {
		return ret;
	}
  
	// Update state
	header.state = state;

	// Write
	if ((ret = _flash_write(bank_phys_addr, &header, sizeof(header.state))) != EEPROMOk) {
		return ret;
	}
	
	return EEPROMOk;
}

/**
 * @brief Executes the Atomic Commit of a pending transaction.
 *
 * The final step of a GC event. Promotes the shadow workspace (spare bank) 
 * to 'Active' status and reclaims the old bank. This transition is fully 
 * power-fail safe as the state update precedes the erase.
 *
 * @return EEPROMOk on success, or flash error code.
 */
 static eeprom_error_t _bank_swap(void) {
	eeprom_error_t ret;
	eeprom_bank_header_t active_header;
	
	// Retrieve bank metadata
	if ((ret = _flash_read(eeprom_ctx.active_bank, &active_header, sizeof(eeprom_bank_header_t))) != EEPROMOk) {
		return ret;
	}

	// Swap complete, promote the spare bank to 'Active' status
	if ((ret = _bank_set_state(eeprom_ctx.spare_bank, EEPROMBankActive)) != EEPROMOk) {
		return ret;
	}

	// Reclaim the old active bank, erase it and increment the wear-leveling counter
	if ((ret = _bank_format(eeprom_ctx.active_bank, active_header.erase_count + 1, EEPROMBankFree)) != EEPROMOk) {
		return ret;
	}

	// Swap
	uint32_t active_bank = eeprom_ctx.active_bank;

	eeprom_ctx.active_bank = eeprom_ctx.spare_bank;	
	eeprom_ctx.spare_bank  = active_bank;
	eeprom_ctx.write_bank  = eeprom_ctx.active_bank;
	eeprom_ctx.read_cursor = eeprom_ctx.active_bank + sizeof(eeprom_bank_header_t);

	return EEPROMOk;
}

/**
 * @brief Initiates Garbage Collection.
 *
 * Prepares the spare bank as a "shadow workspace" for a pending transaction. 
 * Migrates valid pages while ignoring the logical range about to be overwritten. 
 * Rediriges the global write context to the spare bank to allow atomic commits.
 *
 * @param[in] initial_ig Start of logical index range to exclude.
 * @param[in] final_ig   End of logical index range to exclude.
 *
 * @return EEPROMOk on success, or flash error code.
 */
 static eeprom_error_t _gc_collect(eeprom_page_idx_t initial_ig, eeprom_page_idx_t final_ig) {
	eeprom_error_t ret;
	eeprom_bank_header_t active_header;
	eeprom_bank_header_t spare_header;
	
	bool in_ignore_range;
	
	// Retrieve bank metadata to verify structural consistency before garbage collection (gc)
	if ((ret = _flash_read(eeprom_ctx.active_bank, &active_header, sizeof(eeprom_bank_header_t))) != EEPROMOk) {
		return ret;
	}

	if ((ret = _flash_read(eeprom_ctx.spare_bank, &spare_header, sizeof(eeprom_bank_header_t))) != EEPROMOk) {
		return ret;
	}

	// Ensure banks are in the correct state to begin gc
	if ((active_header.state == EEPROMBankActive) && (spare_header.state == EEPROMBankFree)) {
		// Mark spare bank as 'Swapping' to protect integrity during data migration		
 		if ((ret = _bank_set_state(eeprom_ctx.spare_bank, EEPROMBankSwapping)) != EEPROMOk) {
			return ret;
		}

		// Initialize iteration cursors
		uint32_t phys_page_active_addr = eeprom_ctx.active_bank + sizeof(eeprom_bank_header_t);
		uint32_t phys_page_spare_addr = eeprom_ctx.spare_bank + sizeof(eeprom_bank_header_t);
		
		eeprom_page_t page;
	    
		// Linear scan of the active bank to swap valid data
		while (phys_page_active_addr < eeprom_ctx.active_bank + eeprom_ctx.bank_size) {
			if ((ret = _flash_read(phys_page_active_addr, &page, sizeof(eeprom_page_t))) != EEPROMOk) {
				return ret;
			}
			
			if (page.header.state == EEPROMPageActive) {
				in_ignore_range = ((page.header.page_idx >= initial_ig) && (page.header.page_idx <= final_ig));
							
				if (!in_ignore_range) {							 				
					if ((ret = _flash_write(phys_page_spare_addr, &page, sizeof(eeprom_page_t))) != EEPROMOk) {
						return ret;
		 			}
					
					phys_page_spare_addr += sizeof(eeprom_page_t);						
				}				 		
			} else if (page.header.state == EEPROMPageFree) {
				// Optimization, terminate scan upon reaching the unwritten area
				break;
			}
	
			phys_page_active_addr += sizeof(eeprom_page_t);
		}

		eeprom_ctx.write_bank = eeprom_ctx.spare_bank;
		eeprom_ctx.write_cursor = phys_page_spare_addr;
	} else {
		// Potential structural corruption
		return EEPROMFLASHIntegrityError;
	}

	return EEPROMOk;
}

/**
 * @brief Resolves the next available physical slot with Bank-Awareness.
 *
 * Returns the physical address for the next write. It is context-aware: 
 * finding space in the Active bank during normal ops, or the Spare (Shadow) 
 * bank during a GC transaction.
 *
 * @return Physical address of the next free slot.
 */
static uint32_t _page_write_cursor_get(void) {
    uint32_t phys_addr;
    eeprom_page_t page;
    eeprom_error_t ret;

    // Check if the write cursor is already cached and valid
    if (eeprom_ctx.write_cursor != EEPROM_PAGE_NOT_FOUND) {
        if (eeprom_ctx.write_cursor < (eeprom_ctx.write_bank + eeprom_ctx.bank_size)) {
            return eeprom_ctx.write_cursor;
        }
		
        return EEPROM_PAGE_NOT_FOUND;
    }

	// Write cursor not cached, search for first free page in the write bank
    phys_addr = eeprom_ctx.write_bank + sizeof(eeprom_bank_header_t);

    while (phys_addr < (eeprom_ctx.write_bank + eeprom_ctx.bank_size)) {        
        if ((ret = _flash_read(phys_addr, &page, sizeof(eeprom_page_t))) != EEPROMOk) {
            return EEPROM_PAGE_NOT_FOUND;
        }
        
        _update_cursors(&page.header, phys_addr);
        
        if (page.header.state == EEPROMPageFree) {
            eeprom_ctx.write_cursor = phys_addr;
            return phys_addr;
        }

        phys_addr += sizeof(eeprom_page_t);
    }
    
    return EEPROM_PAGE_NOT_FOUND;
}

/**
 * @brief Advances the internal write cursor to the next available physical page.
 *
 * This function calculates the address of the next 'Free' slot in the active 
 * bank. If the cursor is uninitialized, it performs a search to find the 
 * first available space. The pointer is automatically incremented by the 
 * page size for sequential allocations.
 *
 * @return The physical address of the next free page, or EEPROM_PAGE_NOT_FOUND 
 * if the bank is exhausted and requires a swap.
 */
 static uint32_t _page_write_cursor_advance(void) {
     // If the cursor is not yet established, perform an initial discovery
     if (eeprom_ctx.write_cursor == EEPROM_PAGE_NOT_FOUND) {
         return _page_write_cursor_get();
     }
     
     // Advance the cursor by exactly one physical page unit
     uint32_t next_page = eeprom_ctx.write_cursor + sizeof(eeprom_page_t);
     
     // Ensure the new address does not exceed the active bank's memory limits.
     // If we hit the boundary, we return NOT_FOUND to trigger a bank swap.
     if (next_page >= (eeprom_ctx.write_bank + eeprom_ctx.bank_size)) {
         eeprom_ctx.write_cursor = EEPROM_PAGE_NOT_FOUND;
         return EEPROM_PAGE_NOT_FOUND;
     }

     // Update cursors
     eeprom_ctx.write_cursor = next_page;
     
     return eeprom_ctx.write_cursor;
 }

 /**
  * @brief Updates physical page state and enforces Read-Cursor Consistency.
  *
  * Changes the flash page header. Whenever a page is activated or invalidated, 
  * it clears the cached read cursor to force a fresh scan by the lookup engine.
  *
  * @param[in] phys_addr Physical address of the page.
  * @param[in] page_h    Pointer to the page header metadata.
  * @param[in] state     The new state to apply (Active/Inactive).
  *
  * @return EEPROMOk on success, or flash error code.
  */
static eeprom_error_t _page_set_state(uint32_t phys_addr, eeprom_page_header_t *page_h, eeprom_page_state_t state) {
	if ((page_h->state == EEPROMPageActive) && (eeprom_ctx.read_cursor == phys_addr)) {
		eeprom_ctx.read_cursor = EEPROM_PAGE_NOT_FOUND;		
	}
	
	// Write new state
	page_h->state = state;
  
	// Compute the precise physical address of the 'state' field
	uint32_t state_addr = phys_addr + offsetof(eeprom_page_t, header) + 
	                                  offsetof(eeprom_page_header_t, state);
									  	
	return _flash_write(state_addr, &state, sizeof(page_h->state));
}

/**
 * @brief Verifies if a page is physically erased (all bits set to 1).
 *
 * Performs a bitwise inspection of the page buffer to ensure it matches the 
 * factory-erased state (0xFF). This is used during initialization and 
 * recovery to validate that 'Free' pages are truly blank and ready for 
 * new data, preventing write collisions.
 *
 * @param[in] page Pointer to the page buffer to inspect.
 *
 * @return true if the page is entirely 0xFF, false otherwise.
 */
static bool _page_check_blank(eeprom_page_t *page) {
	const uint8_t *ptr = (const uint8_t *)page;
	const uint8_t *end = ptr + sizeof(eeprom_page_t);
	    
	// Iterate through the buffer to detect any non-erased bits
    while (ptr < end) {
	    if (*ptr++ != 0xFF) {
            return false;
        }
    }
	return true;
}

/**
 * @brief Resolves the physical flash address for a given logical page index.
 *
 * Implements a hybrid lookup strategy: first attempts a predictive 
 * sequential access check based on the previous lookup, then falls back 
 * to a linear scan of the active bank if the fast-path fails.
 *
 * @param[in]  page_idx The logical page index (VPN) to locate.
 * @param[out] page     Pointer to a buffer where the page data will be loaded.
 * @return The physical address in flash, or EEPROM_PAGE_NOT_FOUND if missing.
 */
static uint32_t _page_lookup(eeprom_page_idx_t page_idx, eeprom_page_t *page) {
	eeprom_error_t ret;
	uint32_t phys_addr;
	
	// Quick exit, check allocated bits to see if the page was ever allocated
	if (!_is_page_allocated(page_idx)) {
		return EEPROM_PAGE_NOT_FOUND;
	}
	
	// Optimization, check for sequential access
	if ((eeprom_ctx.last_lookup != EEPROM_PAGE_NOT_FOUND) && (page_idx == eeprom_ctx.last_lookup_idx + 1)) {
		// Predictive check, verify the adjacent page
		phys_addr = eeprom_ctx.last_lookup + sizeof(eeprom_page_t);
		
		if ((ret = _flash_read(phys_addr, page, sizeof(eeprom_page_t))) == EEPROMOk) {
			_update_cursors(&page->header, phys_addr);
		
			if ((page->header.state == EEPROMPageActive) && (page->header.page_idx == page_idx)) {
				eeprom_ctx.last_lookup = phys_addr;
				eeprom_ctx.last_lookup_idx = page_idx;
			
				return phys_addr;
			}
		}	
	}
	
	// Fallback, perform a linear scan starting from the first known active page
	if (eeprom_ctx.read_cursor != EEPROM_PAGE_NOT_FOUND) {
    	phys_addr = eeprom_ctx.read_cursor;
	} else {
		phys_addr = eeprom_ctx.active_bank + sizeof(eeprom_bank_header_t);
	}	

	// Iterate through the active bank
	while (phys_addr < (eeprom_ctx.active_bank + eeprom_ctx.bank_size)) {
		// Read page
		if ((ret = _flash_read(phys_addr, page, sizeof(eeprom_page_t))) != EEPROMOk) {
			return EEPROM_PAGE_NOT_FOUND;
		}

		_update_cursors(&page->header, phys_addr);
		
		if (page->header.state == EEPROMPageFree) {
			// Page is free, like others until the end of the bank
			return EEPROM_PAGE_NOT_FOUND;
		}
				
		if ((page->header.state == EEPROMPageActive) && (page->header.page_idx == page_idx)) {
			// Page located
			
			// Update cache for potential future sequential access
			eeprom_ctx.last_lookup = phys_addr;
			eeprom_ctx.last_lookup_idx = page_idx;
			
			return phys_addr;
		}

    	phys_addr += sizeof(eeprom_page_t);
	}

	return EEPROM_PAGE_NOT_FOUND;
}

/**
 * @brief Adds a new logical page to the physical flash storage.
 *
 * This function handles the allocation of a physical page, performs a bank 
 * swap if the active bank is exhausted, and writes the data to flash. 
 * It initializes the page with a 0xFF mask before overlaying the new data 
 * at the specified offset to preserve the erased state of unused bytes.
 *
 * @param[in]  page_idx The logical index of the page to be added.
 * @param[in]  data     Pointer to the source buffer containing the new data.
 * @param[in]  offset   Intra-page offset where the data write should begin.
 * @param[in]  size     Number of bytes to write (must not exceed page size).
 *
 * @return EEPROMOk if successful, or an error code indicating the failure:
 * - EEPROMFLASHNoLeftSpace: No free pages available after swap.
 * - EEPROM_PAGE_NOT_FOUND: Lookup failed after relocation.
 * - Other flash-specific error codes.
 */
static eeprom_error_t _page_add(eeprom_page_idx_t page_idx, void *data, uint8_t offset, size_t size) {
	eeprom_error_t ret;
	uint32_t phys_addr;
	
	// Acquire the the write cursor
	if ((phys_addr = _page_write_cursor_get()) == EEPROM_PAGE_NOT_FOUND) {
		assert(false);
		#if 0
		// Active bank is full, perform bank swap to reclaim space
		if ((ret = _bank_swap(eeprom_ctx.active_bank, eeprom_ctx.spare_bank, EEPROM_PAGE_ID_NONE, EEPROM_PAGE_ID_NONE)) != EEPROMOk) {
			return ret;
		}

		// Retry acquisition after swap, if it fails again, flash is exhausted
		if ((phys_addr = _page_write_cursor_get()) == EEPROM_PAGE_NOT_FOUND) {
			return EEPROMFLASHNoLeftSpace;
		}
		#endif
	}

	// Initialize the new page structure
	eeprom_page_t page;

	page.header.state    = EEPROMPageFree;
	page.header.page_idx = page_idx;
	
	// Prepare data: start with erased state (0xFF) and overlay new content
	memset(page.data, 0xff, EEPROM_PAGE_SIZE);
	memcpy(page.data + offset, data, size);

	// Write the page to physical flash
	if ((ret = _flash_write(phys_addr, &page, sizeof(eeprom_page_t))) != EEPROMOk) {
		return ret;
	}

	// Transition the page state from Free to Active
	if ((ret = _page_set_state(phys_addr, &page.header, EEPROMPageActive)) != EEPROMOk) {
		return ret;
	}
	
	_page_write_cursor_advance();
	_set_page_allocated(page_idx);
	
	return EEPROMOk;		
}

/**
 * @brief Performs a data update on an existing logical page.
 *
 * Since flash memory cannot be overwritten without an erase cycle, this 
 * function updates a page by merging the existing data with the new 
 * modifications into a temporary buffer, then committing the result as a 
 * new physical entry via _page_add().
 *
 * @param[in]  page_idx The logical index of the page to be updated.
 * @param[in]  main     Pointer to the current valid data of the page.
 * @param[in]  offset   Intra-page offset where the update should be applied.
 * @param[in]  data     Pointer to the source buffer with the new data bits.
 * @param[in]  size     Number of bytes to update within the page.
 *
 * @return EEPROMOk if the update and subsequent addition were successful, 
 * or an error code from the underlying flash operations.
 */
static eeprom_error_t _page_update(eeprom_page_idx_t page_idx, void *main, uint8_t offset, void *data, size_t size) {
	uint8_t new_data[EEPROM_PAGE_SIZE];
	
	// Copy the existing page data (main argument) into the reconstruction buffer
	memcpy(new_data, main, EEPROM_PAGE_SIZE);
	
	// Overlay the new data at the specified intra-page offset
	memcpy(new_data + offset, data, size);

	// Commit the reconstructed page as a new physical entry
	return _page_add(page_idx, new_data, 0, EEPROM_PAGE_SIZE);
}

/**
 * @brief Initializes the entire EEPROM storage area to a factory-reset state.
 *
 * This function erases and prepares both physical banks, setting their
 * initial erase counts and headers. It designates Bank 0 as the initial 
 * Active bank and Bank 1 as the Spare. All management metadata, including 
 * the allocation bitmap and read / write cursors, are cleared.
 *
 * @return EEPROMOk on success, or a flash-specific error code on failure.
 */
static eeprom_error_t _format(void) {
	eeprom_error_t ret;
	
	// Initialize both physical banks to an erased 'Free' state
	if ((ret = _bank_format(EEPROM_BANK0_OFFSET, 1, EEPROMBankFree)) != EEPROMOk) {
		return ret;
	}

	if ((ret = _bank_format(EEPROM_BANK1_OFFSET, 2, EEPROMBankFree)) != EEPROMOk) {
		return ret;
	}

	// Transition Bank 0 to the Active state to begin accepting data
	if ((ret = _bank_set_state(EEPROM_BANK0_OFFSET, EEPROMBankActive)) != EEPROMOk) {
		return ret;
	}

	// Configure the global context for initial operation
	eeprom_ctx.active_bank = EEPROM_BANK0_OFFSET;
	eeprom_ctx.spare_bank  = EEPROM_BANK1_OFFSET;
	eeprom_ctx.write_bank  = eeprom_ctx.active_bank; 

	eeprom_ctx.write_cursor   = EEPROM_BANK0_OFFSET + sizeof(eeprom_bank_header_t);
	eeprom_ctx.read_cursor = EEPROM_PAGE_NOT_FOUND;
	
	eeprom_ctx.last_lookup = EEPROM_PAGE_NOT_FOUND;
	
	memset(eeprom_ctx.allocated_bits, 0x00, eeprom_ctx.allocated_bits_size);

	return EEPROMOk;
}

/*
 * Operation functions
 */

driver_error_t *eeprom_init(void) {
	eeprom_error_t ret;
	uint32_t erase_cnt = 0;

	
	// Find EEPROM partition
	eeprom_ctx.partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, LUA_RTOS_EEPROM_PART, NULL);
	if (eeprom_ctx.partition == NULL) {
		return driver_error(EEPROM_DRIVER, EEPROM_ERR_INVALID_PARTITION, NULL);
	}
	
	// Calculate the number of pages per bank
	uint32_t bank_pages = (((eeprom_ctx.partition->size >> 1) - sizeof(eeprom_bank_header_t)) / sizeof(eeprom_page_t));
	
	// Calculate the bank size
	eeprom_ctx.bank_size = (sizeof(eeprom_bank_header_t) + bank_pages * sizeof(eeprom_page_t));
	
	// Calculate the size required for allocated bits
	eeprom_ctx.allocated_bits_size = (bank_pages >> 3) + 1;
	
	// Allocate space for allocated bits
	eeprom_ctx.allocated_bits = calloc(eeprom_ctx.allocated_bits_size, 1);
	if (!eeprom_ctx.allocated_bits) {
		return driver_error(EEPROM_DRIVER, EEPROM_ERR_NO_MEM, NULL);		
	}
	
	// Index initialization
	eeprom_ctx.read_cursor  = EEPROM_PAGE_NOT_FOUND;
	eeprom_ctx.write_cursor = EEPROM_PAGE_NOT_FOUND;
	eeprom_ctx.last_lookup  = EEPROM_PAGE_NOT_FOUND;

	eeprom_bank_header_t bank0_header;
	eeprom_bank_header_t bank1_header;
	
	for(;;) {
		// Read bank headers
		if ((ret = _flash_read(EEPROM_BANK0_OFFSET, &bank0_header, sizeof(eeprom_bank_header_t))) != EEPROMOk) {
			return driver_error(EEPROM_DRIVER, EEPROM_ERR_FLASH_ACCESS_ERROR, "bank0");
		}

		if ((ret = _flash_read(EEPROM_BANK1_OFFSET, &bank1_header, sizeof(eeprom_bank_header_t))) != EEPROMOk) {
			return driver_error(EEPROM_DRIVER, EEPROM_ERR_FLASH_ACCESS_ERROR, "bank1");
		}

		// Check if partition is formatted, or need to recover due to an unfinished previous format (power loss)
		bool format;
		
		// Signature numbers should be present in the banks
		format = ((bank0_header.signature_h != EEPROM_SIGNATURE_H) || (bank0_header.signature_l != EEPROM_SIGNATURE_L));
		format = format || ((bank1_header.signature_h != EEPROM_SIGNATURE_H) && (bank1_header.signature_l != EEPROM_SIGNATURE_L));
		
		// Only 1 bank should be in the EEPROMBankFree state
		bool recovery = ((bank0_header.state == EEPROMBankFree) && (bank1_header.state == EEPROMBankFree));
		
		if (format || recovery) {
			// Need to format
			syslog(LOG_INFO, "eeprom formatting (%s) ...", (format?"initial":"recovery"));
			
			if ((ret = _format()) != EEPROMOk) {
				syslog(LOG_ERR, "eeprom formatting error");
				
				return _get_error(ret);
			}		
			
			// After formatting we can ensure that EEPROM is consistent
			// At this point BANK0 is active, BANK1 is free, and indexes are updated 
			syslog(LOG_INFO, "eeprom formatting done");
			
			break;
		}
		
		// At this point we can ensure that the partition is formatted											
		if ((bank0_header.state == EEPROMBankActive) && (bank1_header.state == EEPROMBankActive)) {
			// Only 1 bank should be in EEPROMBankActive state, otherwise a bank swap failed  (power loss, wear-out)
			syslog(LOG_ERR, "eeprom swap recovery");

			// Set the active bank to the one with highest erase_count, and format the other one
			if (bank0_header.erase_count > bank1_header.erase_count) {
				if ((ret =_bank_format(EEPROM_BANK1_OFFSET, bank0_header.erase_count + 1, EEPROMBankFree)) != EEPROMOk) {
					return _get_error(ret);
				}
			} else {
				if ((ret = _bank_format(EEPROM_BANK0_OFFSET, bank1_header.erase_count + 1, EEPROMBankFree)) != EEPROMOk) {
					return _get_error(ret);
				}
			}			
			
			continue;
		} else if ((bank0_header.state == EEPROMBankActive) && (bank1_header.state == EEPROMBankSwapping)) {
			// A bank swap from 0 to 1 wasn't end, need format bank 1
			if ((ret = _bank_format(EEPROM_BANK1_OFFSET, bank0_header.erase_count + 1, EEPROMBankFree)) != EEPROMOk) {
				return _get_error(ret);
			}
								
			continue;	
		} else if ((bank1_header.state == EEPROMBankActive) && (bank0_header.state == EEPROMBankSwapping)) {
			// A bank swap from 1 to 0 wasn't end, need format bank 0
			if ((ret = _bank_format(EEPROM_BANK0_OFFSET, bank1_header.erase_count + 1, EEPROMBankFree)) != EEPROMOk) {
				return _get_error(ret);
			}					
			
			continue;
		}
		
		if (bank0_header.state == EEPROMBankActive) {
			// Active bank 0, spare 1
			eeprom_ctx.active_bank = EEPROM_BANK0_OFFSET;
			eeprom_ctx.spare_bank  = EEPROM_BANK1_OFFSET;
			
			erase_cnt = bank0_header.erase_count;
		} else if (bank1_header.state == EEPROMBankActive) {
			// Active bank 1, spare 0
			eeprom_ctx.active_bank = EEPROM_BANK1_OFFSET;
			eeprom_ctx.spare_bank  = EEPROM_BANK0_OFFSET;
			
			erase_cnt = bank1_header.erase_count;
	    }
		
		eeprom_ctx.write_bank = eeprom_ctx.active_bank;
	    
		eeprom_page_header_t page_h;
		uint32_t phys_addr = eeprom_ctx.active_bank + sizeof(eeprom_bank_header_t);

		while (phys_addr < eeprom_ctx.active_bank + eeprom_ctx.bank_size) {
			if ((ret = _flash_read(phys_addr, &page_h, sizeof(eeprom_page_header_t))) != EEPROMOk) {	
				return _get_error(ret);
			}

			if (page_h.state == EEPROMPageFree) {
				_update_cursors(&page_h, phys_addr);
				break;
			} else if (page_h.state == EEPROMPageActive) {
				if (_is_page_allocated(page_h.page_idx)) {
					// Integrity error, page address is not unique
					if ((ret = _page_set_state(phys_addr, &page_h, EEPROMPageInactive)) != EEPROMOk) {
						return _get_error(ret);
					}
				} else {
					_set_page_allocated(page_h.page_idx);
					_update_cursors(&page_h, phys_addr);
				}
			}
								
			phys_addr += sizeof(eeprom_page_t);
		}
		
		break;
	}
	
	syslog(LOG_INFO, "eeprom active bank %d, erase cnt %d", (eeprom_ctx.active_bank == EEPROM_BANK0_OFFSET)?0:1, erase_cnt);

	return NULL;
}

 driver_error_t *eeprom_read(uint32_t addr, void *dst, uint32_t size) {
	eeprom_page_t page;
	eeprom_page_idx_t page_idx;

	// Compute the relative offset from the start of the page 
	uint32_t offset = addr & (EEPROM_PAGE_SIZE - 1);

	// Read in chunks of up to EEPROM_PAGE_SIZE
	uint32_t chunk_size;

	while (size > 0) {
		// Compute the page index
		page_idx = (addr >> EEPROM_PAGE_BITS);

		// Compute the chunk size
		chunk_size = ((size < (EEPROM_PAGE_SIZE - offset))?size:(EEPROM_PAGE_SIZE - offset));

		if (_page_lookup(page_idx, &page) == EEPROM_PAGE_NOT_FOUND) {
			memset(dst, 0xff, chunk_size);
		} else {
			memcpy(dst, page.data + offset, chunk_size);
		}

		// Advance pointers and counters
		addr += chunk_size;
		dst   = (uint8_t *)dst + chunk_size;
		size -= chunk_size;
		
		// Subsequent chunks always start at the beginning of the next page
    	offset = 0;
	}
	
	return NULL;
}

driver_error_t *eeprom_write(uint32_t addr, void *src, size_t size) {
	eeprom_error_t ret;
	eeprom_page_t page;
	eeprom_page_idx_t page_idx;
	uint32_t phys_addr;
	bool garbage_collected = false;

	// To guarantee data integrity during power-loss, we must ensure the active bank 
	// has contiguous free space for the entire transaction before commencing. 
	// If the required size (in pages) exceeds the remaining space between the 
	// write_cursor and the end of the bank, the garbage collector is triggered 
	// immediately. This prevents a split-write scenario where half the data is 
	// written to the active bank and the remainder to spare bank, which would break
	// atomicity if power is lost mid-process.
	
	// Calculate affected logical pages regardless of size
    uint16_t initial_page = (addr >> EEPROM_PAGE_BITS);
    uint16_t final_page = ((addr + size - 1) >> EEPROM_PAGE_BITS);
    uint16_t affected_pages = final_page - initial_page + 1;

    // Check for contiguous space before commencing
    uint32_t write_cursor = _page_write_cursor_get();
    uint32_t free_pages = (write_cursor == EEPROM_PAGE_NOT_FOUND) ? 0 :
                          ((eeprom_ctx.active_bank + eeprom_ctx.bank_size - write_cursor) / sizeof(eeprom_page_t));

    if (affected_pages > free_pages) {
		// Reclaim space in the spare bank before we start writing any chunks.
		// We pass the range [initial_page, final_page] to avoid migrating obsolete data.
		ret = _gc_collect(initial_page, final_page);
        if (ret != EEPROMOk) {
			return _get_error(ret);
		}
		
		garbage_collected = true;
    }
								
	// Compute the relative offset from the start of the page 
	uint32_t offset = addr & (EEPROM_PAGE_SIZE - 1);
	
	// Write in chunks of up to EEPROM_PAGE_SIZE
	uint32_t chunk_size;
	
	while (size > 0) {
		// Compute the page index
		page_idx = (addr >> EEPROM_PAGE_BITS);
		
		// Compute the chunk size		
		chunk_size = ((size < (EEPROM_PAGE_SIZE - offset))?size:(EEPROM_PAGE_SIZE - offset));
		
		// Lookup the physical location of the page in the active flash bank
		if ((phys_addr = _page_lookup(page_idx, &page)) == EEPROM_PAGE_NOT_FOUND) {
			if ((ret = _page_add(page_idx, src, offset, chunk_size)) != EEPROMOk) {
				return _get_error(ret);
			}
		} else {
			if ((ret = _page_update(page_idx, page.data, offset, src, chunk_size)) != EEPROMOk) {
				return _get_error(ret);
			}
			
			// Invalidate the previous physical instance of the page
			if (!garbage_collected) {
				if ((ret = _page_set_state(phys_addr, &page.header, EEPROMPageInactive)) != EEPROMOk) {
					return _get_error(ret);
				}
			}	
		}

		// Advance pointers and counters
		addr += chunk_size;
		src   = (uint8_t *)src + chunk_size;
		size -= chunk_size;
		
		// Subsequent chunks always start at the beginning of the next page
		offset = 0;
	}
	
	if (garbage_collected) {
		_bank_swap();
	}
	
	return NULL;
}

driver_error_t *eeprom_erase(void) {
	eeprom_error_t ret;
	
	if ((ret = _format()) != EEPROMOk) {
		return _get_error(ret);
	}
	
	return NULL;
}	