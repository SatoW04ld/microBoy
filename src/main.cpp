#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include "incbin.h"
#include "hardware/timer.h"
#include "pico/stdio.h"
#include "pico/multicore.h"
#include "pico_explorer.hpp"
#include "pico_graphics.hpp"
#include <chrono>
#include <thread>

extern "C" INCBIN(Rom, "gameboy.gb"); 
extern "C" INCBIN(Boot, "dmg_boot.bin"); 

using namespace std::chrono_literals;
using namespace std::chrono;

bool frameDone = false;
unsigned long totalInstructions = 0;

using namespace pimoroni;

static cart* cartridge;
static ppu* PPU;
static bus* Bus;
static cpu* CPU;
static PicoExplorer* Pico;

bool CPU_active = false;

int main()
{     
    stdio_init_all();

    int frames = 0;

    cartridge = new cart( (uint8_t*)gRomData, (uint8_t*) gBootData, gRomSize);
   
    PPU = new ppu();

    Bus = new bus(cartridge, PPU);
    CPU = new cpu(Bus);

    PPU->connectBus(Bus);
    PPU->connectCPU(CPU);
    Bus->connectCPU(CPU);
    
    uint16_t buffer[PicoExplorer::WIDTH * PicoExplorer::HEIGHT];
    Pico = new PicoExplorer(buffer);
    Pico->init();

    PPU->frameBuffer = buffer;

    cartridge->printCart();

    if(!set_sys_clock_khz(270000, true))
    {
        while(true)
        {
            printf("ERROR");
        }
    }
    
    uint64_t totalCPULength = 0;
    uint64_t totalTimerLength = 0;

    while(true)
    {
        uint64_t start = time_us_64();
        while(!PPU->frameDone)
        {             
            if(!(PPU->regs.bytes.LCDC & 0b10000000)) //doesn't tick ppu while not enabled
            {
               uint64_t start = time_us_64();
                CPU->execOP();
                uint64_t stop = time_us_64();
                uint64_t length = stop - start;
               totalCPULength+=length;

                start = time_us_64();
                
                CPU->updateTimers(); 

                stop = time_us_64();
                length = stop - start;
                totalTimerLength +=length;

                CPU->cycles = 0;

            } else 
            {               
                uint64_t start = time_us_64();
                CPU->execOP();
                uint64_t stop = time_us_64();
                uint64_t length = stop - start;
                totalCPULength+=length;


               start = time_us_64();

                CPU->updateTimers();

                stop = time_us_64();
               length = stop - start;
               totalTimerLength +=length;


                PPU->tick();
        
                CPU->cycles = 0;

            }
        
        }    

        frameDone = true;
        
        uint64_t stop = time_us_64();
        
        uint64_t length = (stop - start);

        totalCPULength = 0;
        totalTimerLength = 0;

        start = time_us_64();

        Pico->update();

        stop = time_us_64();
        length = (stop - start);

        printf("Screen: %llu\n", length);

        PPU->frameDone = false;
            
        frames++;    

    }

    return 0;   
}