#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt-lowpower)

set(SOURCES src/main.c)

if (CONFIG_PWM)
    list(APPEND SOURCES src/periph_pwm.c)
endif()

if( CONFIG_NRFX_PWM_GRTC )
    list(APPEND SOURCES src/grtc_pwm.c)
endif()

if( CONFIG_WATCHDOG )
    list(APPEND SOURCES src/wdg_conf.c)
endif()

if(CONFIG_NRFX_TIMER)
    list(APPEND SOURCES src/timer_conf.c)
endif(CONFIG_NRFX_TIMER)

target_include_directories(app PRIVATE inc)
target_sources(app PRIVATE ${SOURCES} )
