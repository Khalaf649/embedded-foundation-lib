cmake_minimum_required(VERSION 3.30)
include(cmake/ArmToolchain.cmake)

project(stm32-template C)

set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
enable_language(ASM)

set(PROJECT_PATH                         ${CMAKE_CURRENT_SOURCE_DIR})
include(cmake/Device.cmake)
include(cmake/SourceList.cmake)
include(cmake/IncludeList.cmake)

set(LINKER_SCRIPTS_BASE_DIR              ${PROJECT_PATH}/STM32-base/linker/)
set(STARTUP_BASE_DIR                     ${PROJECT_PATH}/STM32-base/startup/)


set(ALL_PROJECT_SOURCES
        ${SOURCE_LIST}
        ${STARTUP_BASE_DIR}/${SERIES_FOLDER}/${MAPPED_DEVICE}.s
        System/System.h System/System.c
        exti/exti.c exti/exti.h exti/exti_private.h
        app/App.h app/App.c
        Led/Led.h Led/Led.c
        Utils/Utils.c Utils/Utils.h
        Usart/Usart.h Usart/Usart.c
        Lcd/Lcd.c Lcd/Lcd.h
        ADC/Adc.c ADC/Adc.h ADC/Adc_private.h
        Nvic/Nvic.h Nvic/Nvic_private.h Nvic/Nvic.c
        lm35/lm35.c lm35/lm35.h
        Timer/Timer.c Timer/Timer.h Timer/Timer_Private.h
        PWM/PWM.c PWM/PWM.h
        Fan/Fan.h Fan/Fan.c
        Button/Button.c Button/Button.h
        Motor/Motor.c
        Motor/Motor.h
)

set(MASTER_EXE "Elevator_Master")
set(SLAVE_EXE  "Elevator_Slave")


add_executable(${MASTER_EXE} ${ALL_PROJECT_SOURCES})
target_include_directories(${MASTER_EXE} PUBLIC ${INCLUDE_LIST})

target_compile_definitions(${MASTER_EXE} PUBLIC ${COMPILER_DEFINE} BUILD_AS_MASTER)

target_compile_options(${MASTER_EXE} PUBLIC ${CPU_PARAMETERS} -Wall -Wpedantic -Wno-unused-parameter)
target_link_options(${MASTER_EXE} PRIVATE ${CPU_PARAMETERS} -L${LINKER_SCRIPTS_BASE_DIR} -T${LINKER_SCRIPTS_BASE_DIR}/${SERIES_FOLDER}/${DEVICE}.ld -Xlinker --gc-sections -Wl,-Map=${MASTER_EXE}.map --specs=nano.specs)


add_executable(${SLAVE_EXE} ${ALL_PROJECT_SOURCES})
target_include_directories(${SLAVE_EXE} PUBLIC ${INCLUDE_LIST})

target_compile_definitions(${SLAVE_EXE} PUBLIC ${COMPILER_DEFINE})

target_compile_options(${SLAVE_EXE} PUBLIC ${CPU_PARAMETERS} -Wall -Wpedantic -Wno-unused-parameter)
target_link_options(${SLAVE_EXE} PRIVATE ${CPU_PARAMETERS} -L${LINKER_SCRIPTS_BASE_DIR} -T${LINKER_SCRIPTS_BASE_DIR}/${SERIES_FOLDER}/${DEVICE}.ld -Xlinker --gc-sections -Wl,-Map=${SLAVE_EXE}.map --specs=nano.specs)


foreach(EXE ${MASTER_EXE} ${SLAVE_EXE})
    add_custom_command(TARGET ${EXE} POST_BUILD
            COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXE}>
    )
    add_custom_command(TARGET ${EXE} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXE}> ${EXE}.hex
    )
    add_custom_command(TARGET ${EXE} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXE}> ${EXE}.bin
    )
endforeach()