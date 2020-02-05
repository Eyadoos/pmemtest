#include <stdint.h>
#include <atomic>
#include <iostream>

enum class NTypes : uint8_t {
        N4 = 0,
        N16 = 1,
        N48 = 2,
        N256 = 3
};

std::atomic<uint64_t> version{0b100};

uint64_t convert(NTypes type){
        return (static_cast<uint64_t>(type) << 62);
}

void getback() {
        std::cout << static_cast<int>(static_cast<NTypes>(version.load(std::memory_order_relaxed) >> 62)) << std::endl;
}

bool isLock(uint64_t version) {
        return (version & 0b10);
}

void lock(uint64_t v1) {
        if(version.compare_exchange_strong(v1, v1 + 0b10)){
                std::cout << "Successful" << std::endl;
        } else{
                std::cout << "Failed" << std::endl;
        }
}

void unlock(){
        std::cout << "UnLocking.. " << std::endl;
        version.fetch_sub(0b10);
}

int main(){

    std::cout << "Default: ";
    std::cout << version << std::endl;

    version.fetch_add(convert(NTypes::N48));
    std::cout << "Before: ";
    std::cout << version << std::endl;
    
    std::cout << "Locking.. ";
    lock(version);
    std::cout << version << std::endl;

    std::cout << "Lock: ";
    std::cout << isLock(version) << std::endl;

    unlock();
    std::cout << "Lock: ";
    std::cout << isLock(version) << std::endl;
    std::cout << version << std::endl;

    std::cout << "After: ";
    getback();

    return 0;
}

