/* Linker script to configure memory regions. */
/* This is a slightly adjust copy from $NRF5_DIR/config/nrf52832/armgcc/generic_gcc_nrf52.ld */
/* TODO: make RAM (rwx) :  ORIGIN = 0x200021B0, LENGTH = 0xDE50 be filled in from RAM_R1_BASE and RAM_APPLICATION_AMOUNT */
/*
MEMORY
{
  FLASH (rx) : ORIGIN = 0x2d000, LENGTH = 0x53000
  RAM (rwx) :  ORIGIN = 0x20001f30, LENGTH = 0xe0d0
}
*/

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

/**
 * See S132_nrf52_6.0.0_release_notes.pdf. 
 *
 * Combined MBR and softdevice requirements:
 *   FLASH: 152.00 kB (0x26000 bytes)
 *   RAM:     5.54 kB (0x01628 bytes), but depending on the configuration when sd_ble_enable() is called
 *
 * Note that this information is from the perspective of the application to be compiled.
 *   So, we start where the SoftDevice ends... At 0x26000. 
 *   Total memory is 512 kB, so the size available to the application is 0x80000 - 0x26000 = 0x5A000.
 * 
 * According to nrf_sd_ble debug information RAM start location can be adjusted to 0x200020B0 and the RAM size
 * for the application to 0xDF50
 *
 * RAM (rwx) :  ORIGIN = 0x20002430, LENGTH = 0xdad0
 *
 * Update the sections using a recent flash_placement.xml file...
 */

MEMORY
{
  FLASH (rx) : ORIGIN = 0x26000, LENGTH = 0x5A000
  RAM (rwx) :  ORIGIN = 0x20002380, LENGTH = 0xDC80
}

SECTIONS
{
}

SECTIONS
{
  . = ALIGN(4);
  .mem_section_dummy_ram :
  {
  }
  .log_dynamic_data :
  {
    PROVIDE(__start_log_dynamic_data = .);
    KEEP(*(SORT(.log_dynamic_data*)))
    PROVIDE(__stop_log_dynamic_data = .);
  } > RAM
  .log_filter_data :                                                                                                    
  {                                                                                                                     
    PROVIDE(__start_log_filter_data = .);                                                                               
    KEEP(*(SORT(.log_filter_data*)))                                                                                    
    PROVIDE(__stop_log_filter_data = .);                                                                                
  } > RAM    
  .cli_sorted_cmd_ptrs :
  {
    PROVIDE(__start_cli_sorted_cmd_ptrs = .);
    KEEP(*(.cli_sorted_cmd_ptrs))
    PROVIDE(__stop_cli_sorted_cmd_ptrs = .);
  } > RAM
  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > RAM
  .nrf_mesh_ram :
  {
    PROVIDE(__start_nrf_mesh_ram = .);
    KEEP(*(SORT(.nrf_mesh_ram.*)))
    PROVIDE(__stop_nrf_mesh_ram = .);
  } > RAM

} INSERT AFTER .data;

SECTIONS
{
  .mem_section_dummy_rom :
  {
  }
  .sdh_ble_observers :
  {
    PROVIDE(__start_sdh_ble_observers = .);
    KEEP(*(SORT(.sdh_ble_observers*)))
    PROVIDE(__stop_sdh_ble_observers = .);
  } > FLASH
  .sdh_soc_observers :
  {
    PROVIDE(__start_sdh_soc_observers = .);
    KEEP(*(SORT(.sdh_soc_observers*)))
    PROVIDE(__stop_sdh_soc_observers = .);
  } > FLASH
  .sdh_state_observers :
  {
    PROVIDE(__start_sdh_state_observers = .);
    KEEP(*(SORT(.sdh_state_observers*)))
    PROVIDE(__stop_sdh_state_observers = .);
  } > FLASH
  .sdh_stack_observers :
  {
    PROVIDE(__start_sdh_stack_observers = .);
    KEEP(*(SORT(.sdh_stack_observers*)))
    PROVIDE(__stop_sdh_stack_observers = .);
  } > FLASH
  .sdh_req_observers :
  {
    PROVIDE(__start_sdh_req_observers = .);
    KEEP(*(SORT(.sdh_req_observers*)))
    PROVIDE(__stop_sdh_req_observers = .);
  } > FLASH
  .log_const_data :
  {
    PROVIDE(__start_log_const_data = .);
    KEEP(*(SORT(.log_const_data*)))
    PROVIDE(__stop_log_const_data = .);
  } > FLASH
  .log_backends :                                                                                                       
  {                                                                                                                     
    PROVIDE(__start_log_backends = .);                                                                                  
    KEEP(*(SORT(.log_backends*)))                                                                                       
    PROVIDE(__stop_log_backends = .);                                                                                   
  } > FLASH        
  .nrf_balloc :
  {
    PROVIDE(__start_nrf_balloc = .);
    KEEP(*(.nrf_balloc))
    PROVIDE(__stop_nrf_balloc = .);
  } > FLASH
    .nrf_queue :
  {
    PROVIDE(__start_nrf_queue = .);
    KEEP(*(.nrf_queue))
    PROVIDE(__stop_nrf_queue = .);
  } > FLASH
    .cli_command :
  {
    PROVIDE(__start_cli_command = .);
    KEEP(*(.cli_command))
    PROVIDE(__stop_cli_command = .);
  } > FLASH
  .crypto_data :
  {
    PROVIDE(__start_crypto_data = .);
    KEEP(*(SORT(.crypto_data*)))
    PROVIDE(__stop_crypto_data = .);
  } > FLASH
  .pwr_mgmt_data :
  {
    PROVIDE(__start_pwr_mgmt_data = .);
    KEEP(*(SORT(.pwr_mgmt_data*)))
    PROVIDE(__stop_pwr_mgmt_data = .);
  } > FLASH
  .nrf_mesh_flash :
  {
    PROVIDE(__start_nrf_mesh_flash = .);
    KEEP(*(SORT(.nrf_mesh_flash.*)))
    PROVIDE(__stop_nrf_mesh_flash = .);
  } > FLASH
} INSERT AFTER .text

INCLUDE "nrf_common.ld"
