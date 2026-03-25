
/*
in stm32 cude just include the generated LUT.h and use the precomputed dead time values from the LUT for each DTG setting. This will allow you to quickly look up the dead time for any given DTG value without needing to perform calculations at runtime, 
which can be beneficial for performance in time-critical applications.

use this fuction to find the best DTG value for a desired dead time in nanoseconds:

uint8_t find_dtg(uint32_t desired_ns)
{
    uint8_t best_dtg = 0;
    uint32_t best_error = 0xFFFFFFFF;

    for (uint16_t i = 0; i < 256; i++)
    {
        uint32_t dt = dt_lut[i];
        uint32_t err = (dt > desired_ns) ? (dt - desired_ns) : (desired_ns - dt);

        if (err < best_error)
        {
            best_error = err;
            best_dtg = i;
        }
    }

    return best_dtg;
}

or use binary search for faster lookup since the LUT is sorted in ascending order:
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

then set the DTG register in your STM32 code like this:

TIM1->BDTR = (TIM1->BDTR & ~0xFF) | (dtg_value & 0xFF);

*/

