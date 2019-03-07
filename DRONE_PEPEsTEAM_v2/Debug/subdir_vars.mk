################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../drone_pepesteam.cfg 

CMD_SRCS += \
../MSP_EXP432P401R.cmd 

C_SRCS += \
../MSP_EXP432P401R.c \
../main.c \
../mpu9250.c \
../msprf24.c \
../nRF24L01_SPI.c \
../quaternionFilters.c 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt 

GEN_MISC_DIRS += \
./configPkg/ 

C_DEPS += \
./MSP_EXP432P401R.d \
./main.d \
./mpu9250.d \
./msprf24.d \
./nRF24L01_SPI.d \
./quaternionFilters.d 

GEN_OPTS += \
./configPkg/compiler.opt 

OBJS += \
./MSP_EXP432P401R.obj \
./main.obj \
./mpu9250.obj \
./msprf24.obj \
./nRF24L01_SPI.obj \
./quaternionFilters.obj 

GEN_MISC_DIRS__QUOTED += \
"configPkg\" 

OBJS__QUOTED += \
"MSP_EXP432P401R.obj" \
"main.obj" \
"mpu9250.obj" \
"msprf24.obj" \
"nRF24L01_SPI.obj" \
"quaternionFilters.obj" 

C_DEPS__QUOTED += \
"MSP_EXP432P401R.d" \
"main.d" \
"mpu9250.d" \
"msprf24.d" \
"nRF24L01_SPI.d" \
"quaternionFilters.d" 

GEN_FILES__QUOTED += \
"configPkg\linker.cmd" \
"configPkg\compiler.opt" 

C_SRCS__QUOTED += \
"../MSP_EXP432P401R.c" \
"../main.c" \
"../mpu9250.c" \
"../msprf24.c" \
"../nRF24L01_SPI.c" \
"../quaternionFilters.c" 


