cmake_minimum_required(VERSION 3.22)

enable_language(C ASM)

set(Defines_Syms
    GD32_ENV
    GD32G5x3
    GD32G5X3)

set(Include_Dirs
    ${CMAKE_SOURCE_DIR}/firmware/cmsis/inc
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Include
    ${CMAKE_SOURCE_DIR}/inc
)

set(Application_Src
    ${CMAKE_SOURCE_DIR}/src/gd32g5x3_init.c
    ${CMAKE_SOURCE_DIR}/src/gd32g5x3_it.c
    ${CMAKE_SOURCE_DIR}/src/main.c
)

set(Drivers_Src
    ${CMAKE_SOURCE_DIR}/gcc_startup/startup_gd32g5x3.S
    ${CMAKE_SOURCE_DIR}/firmware/cmsis/src/syscalls.c
    ${CMAKE_SOURCE_DIR}/firmware/cmsis/src/system_gd32g5x3.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_adc.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_can.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cau.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cau_aes.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cau_des.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cau_tdes.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cla.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cmp.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_cpdm.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_crc.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_dac.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_dbg.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_dma.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_exmc.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_exti.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fac.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fft.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fmc.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fmc_ramfunc.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_fwdgt.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_gpio.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_hpdf.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_hrtimer.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_misc.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_pmu.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_qspi.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_rcu.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_rtc.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_spi.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_syscfg.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_timer.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_tmu.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_trigsel.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_trng.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_usart.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_vref.c
    ${CMAKE_SOURCE_DIR}/firmware/GD32G5x3_standard_peripheral/Source/gd32g5x3_wwdgt.c
)

set(LINK_LIBS
    gd32_drivers
)

add_library(gd32 INTERFACE)
target_include_directories(gd32 INTERFACE ${Include_Dirs})
target_compile_definitions(gd32 INTERFACE ${Defines_Syms})

add_library(gd32_drivers OBJECT)
target_sources(gd32_drivers PRIVATE ${Drivers_Src})
target_link_libraries(gd32_drivers PUBLIC gd32)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${Application_Src})

target_link_libraries(${CMAKE_PROJECT_NAME} ${LINK_LIBS})

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)