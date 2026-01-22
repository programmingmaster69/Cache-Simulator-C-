#include <iostream>
#include <string>
#include <vector>
#include <iomanip> 
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace std;
int cache_type, block_size, cache_size,number_of_blocks=0;;

int compulsory_misses=0, capacity_misses=0, conflict_misses=0;


#define		DRAM_SIZE		(64*1024*1024)

unsigned int m_w = 0xABABAB55;    
unsigned int m_z = 0x05080902;   




unsigned int rand_()
{
    m_z = 36969 * (m_z & 65535) + (m_z >> 16); // 65535 -- 16 ones. Extracts lower 16 bits of m_z
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) + m_w;  /* 32-bit result */
}

unsigned int memGen1()
{
    static unsigned int addr=0;
    return (addr++)%(DRAM_SIZE); // Sequential memory progression
}

unsigned int memGen2()
{
    static unsigned int addr=0;
    return  rand_()%(128*1024); // Address anywhere between 0 to 128KB (for capacity testing)
}

unsigned int memGen3()
{
    return rand_()%(DRAM_SIZE); // Pure random thrashing
}

unsigned int memGen4()
{
    static unsigned int addr=0; 
    return (addr++)%(1024); // 0-1 KB, but goes sequentially.
}

unsigned int memGen5()
{
    static unsigned int addr=0;
    return (addr++)%(1024*64); // 0-64 KB, goes sequentially
}

unsigned int memGen6()
{
    static unsigned int addr=0;
    return (addr+=256)%(DRAM_SIZE); // Whole Mem range, but jumps 256 bytes.
}


// Cache Simulator
bool cacheSim(unsigned int addr, int cache[3][100000], int type, int &block_counter, int index_addr, int tag_addr)
{
    int shift_offset=log2(block_size);
    bool detected=false;
    bool misses_flag=true;
    
    if (cache_type==0)    // Direct Mapped Cache
    {
        if (cache[0][index_addr]==tag_addr)
        {
            return true; // Hit
        }
        else //Miss
            {
                cache[0][index_addr]= tag_addr;
                for (int i=0; i < number_of_blocks; i++)
                {
                    if (cache[1][i]!=1)
                    {   misses_flag=false; // It is not a capacity miss.
                        i=number_of_blocks;}
                    
                }
                if (misses_flag)
                    capacity_misses++;   // It is a capacity miss because the cache is full
                else
                {
                    if(cache[1][index_addr]==1)
                        conflict_misses++;
                    else
                    {
                        compulsory_misses++;
                    }
                }
                cache[1][index_addr]= 1;
                return 0;
            }
    } // End of direct mapped cache
    
    else if (cache_type==1)   // Set associative cache
    {
        index_addr=index_addr * type;
        for (int i=0; i < type ; i++)
        {
            if (cache[0][index_addr+i]==tag_addr)
            {
                return 1; // Hit. LRU isn't used, so no variables are tracked
            }
        }
        for (int j=0; j < type; j++) // Loop runs if it is a miss
        {
            if (cache[1][index_addr+j] == -1) // Checks if slot is empty/invalid
            {
                compulsory_misses++;
                cache[0][index_addr+j]=tag_addr;
                cache[1][index_addr+j]=1;
                return 0;
            }
        }
        
        //If no slot is empty, then assign any random slot and evict the previous one.
        int x=rand()%(type);
        cache[0][index_addr+x]=tag_addr;
        cache[1][index_addr+x]=1;
        capacity_misses++;
        return 0;
        
    }//End of Set Associative Cache

    
    
    else if (cache_type==2)        // Fully associative cache
    {
        if (type==0)    // LRU(Least Recently Used) Policy
        {
            if (block_counter < number_of_blocks) // Cache has not been initialized/ is empty
            {
                for (int i=0; i < number_of_blocks; i++)
                {
                    if (cache[0][i]==addr >> shift_offset)
                    {
                        cache[1][i]=block_counter;
                        block_counter--; // Compensates for the block counter++ in the main loop.
                        return true; // Hit
                    }
                }
                    compulsory_misses++;
                    cache[0][block_counter]=addr>>shift_offset;
                    cache[1][block_counter]=block_counter;
                    return false;  // Miss
            }
            else  // block counter is more than block size (cache is fullly initialized)
            {
                // 1. SEARCH FOR HIT
                for (int i = 0; i < number_of_blocks; i++)
                {
                    if (cache[0][i] == (addr >> shift_offset))
                    {
                        cache[1][i] = block_counter; 
                        return true; // Hit
                    }
                }

                // Miss case
                int compare = 0;
                for (int i = 1; i < number_of_blocks; i++) // Finds min recently used block
                {
                    if (cache[1][compare] > cache[1][i])
                        compare = i;
                }
                cache[0][compare] = addr >> shift_offset;
                cache[1][compare] = block_counter;
                capacity_misses++;
                return false; // MISS
            }
        }   // end of LRU Policy
        
        else if (type==1)   // LFU Policy
        {
            if (block_counter < number_of_blocks) // There are some empty blocks in cache
            {
                for (int i=0; i < number_of_blocks; i++)
                {
                    if (cache[0][i]==addr >> shift_offset)
                    {
                        cache[1][i]++;
                        block_counter--;
                        return true; // Hit
                    }
                }
                    cache[0][block_counter]=addr>>shift_offset;
                    cache[1][block_counter]=-1; // Can be set to 0, but set to -1, so its likely to preserve more frequently used data
                    compulsory_misses++;
                    return false;  // Miss
            }
            else  // Block counter is more than block size
            {
                for (int i=0; i < number_of_blocks; i++)
                {
                    if (cache[0][i]==(addr >> shift_offset))
                    {
                        cache[1][i]++;
                        block_counter--;
                        return true; // Hit
                    }
                }
                    int compare2=0;
                    for (int i=1; i < number_of_blocks; i++) // Finds min frequently used block
                    {
                        if (cache[1][compare2] >= cache[1][i])
                            compare2=i;
                    }
                    cache[0][compare2]=addr >> shift_offset;
                    cache[1][compare2]=-1; // Can be set to 0, but set to -1, so its likely to preserve more frequently used data
                    capacity_misses++;
                    return false; // Miss
            }

        }  // end if LFU
        
        else if (type==2) // FIFO
        {
            if (block_counter < number_of_blocks) // There are some empty blocks in cache
            {
                for (int i=0; i < number_of_blocks; i++)
                {
                    if (cache[0][i]==addr >> shift_offset)
                    {
                        block_counter--;
                        return true; // Hit
                    }
                }
                
                    cache[0][block_counter]=addr>>shift_offset;
                    cache[1][block_counter]=block_counter;
                    compulsory_misses++;
                    return false;  // Miss
            }
            else  // block counter is more than block size
            {
                for (int i=0; i < number_of_blocks; i++)
                {
                    if (cache[0][i]==(addr >> shift_offset))
                    {
                        return true; // Hit. Timestamp is not updated here.
                    }
                }
                
                    int compare=0;
                    for (int i=1; i < number_of_blocks; i++)
                    {
                        if (cache[1][compare] > cache[1][i])
                            compare=i;
                    }
                    cache[0][compare]=addr >> shift_offset;
                    cache[1][compare]=block_counter;
                    capacity_misses++;
                    return false; //miss
            }
        }// end of FIFO
        else if (type==3) // Random Policy
        {
            if (block_counter < number_of_blocks) // Some blocks in cache are empty
            {
                for (int i=0; i < number_of_blocks; i++)
                {
                    if (cache[0][i]==addr >> shift_offset)
                    {
                        block_counter--;
                        return true; // Hit
                    }
                }
                
                    cache[0][block_counter]=addr>>shift_offset;
                    compulsory_misses++;
                    return false;  // Miss
                }
            }
            else  // block counter is more than block size
            {
                for (int i=0; i < number_of_blocks; i++)
                {
                    if (cache[0][i]==(addr >> shift_offset))
                    {
                        return true; // Hit
                    }
                }
                    cache[0][rand()%number_of_blocks]=addr >> shift_offset;
                    capacity_misses++;
                    return false; // Miss
            }
        }// End of Random Policy
    }  // End of Fully associative Mapping
    return true;
}

char *msg[2] = {"Miss","Hit"};

int main(int argc, const char * argv[]) {
    srand(time(NULL)); // Provide seed for rand() function.
    
    int looper=1000000, addr, flag, shift;

    cout << "Please enter 0 for Direct mapped, 1 for Set Associative, 2 for Fully Associative: " << endl;
    cin >> cache_type;
    cout << "Please enter the size of the block as a power of 2 between 4 and 128 bytes: " << endl;
    cin >> block_size;
    cout << "Please enter cache size: 1KB – 64KB; in steps that are powers of 2: " << endl;
    cin >> cache_size;
    
    

    int cache[3][100000];
    int block_counter=0;
    int hit_counter=0;
    int index_addr=0, tag_addr=0;
    
    if ( cache_type==0)   //Direct Mapped Cache
    {
        number_of_blocks= (cache_size*1024)/block_size;
        
        for (int i=0; i < 2; i++)   // Setting all the cache with -1 (Empty)
            for (int j=0; j < number_of_blocks; j++)
                cache[i][j]=-1;
        
        for(int i=0; i <looper ;i++)
        {
            addr = memGen1(); // Sequential Memory address generation (Good for spatial locality)
            shift= log2(block_size);
            index_addr= (addr >> shift)% number_of_blocks;
            shift = log2(number_of_blocks * block_size);
            tag_addr= addr >>shift;    // Shifted by the amount of offset and the index bits
            flag = cacheSim(addr, cache, 0,block_counter, index_addr, tag_addr);
            cout <<"0x" << setfill('0') << setw(8) << hex << addr <<" ("<< msg[flag] <<")\n";
            if (flag) 
            {
                hit_counter++;
            }
        }
    cout << "Hits  " << hit_counter<<endl << "Compulsory: " << compulsory_misses <<endl<< "Capacity: " << capacity_misses <<endl<< "Conflict: " << conflict_misses <<  endl;

    }
    else if (cache_type==2)  //  Fully associative Cache
    {
        int replacment_type;
        number_of_blocks= (cache_size*1024)/block_size;
        
        cout << "Please enter the type of replacement policy for the Fully Associative Mapping: LRU->0 , LFU->1, FIFO->2, RANDOM->3: " << endl;
        cin >> replacment_type;
        
        for (int i=0; i < 2; i++)   // Setting all the cache with -1 (Empty)
            for (int j=0; j < number_of_blocks; j++)
                cache[i][j]=-1;
        
        for(int i=0; i <looper ;i++)
        {
            addr = memGen4(); // 0-1 KB sequentially
            flag = cacheSim(addr, cache, replacment_type, block_counter, index_addr, tag_addr);
            cout <<"0x" << setfill('0') << setw(8) << hex << addr <<" ("<< msg[flag] <<")\n";
            if (flag)
            {
                hit_counter++;
            }
            block_counter++;
        }
        
        cout << "Hits: " << hit_counter<<endl << "Compulsory: " << compulsory_misses <<endl<< "Capacity: " << capacity_misses <<endl<< "Conflict: " << conflict_misses <<  endl;
        
    } // End of Fully Associative Mapping

    else if (cache_type==1) // Set Associative Mapping
    {
        int number_of_ways;
        cout << "Please enter the number of ways for the set associative cache(2,4,8,16): " << endl;
        cin >> number_of_ways;
        number_of_blocks= (cache_size*1024)/(block_size*number_of_ways); // Actually no. of sets in case of SAM.
        
        for (int i=0; i < 3; i++)   // setting all the cache with -1
            for (int j=0; j < 100000; j++)
                cache[i][j]=-1;
        
        for(int i=0; i <looper ;i++)
        {
            addr = memGen5(); // 0-64 KB sequentially
            shift= log2(block_size);
            index_addr= (addr >> shift)% (number_of_blocks);
            shift= log2(number_of_blocks+block_size);
            tag_addr= addr >>shift;
            flag = cacheSim(addr, cache, number_of_ways, block_counter, index_addr, tag_addr);
            cout <<"0x" << setfill('0') << setw(8) << hex << addr <<" ("<< msg[flag] <<")\n";
            if (flag)
            {
                hit_counter++;
            }
            block_counter++;
        }
        
        cout << "Hits: " << hit_counter<<endl << "Compulsory: " << compulsory_misses <<endl<< "Capacity: " << capacity_misses <<endl<< "Conflict: " << conflict_misses <<  endl;
    }
  
}