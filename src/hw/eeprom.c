#include "pw_internal.h"

#define EEPROM_PAGE_SIZE 0x80
#define SPI_ERROR_FLAG   (1 << 6)

static void clear_spi_error(pw_state_t *s) {
    wr8(s, RAM_common_bit_flags, rd8(s, RAM_common_bit_flags) & ~SPI_ERROR_FLAG);
}

static uint8_t hal_read_byte(pw_state_t *s, uint16_t addr) {
    uint8_t b;
    s->hal->eeprom_read(s->hal->user, addr, &b, 1);
    return b;
}

static void hal_write_byte(pw_state_t *s, uint16_t addr, uint8_t b) {
    s->hal->eeprom_write(s->hal->user, addr, &b, 1);
}

static uint16_t page_wrap(uint16_t start, uint16_t offset) {
    return (start & ~(EEPROM_PAGE_SIZE - 1)) | ((start + offset) & (EEPROM_PAGE_SIZE - 1));
}

void eepromReadToRamAlso(pw_state_t *s, uint16_t eeprom_addr, uint16_t ram_dst, uint16_t len) {
    clear_spi_error(s);
    if (len == 0) {
        wr8(s, ram_dst, 0);
    }
    for (uint16_t i = 0; i < len; i++) {
        wr8(s, ram_dst + i, hal_read_byte(s, eeprom_addr + i));
    }
}

uint8_t eepromReadSingleByte(pw_state_t *s, uint16_t eeprom_addr) {
    clear_spi_error(s);
    return hal_read_byte(s, eeprom_addr);
}

void eepromWriteData(pw_state_t *s, uint16_t eeprom_addr, uint16_t ram_src, uint16_t len) {
    clear_spi_error(s);
    for (uint16_t i = 0; i < len; i++) {
        hal_write_byte(s, eeprom_addr + i, rd8(s, ram_src + i));
    }
}

uint8_t eepromWriteSingleByte(pw_state_t *s, uint16_t eeprom_addr, uint8_t value) {
    clear_spi_error(s);
    hal_write_byte(s, eeprom_addr, value);
    return rd8(s, RAM_common_bit_flags);
}

void eepromFillWithByte(pw_state_t *s, uint16_t eeprom_addr, uint16_t len, uint8_t value) {
    clear_spi_error(s);
    for (uint16_t i = 0; i < len; i++) {
        hal_write_byte(s, eeprom_addr + i, value);
    }
}

void eepromFillPageWithByte(pw_state_t *s, uint16_t eeprom_addr, uint8_t value) {
    clear_spi_error(s);
    for (uint16_t i = 0; i < EEPROM_PAGE_SIZE; i++) {
        hal_write_byte(s, page_wrap(eeprom_addr, i), value);
    }
}

void eepromWrite128Bytes(pw_state_t *s, uint16_t eeprom_addr, uint16_t ram_src) {
    clear_spi_error(s);
    for (uint16_t i = 0; i < EEPROM_PAGE_SIZE; i++) {
        hal_write_byte(s, page_wrap(eeprom_addr, i), rd8(s, ram_src + i));
    }
}

static uint8_t checksum(pw_state_t *s, uint16_t ram_addr, uint16_t len) {
    uint8_t sum = 1;
    for (uint16_t i = 0; i < len; i++) {
        sum += rd8(s, ram_addr + i);
    }
    return sum;
}

uint8_t readReliableDataFromEeprom(pw_state_t *s, uint16_t addr0, uint16_t addr1, uint16_t ram_addr, uint16_t len) {
    eepromReadToRamAlso(s, addr1, ram_addr, len);
    uint8_t sum1 = checksum(s, ram_addr, len);
    eepromReadToRamAlso(s, addr0, ram_addr, len);
    uint8_t sum0 = checksum(s, ram_addr, len);

    uint16_t sum_addr0 = addr0 + len;
    uint16_t sum_addr1 = addr1 + len;
    int ok0 = eepromReadSingleByte(s, sum_addr0) == sum0;
    int ok1 = eepromReadSingleByte(s, sum_addr1) == sum1;

    if (ok0 && ok1) {
        if (sum0 == sum1) {
            return sum0;
        }

        eepromWriteData(s, addr1, ram_addr, len);
        return eepromWriteSingleByte(s, sum_addr1, sum0);
    }
    if (ok0) {
        eepromReadToRamAlso(s, addr0, ram_addr, len);
        eepromWriteData(s, addr1, ram_addr, len);
        return eepromWriteSingleByte(s, sum_addr1, sum0);
    }
    if (ok1) {
        eepromReadToRamAlso(s, addr1, ram_addr, len);
        eepromWriteData(s, addr0, ram_addr, len);
        return eepromWriteSingleByte(s, sum_addr0, sum1);
    }
    for (uint16_t i = 0; i < len; i++) {
        wr8(s, ram_addr + i, 0xFF);
    }
    eepromWriteData(s, addr0, ram_addr, len);
    eepromWriteSingleByte(s, sum_addr0, 0xFF);
    eepromWriteData(s, addr1, ram_addr, len);
    return eepromWriteSingleByte(s, sum_addr1, 0xFF);
}

void writeReliableDataToEeprom(pw_state_t *s, uint16_t addr0, uint16_t addr1, uint16_t ram_addr, uint16_t len) {
    eepromWriteData(s, addr0, ram_addr, len);
    uint8_t sum = checksum(s, ram_addr, len);
    eepromWriteSingleByte(s, addr0 + len, sum);
    eepromWriteData(s, addr1, ram_addr, len);
    eepromWriteSingleByte(s, addr1 + len, sum);
}

#define HEAP_SIZE 0x400

void setHeapPointer(pw_state_t *s) {
    wr16(s, RAM_CUR_HEAP_PTR, RAM_HEAP_START);
}

uint16_t static_malloc(pw_state_t *s, uint16_t size) {
    uint16_t ptr = rd16(s, RAM_CUR_HEAP_PTR);
    wr16(s, RAM_CUR_HEAP_PTR, ptr + size);
    (void)HEAP_SIZE;
    return ptr;
}

void copyTeamAndconfigDataFromStagingArea(pw_state_t *s) {
    setHeapPointer(s);
    uint16_t buf = static_malloc(s, 0x80);
    for (uint16_t i = 0; i < 0x52; i++) {
        eepromReadToRamAlso(s, 0xD700 + i * 0x80, buf, 0x80);
        eepromWriteData(s, 0x8F00 + i * 0x80, buf, 0x80);
    }
    for (uint16_t i = 0; i < 5; i++) {
        eepromReadToRamAlso(s, 0xD480 + i * 0x80, buf, 0x80);
        eepromWriteData(s, 0xCC00 + i * 0x80, buf, 0x80);
    }
}
