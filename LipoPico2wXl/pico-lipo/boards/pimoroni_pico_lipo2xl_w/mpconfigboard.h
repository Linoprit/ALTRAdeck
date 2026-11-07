// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "Pimoroni Pico LiPo 2XL W"
// #define MICROPY_HW_FLASH_STORAGE_BYTES          (PICO_FLASH_SIZE_BYTES - 1024 * 1024 * 2)

#define MICROPY_PY_NETWORK_HOSTNAME_DEFAULT     "PicoLiPo2XLW"

// Enable PPP
#define MICROPY_PY_NETWORK_PPP_LWIP             (1)

// Enable networking.
#define MICROPY_PY_NETWORK 1

// CYW43 driver configuration.
#define CYW43_USE_SPI (1)
#define CYW43_LWIP (1)
#define CYW43_GPIO (1)
#define CYW43_SPI_PIO (1)

// For debugging mbedtls - also set
// Debug level (0-4) 1=warning, 2=info, 3=debug, 4=verbose
// #define MODUSSL_MBEDTLS_DEBUG_LEVEL 1

#ifndef CYW43_WL_GPIO_COUNT
#define CYW43_WL_GPIO_COUNT 3
#endif

#define MICROPY_HW_PIN_EXT_COUNT CYW43_WL_GPIO_COUNT

int mp_hal_is_pin_reserved(int n);
#define MICROPY_HW_PIN_RESERVED(i) mp_hal_is_pin_reserved(i)

// PSRAM Settings
// #define MICROPY_HW_ENABLE_PSRAM                 (1)
// #define MICROPY_HW_PSRAM_CS_PIN                 PIMORONI_PICO_LIPO2_PSRAM_CS_PIN
#define MICROPY_GC_SPLIT_HEAP                   (1)
