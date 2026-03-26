#include <iostream>
#include <cstdint>
#include <bitset>
#include <math.h>
#include <algorithm>
#include <iomanip>
#include <fstream>
void list_possible(double dt_step);

std::string get_region(uint8_t dtg);

class DeadTimeCalculator {

    private:
    double target_ticks;
    struct DeadTimeResult{
        uint8_t dtg;
        double actual_dt;
        double error;
    };
    
    // Function to calculate the dead time generator (DTG) value based on the desired dead time in nanoseconds
    void calculate_target_ticks()
    {
        target_ticks = desired_dead_time_ns / get_dts_ns();
    }
    public:
    uint32_t timer_clock_freq;  // Timer clock frequency in Hz
    uint32_t clock_division_factor; // Clock division factor (prescaler)
    double desired_dead_time_ns;    // Desired dead time in nanoseconds
    DeadTimeResult best{0 , 0.0 , 1e9};
    
    void SetTimerClockFrequency(uint32_t freq)
    {
        timer_clock_freq = freq;
    }
    
    void SetClockDivisionFactor(uint32_t factor)
    {
        clock_division_factor = factor;
    }   
    double get_dts_ns()
    {
        return ((double)1000000000UL * (double)clock_division_factor) /(double)timer_clock_freq;
    }

    double compute_dead_time(uint8_t dtg_value)
    {
    if (dtg_value <= 0x7F)
        return dtg_value * get_dts_ns();
    else if (dtg_value > 0x7F && dtg_value <= 0xBF)
        return (64 + (dtg_value & 0x3F)) * 2 * get_dts_ns();
    else if (dtg_value > 0xBF && dtg_value <= 0xDF)
        return (32 + (dtg_value & 0x1F)) * 8 * get_dts_ns();
    else
        return (32 + (dtg_value & 0x1F)) * 16 * get_dts_ns();
    }

    void find_best_dtg()
    {
      
        calculate_target_ticks();
        best = {0 , 0.0 , 1e9}; // Reset best result
        auto try_candidate = [&](uint8_t dtg) {
            double actual_dt = compute_dead_time(dtg);
            double error = std::abs(actual_dt - desired_dead_time_ns);
            if (error < best.error) {
                best = {dtg, actual_dt, error};

            }
        };

        //Region 1
        {
        int dtg = (int)round(target_ticks);
        dtg = std::clamp(dtg, 0, 127);
        try_candidate((uint8_t)dtg);
        }
            // Region 2
    {
        int x = (int)round(target_ticks / 2.0 - 64);
        x = std::clamp(x, 0, 63);
        try_candidate(0x80 | x);
    }

    // Region 3
    {
        int x = (int)round(target_ticks / 8.0 - 32);
        x = std::clamp(x, 0, 31);
        try_candidate(0xA0 | x);
    }

    // Region 4
    {
        int x = (int)round(target_ticks / 16.0 - 32);
        x = std::clamp(x, 0, 31);
        try_candidate(0xC0 | x);
    }
}
   double get_resuolution_ns()
    {
        if (best.dtg <= 0x7F) return get_dts_ns();
        else if (best.dtg <= 0x9F) return get_dts_ns() * 2;
        else if (best.dtg <= 0xBF) return get_dts_ns() * 8;
        else return get_dts_ns() * 16;
    }
};

int main() {

 
    DeadTimeCalculator dt_gen;
    MainMenu:
    std::cout<<"Select mode: "<<std::endl;
    std::cout<<"1. Calculate best DTG for desired dead time"<<std::endl;
    std::cout<<"2. Print possible dead time values for all DTG settings"<<std::endl;
    std::cout<<"3. Generate LUT for all DTG values"<<std::endl;
    std::cout<<"0. Exit"<<std::endl;
    std::cout<<"Enter mode number: ";
    int mode;
    std::cin>>mode;

    if(mode == 0)
    {
        goto Exit;
    }
    else if(mode == 1)
    {

    std::cout<<"Enter Timer Clock Frequency (Hz): ";
    std::cin>>dt_gen.timer_clock_freq;
    std::cout<<"Enter Clock Division Factor: ";
    std::cin>>dt_gen.clock_division_factor;
    std::cout<<"Enter Desired Dead Time (ns): ";
    std::cin>>dt_gen.desired_dead_time_ns;

    dt_gen.find_best_dtg();
    
    std::cout<<"\n ===== Dead Time Calculation Result ===== \n";
    std::cout<<"T_dts (ns): "<<dt_gen.get_dts_ns()<<" ns"<<std::endl;
    std::cout<<"DTG Resolution: "<<dt_gen.get_resuolution_ns()<<" ns"<<std::endl;
    std::cout<<"Desired Dead Time: "<<dt_gen.desired_dead_time_ns<<" ns"<<std::endl;
    std::cout<<"Actual Dead Time: "<<dt_gen.best.actual_dt<<" ns"<<std::endl;
    std::cout<<"Error: "<<dt_gen.best.error<<" ns"<<std::endl;
    double error_percent = (dt_gen.best.error / dt_gen.desired_dead_time_ns) * 100.0;
    std::cout<<"Error Percentage: "<<error_percent<<" %"<<std::endl;

    std::cout<<"Neighboring DTG values: "<<std::endl;
    if(dt_gen.best.dtg > 0)
    {
        std::cout<<"DTG = 0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)(dt_gen.best.dtg - 1)<<std::dec<<std::nouppercase
                 <<" -> Dead Time: "<<dt_gen.compute_dead_time(dt_gen.best.dtg - 1)<<" ns"<<std::endl;
    }
    if(dt_gen.best.dtg < 127)
    {
        std::cout<<"DTG = 0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)(dt_gen.best.dtg + 1)<<std::dec<<std::nouppercase
                 <<" -> Dead Time: "<<dt_gen.compute_dead_time(dt_gen.best.dtg + 1)<<" ns"<<std::endl;
    }
    std::cout<<"DTG Register value: 0x"
             << std::uppercase<<std::hex
             <<std::setw(2)<<std::setfill('0')
             <<(int)dt_gen.best.dtg
             <<std::dec<<std::nouppercase<<std::endl;

    std::cout<<"DTG[7:5] bits: "<<get_region(dt_gen.best.dtg)<<std::endl;
    
    std::cout<<"\n ====== STM32 Code ======== \n";
    std::cout<<"TIM1->BDTR = (TIM1->BDTR & ~0xFF) | 0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')
                <<(int)dt_gen.best.dtg
                <<std::dec<<std::nouppercase<<";\n";
    
    }
    else if(mode == 2)
    {
        std::cout<<"Enter Timer Clock Frequency (Hz): ";
        std::cin>>dt_gen.timer_clock_freq;
        std::cout<<"Enter Clock Division Factor: ";
        std::cin>>dt_gen.clock_division_factor;

        std::cout<<"\nPossible Dead Time values for all DTG settings:\n";
        for(int dtg = 0 ; dtg < 256 ; dtg++)
        {
            double dt = dt_gen.compute_dead_time((uint8_t)dtg);
            std::cout<<"DTG = 0x"<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)dtg<<std::dec<<std::nouppercase
                     <<" -> Dead Time: "<<dt<<" ns"<<std::endl;
        }
    }
    else if(mode == 3)
    {
        // Generate LUT for all DTG values
        std::cout<<"Enter Timer Clock Frequency (Hz): ";
        std::cin>>dt_gen.timer_clock_freq;
        std::cout<<"Enter Clock Division Factor: ";
        std::cin>>dt_gen.clock_division_factor;
        std::ofstream lut_file("LUT.h");
        lut_file << "#ifndef LUT_H\n#define LUT_H\n\nconst uint32_t dead_time_lut[256] = {\n";
        for(int dtg = 0 ; dtg < 256 ; dtg++)
        {
            double dt = dt_gen.compute_dead_time((uint8_t)dtg);
            lut_file << "    " << (uint32_t)round(dt);
            // print each 16 values in a new line for better readability
            if((dtg + 1) % 16 == 0) lut_file << "\n";
            if(dtg < 255) lut_file << ",";
        }
        lut_file << "};\n\n#endif // LUT_H\n";
        lut_file.close();
    }

    // go back to main menu
    std::cout<<"\nReturning to main menu...\n"<<std::endl;
    goto MainMenu;

    Exit:
    return 0;
}



std::string get_region(uint8_t dtg)
{
    if (dtg <= 0x7F) return "0xx";
    else if (dtg <= 0x9F) return "10x";
    else if (dtg <= 0xBF) return "110";
    else return "111";
}