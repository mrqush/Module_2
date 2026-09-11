#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "printf/usb_printf.h"

extern "C" void main_cpp() {

    while(1) {
        bool buttonPressed = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_SET;
        if(buttonPressed) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        }

        HAL_Delay(50);
    }
}