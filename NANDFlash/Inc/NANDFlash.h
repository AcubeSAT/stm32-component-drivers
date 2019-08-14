#ifndef STM32_COMPONENT_DRIVERS_NANDFLASH_H
#define STM32_COMPONENT_DRIVERS_NANDFLASH_H

#include <cstdint>
#include "stm32l4xx_hal.h"

/**
 * @page NANDFlash NAND flash
 *
 * @section OpD Operation & Design
 *
 * **NAND flash** is a type of **non-volatile** data memory that is often used on modern electronics. It allows high
 * capacities (in the order of GiBs, depending on the technology) with relatively fast speeds and low costs. It is
 * interesting to note that NAND Flash is the technology used to power most SD cards, USB flash drives and SSDs as of
 * 2019.
 *
 * @image latex nand.jpg "A discrete NAND memory IC" width=4cm
 *
 * The operation of this memory depends on interconnected *floating-gate MOSFETs*, which are transistors that can store
 * charge for a large amount of time.
 *
 * @image latex Nand_flash_structure.pdf "NAND Flash structure (Source: Cyferz)" width=10cm
 * @see https://gr.mouser.com/Semiconductors/Memory-ICs/NAND-Flash/_/N-fb8wq
 *
 * @subsection Organisation
 *
 * NAND memory is organised into:
 * - **Pages**: Areas of typically *2048* consecutive bytes that are **read** and **programmed** together.
 * - **Blocks**: Groups of multiple pages that can be **erased** together.
 * - **Planes**: Separate groups of blocks. There is a possibility to write to many planes simultaneously, in order to
 *   provide a form of redundancy.
 *
 * These devices are **block-based memories**: i.e, **reading and writing happens in groups of multiple bytes, not on
 * single bytes**. If one needs to replace just *1 bit* on the memory, they need to *read and write an entire block*.
 *
 * Another important distinction is the **difference between writing and erasing**: NAND flashes come with all bytes
 * preset to 255 (`11111111`). **Programming** is the operation of changing some of these `1`s to `0`s in order to
 * represent the requested data. In order to *replace* this area of the memory, a separate **erase** operation is
 * required, which writes `0`s to an entire number of pages.
 *
 * |            |  Page |  Block |
 * |:----------:|:-----:|:------:|
 * |    Read    |  Yes  |        |
 * |  Write `0` |  Yes  |        |
 * |    Erase   |       |   Yes  |
 *
 * The numbering of blocks, pages and planes is mentioned in the datasheet of the manufacturer.
 *
 * **Erasing** is the slowest and most expensive operation of the above. The operation of NAND flashes also causes
 * *wear* to the memories. A typical NAND memory will endure 100,000 program/erase cycles, or 1,000 cycles per block.
 * This is not a concern for our mission, since we expect to erase data only a for very small number of times.
 *
 * A NAND flash is expected to have a small number of bad blocks that can be ignored and not operated at all by the
 * user.
 *
 * Flash memories also usually contain a **spare area** with extra bytes for each page. These can contain ECC
 * (Error-Correcting Code) redundant bits, or other information, such as bad block marks.
 *
 * In addition to SLC (single-level cell) memories, where each transistor switches between two (`1` & `0`) states,
 * there are MLC (multi-level cell) memories, where each transistor contains an analog valued charge. These memories
 * offer advantages in density, but suffer in reliability & speed. For our project, we chose the more expensive but
 * more robust SLC memories.
 *
 * @see https://blog.silicon-power.com/index.php/guides/nand-flash-memory-technology-basics/
 * @see https://www.design-reuse.com/articles/24503/nand-flash-memory-embedded-systems.html
 * @see https://www.csd.uoc.gr/~hy428/reading/tn2919.pdf
 *
 * @subsection NANDSDComp Comparison with SD cards
 *
 * NAND flash memories present similar capabilities to SD cards. On a mission-critical embedded system, the advantages
 * of each are:
 * @par Advantages of using an SD card
 *  - Higher capacities on a smaller size
 *  - Standarised interface using STM `SPI` and `SDIO` peripherals
 *  - Simple connectivity to a computer for easier debugging
 *  - No need to take *wear levelling* and bad block management into account, as the SD controller is transparently
 *  responsible for these.
 *  - Ability to purchase industrial and more robust SD cards
 *  - A thriving and evolving market, contrary to NAND Flashes which are difficult to find in non-BGA packages
 *  - Routing possible with a few cables only
 *
 *  @par Advantages of using a discrete NAND flash
 *   - No need to mount, no mechanical parts that may fail, no need for gluing, and no thermal concerns
 *   - Precise control and knowledge of all the internals and operational details
 *   - Native ability to use ECC (Error Correction)
 *   - Native ability for dual redundancy through multi-plane programming
 *   - No lack of technical documentation
 *   - Consistent standards, no trouble with SD card compatibility issues
 *   - Compatibility with `FSMC`/`FMC` STM32 peripheral, with no extra software configuration needed
 *   - Easier hardware debugging, due to the abundance of connections
 *
 * Our decision was to proceed the design with SLC NAND flashes, as they are well supported and offer high reliability
 * on the mechanical and electrical levels.
 *
 * @section STNand NAND Flash on STM
 *
 * NAND flash on the STM32 can be *bit-banged* (i.e. programmed entirely in software using GPIO pins), or programmed
 * using the **FSMC** (flexible static memory controller) or **FMC** peripherals, which set up all the pins and the
 * communication with the memory, based on the NAND operating standards.
 *
 * Sending data to the NAND is possible by **writing to specific memory addresses**, which are defined in the
 * *reference manual* of each MCU. However, writing requires the appropriate commands to be first passed to the memory,
 * as specified in the memory's datasheet. Thus, it is much preferable to use the **HAL** or **LL** libraries'
 * functions in order to communicate with the NAND memory.
 *
 * @image latex fsmc_memory.png "Memory banks reserved by the FSMC on the STM32F407" width=12cm
 *
 * @subsection NANDCubeMX STM32CubeMX configuration
 * The FMSC/FMC peripheral can be enabled on the STM32CubeMX. The configuration settings for the NAND flash banks
 * depend on the specifications of the individual flash memory, as listed on its datasheet. **Timing** is imporant
 * to be specified correctly. Parameters on the STM32CubeMX are multiples of the `HCLK` frequency of the
 * microcontroller. Setting them to high values will work, but more fine-tuning will be needed for increased
 * performance, taking into account the specified timings on the manufacturer's datasheets.
 *
 * @image latex fsmc_cubemx.png "Configuration of the S32ML01G100TF100 memory on STM32CubeMX" width=10cm
 *
 * A typical **13** or more pins will be needed to communicate with the NAND memory. No other components are necessary.
 *
 * @warning
 * For our experiments, we disabled the **NWAIT** pin, due to an erratum (2.11.2 for the STM32F407) that caused an
 * irrecoverable microcontroller hang. Specifying timings with enough margins should eliminate the need for this pin,
 * however.
 *
 * There is also the possibility of enabling ECC computation and checks. The *extra command enable* option depends
 * on the operation of the individual NAND flash, and needed to be on for our checks.
 *
 * @subsection NANDExample Example code using HAL libraries
 * @code{.c}
 *  NAND_AddressTypeDef address = {0,0,0};
 *
 *  uint8_t hello [500];
 *  uint8_t stuff [7000] = "Hello Cubesat OBC and other subsystems!";
 *
 *  HAL_NAND_Erase_Block(&hnand1, &address); // First, erase the block
 *  HAL_NAND_Write_Page_8b(&hnand1, &address, stuff, 3); // Then, write the page
 *  snprintf(hello, 500, "Written\r\n");
 *  HAL_UART_Transmit(&huart1, hello, strlen(hello), 10000);
 *
 *  uint8_t rstuff [2048] = {'b'};
 *  HAL_NAND_Read_Page_8b(&hnand1, &address, rstuff, 1); // Read the page
 *
 *  snprintf(hello, 500, "DATA from NAND: %s\r\n", rstuff);
 *  HAL_UART_Transmit(&huart1, hello, strlen(hello), 10000); // This should show the stored message
 * @endcode
 */

/**
 * The NAND flash peripheral to write and read data from an external non-volatile memory.
 *
 * @note
 * This class is a stub for the time being, and is provided solely as a demonstration of the operation of the NAND
 * flash. The methods of reading from and writing to the NAND flash will mostly depend on the chosen operating system,
 * its interfaces, and its support for low-level NAND operations.
 */
class NANDFlash {
    /**
     * Pointer to the NAND Flash FSMC or FMC handle provided by ST's HAL library
     */
    NAND_HandleTypeDef *nand;
public:
    /**
     * Construct a new NAND flash peripheral
     * @param nand A pointer to the handle provided by HAL
     */
    explicit NANDFlash(NAND_HandleTypeDef *nand);

    /**
     * Performs a write to a specific NAND address
     *
     * @param address The address page, plane and block
     * @param buffer An array where to read the data. The array should have a sufficient size, equal to `bytesPerPage` *
     * \p numPagesToWrite
     * @param numPagesToWrite The number of flash memory pages to write
     */
    inline void write(NAND_AddressTypeDef &address, uint8_t *buffer, uint16_t numPagesToWrite) {
        HAL_NAND_Erase_Block(nand, &address);
        HAL_NAND_Write_Page_8b(nand, &address, buffer, numPagesToWrite);
    }

    /**
     * Performs a read from a specific NAND address
     *
     * @param address The address page, plane and block
     * @param buffer An array where to write the data. The array should have a sufficient size, equal to
     * `bytesPerPage` * \p numPagesToWrite
     * @param numPagesToRead The number of flash memory pages to write
     */
    inline void read(NAND_AddressTypeDef &address, uint8_t *buffer, uint16_t numPagesToRead) {
        HAL_NAND_Read_Page_8b(nand, &address, buffer, numPagesToRead);
    }
};

#endif //STM32_COMPONENT_DRIVERS_NANDFLASH_H
