#ifndef ECU_COMMON_H
#define ECU_COMMON_H

#include <stdint.h>
#include <stddef.h>

enum SpiNetwork : uint8_t {
    SPI_NETWORK_RASPBERRY,
    SPI_NETWORK_SPARE,
    SPI_NETWORK_COUNT,
};

#endif // ECU_COMMON_H
