# Moving average filter
[![build](https://github.com/pilotak/MovingAverage/workflows/build/badge.svg)](https://github.com/pilotak/MovingAverage/actions) 
[![Framework Badge Arduino](https://img.shields.io/badge/framework-arduino-00979C.svg)](https://arduino.cc)
[![Framework Badge mbed](https://img.shields.io/badge/framework-mbed-008fbe.svg)](https://os.mbed.com/)


**_Buffer_** type can be:
 - `uint8_t` or `int8_t` with maximum buffer length of **16843009**
 - `uint16_t` or `int16_t` with maximum buffer length of **65537**
 - `uint32_t` or `int32_t` but added number can be as long as 30 bits which eaquals to maximum buffer length of **2**, 29 bits number = max buffer length of **4**, etc.

**_Buffer length_**:
- power of 2 only: 2, 4, 8, 16, 32, 64, 128, etc.

## Arduino example
Please see `examples` folder

## Mbed example
```cpp
#include "mbed.h"
#include "MovingAverage.h"

// Buffer (and added samples) will be initialised as uint8_t, total 16 samples
MovingAverage <uint8_t, 16> filter;

int main() {
    printf("result: %u\n", filter.add(255)); // insert new number and get result
    printf("result: %u\n", filter.add(6));   // insert new number and get result
    printf("result: %u\n", filter.add(9));   // insert new number and get result
    printf("result: %u\n", filter.get());    // get last result, without adding a newone

    return 0;
}
```

### Output
> result: 255
> 
> result: 239
> 
> result: 224
> 
> result: 224
