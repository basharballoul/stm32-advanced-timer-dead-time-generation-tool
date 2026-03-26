
## Overview 
This is a C++ command-line tool for calculating and analyzing dead-time (DTG) values for STM32 advanced timers (e.g. TIM1/TIM8).

It helps you:

- Find the best DTG value for a desired dead time (in ns)
- Understand hardware limitations and resolution
- Explore all possible DTG → dead-time mappings
- Generate a Lookup Table (LUT) for embedded use
- Output ready-to-use STM32 register configuration code
- --
## How it Work
STM32 advanced timers use a **nonlinear dead-time encoding** via the BDTR register.
|DTG Range|Encoding Behavior|


| DTG Range | Encoding Behavior                    |
| --------- | ------------------------------------ |
| 0x00–0x7F | Linear (fine resolution)             |
| 0x80–0x9F | Medium resolution (×2)               |
| 0xA0–0xBF | Coarse resolution (×8)               |
| 0xC0–0xFF | Very coarse (×16, duplicated values) |

The tool:
1. Converts desired dead time → timer ticks
2. Evaluates all encoding regions
3. Selects the closest valid DTG value
4. Reports error and resolution

---
## Usage
When you run the program, you will see:

``` cmd
Select mode:
1.Calculate best DTG for desired dead time
2.Print possible dead time values for all DTG settings
3.Generate LUT for all DTG values
0.Exit
```

### Mode 1 — Best DTG Calculation

##### Input:

- Timer clock frequency (Hz)
- Clock division factor
- Desired dead time (ns)

##### Example:

```
Enter Timer Clock Frequency (Hz): 8000000
Enter Clock Division Factor: 1
Enter Desired Dead Time (ns): 100000
```

##### Output:

```
===== Dead Time Calculation Result =====
T_dts (ns): 125 ns
DTG Resolution: 2000 ns
Desired Dead Time: 100000 ns
Actual Dead Time: 100000 ns
Error: 0 ns
Error Percentage: 0 %

Neighboring DTG values:
DTG = 0xD1 -> Dead Time: 98000 ns
DTG = 0xD3 -> Dead Time: 102000 ns

DTG Register value: 0xD2
DTG[7:5] bits: 111
```

##### STM32 Code Output:

```
TIM1->BDTR = (TIM1->BDTR & ~0xFF) | 0xD2;
```

---
### Mode 2 — List All DTG Values
Prints all 256 DTG values and their corresponding dead times:
```
DTG = 0x00 -> Dead Time: 0 ns
DTG = 0x01 -> Dead Time: 125 ns
...
DTG = 0xFF -> Dead Time: 126000 ns
```
### Notes:

- Dead time is **nonlinear**
- Higher values have **lower resolution**
- Some values (especially in 0xE0–0xFF) are **duplicates**

---
### Mode 3 — Generate LUT
Generates a header file:
##### Output format:

```
#ifndef LUT_H
#define LUT_H

const uint32_t dead_time_lut[256] = {
    0, 125, 250, ..., 126000
};

#endif
```

##### Using LUT in Stm32

Include in your firmware:

```
#include "LUT.h"
```

Example usage:

```C
uint32_t desired_ns = 100000;
uint8_t best_dtg = 0;
uint32_t best_error = 0xFFFFFFFF;
for (int i = 0; i < 256; i++) { 

	uint32_t dt = dead_time_lut[i];
	uint32_t err = (dt > desired_ns) ? (dt - desired_ns) : (desired_ns - dt);
	if (err < best_error) {
	 best_error = err;
	  best_dtg = i;
	}
}
```
or use binary search for faster lookup since the LUT is sorted in ascending order:
```C
uint8_t find_dtg(uint32_t desired_ns)
{
    int left = 0;
    int right = 255;
    uint8_t best_dtg = 0;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        uint32_t dt = dt_lut[mid];

        if (dt == desired_ns)
        {
            return mid; // Exact match found
        }
        else if (dt < desired_ns)
        {
            best_dtg = mid; // Keep track of the best match so far
            left = mid + 1; // Search in the right half
        }
        else
        {
            right = mid - 1; // Search in the left half
        }
    }

    return best_dtg; // Return the closest DTG value found
}
```

To apply dead time:

```C
TIM1->BDTR = (TIM1->BDTR & ~0xFF) | (dtg_value & 0xFF);
```

Recommended function:

```
void TIM1_SetDeadTime(uint8_t dtg)
{
    uint32_t bdtr = TIM1->BDTR;
    bdtr &= ~0xFF;
    bdtr |= (dtg & 0xFF);
    TIM1->BDTR = bdtr;
}
```

- Dead time is **quantized**, exact value may not always be achievable
- Resolution decreases at higher dead times
- LUT must be regenerated if:
    - Timer clock changes
    - Clock division changes