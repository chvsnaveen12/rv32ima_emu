#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "blk.h"

uint8_t* disc;
uint64_t disc_size;
uint8_t* ram_ptr;

bool interrupt;
bool init = false;

uint16_t old_idx;

struct virtq_desc{
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[1024];
}__attribute__((packed));

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
}__attribute__((packed));

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[1024];
}__attribute__((packed));

struct virtio_blk{
    uint32_t status;
    uint32_t device_features_sel;
    uint32_t queue_sel;
    uint32_t queue_num;
    uint32_t queue_ready;
    uint32_t queue_notify;

    uint32_t queue_desc_low;
    uint32_t queue_avail_low;
    uint32_t queue_used_low;

    // Internal
    struct virtq_desc* desc_ptr;
    struct virtq_avail* avail_ptr;
    struct virtq_used* used_ptr;
} blk;

struct virtio_blk_config {
    uint64_t capacity;
    uint32_t size_max;
    uint32_t seg_max;
    struct virtio_blk_geometry {
        uint16_t cylinders;
        uint8_t heads;
        uint8_t sectors;
    } geometry;
    uint32_t blk_size;
    struct virtio_blk_topology {
        uint8_t physical_block_exp;
        uint8_t alignment_offset;
        uint16_t min_io_size;
        uint32_t opt_io_size;
    } topology;
    uint8_t writeback;
} __attribute__((packed)) blk_config;


void blk_process_req(){
    uint32_t dummy;
    uint32_t desc_addr[3], desc_len[3];
    uint16_t desc_flags[3], desc_next[3];

    uint32_t header_type, header_sector;

    uint16_t used_flags, used_idx, used_elem_id, used_elem_len;

    uint16_t avail_flags, avail_idx, avail_ring_idx;

    soc_bus_access_func((uint32_t)&blk.avail_ptr->idx, machine, bus_read, &dummy, 4);
    avail_idx = (uint16_t)dummy;

    if(avail_idx == old_idx)
        return;

    avail_idx = old_idx++;

    soc_bus_access_func((uint32_t)&blk.avail_ptr->ring[avail_idx], machine, bus_read, &dummy, 4);
    avail_ring_idx = (uint16_t)dummy;

    soc_bus_access_func((uint32_t)&blk.avail_ptr->flags, machine, bus_read, &dummy, 4);
    avail_flags = (uint16_t)dummy;

    // printf("avail_idx: %u\n", avail_idx);
    // printf("avail_ring_idx: %u\n", avail_ring_idx);
    // printf("avail_flags: %u\n", avail_flags);

    uint16_t temp_idx = avail_ring_idx;
    for(int i = 0; i < 3; i++){
        soc_bus_access_func((uint32_t)&blk.desc_ptr[temp_idx].addr, machine, bus_read, &dummy, 4);
        desc_addr[i] = (uint32_t)dummy;

        soc_bus_access_func((uint32_t)&blk.desc_ptr[temp_idx].len, machine, bus_read, &dummy, 4);
        desc_len[i] = (uint32_t)dummy;

        soc_bus_access_func((uint32_t)&blk.desc_ptr[temp_idx].flags, machine, bus_read, &dummy, 4);
        desc_flags[i] = (uint16_t)dummy;

        soc_bus_access_func((uint32_t)&blk.desc_ptr[temp_idx].next, machine, bus_read, &dummy, 4);
        desc_next[i] = (uint16_t)dummy;

        temp_idx = desc_next[i];

        // printf("desc_addr[%u]: 0x%08x\n", i, desc_addr[i]);
        // printf("desc_len[%u]: %u\n", i, desc_len[i]);
        // printf("desc_flags[%u]: 0x%08x\n", i, desc_flags[i]);
        // printf("desc_next[%u]: %u\n", i, desc_next[i]);
    }


    soc_bus_access_func(desc_addr[0], machine, bus_read, &dummy, 4);
    header_type = (uint32_t)dummy;

    soc_bus_access_func(desc_addr[0] + 8, machine, bus_read, &dummy, 4);
    header_sector = (uint32_t)dummy;

    // printf("header_type: %u\n", header_type);
    // printf("header_sector: 0x%08x\n", header_sector);

    if(desc_flags[0] != 1 || (desc_flags[1] != 3 && desc_flags[1] != 1) || desc_flags[2] != 2){
        printf("virtio-blk chain error\n");
        exit(-1);
    }
    if(desc_len[0] != 16 || desc_len[2] != 1){
        printf("virtio-blk chain len error\n");
        exit(-1);
    }

    if(header_type != 0 && header_type != 1){
        printf("IOerror\n");
        exit(-1);
    }
    
    if(desc_flags[1] & 2)
        memcpy(&ram_ptr[desc_addr[1] - 0x80000000], &disc[header_sector << 9], desc_len[1]);
    else
        memcpy(&disc[header_sector << 9], &ram_ptr[desc_addr[1] - 0x80000000], desc_len[1]);

    ram_ptr[desc_addr[0] - 0x80000000] = 0;

    soc_bus_access_func((uint32_t)&blk.used_ptr->idx, machine, bus_read, &dummy, 4);
    used_idx = (uint16_t)dummy;

    soc_bus_access_func((uint32_t)&blk.used_ptr->ring[used_idx % blk.queue_num].id, machine, bus_write, &avail_ring_idx, 2);
    dummy = 0;
    soc_bus_access_func((uint32_t)&blk.used_ptr->ring[used_idx % blk.queue_num].id + 2, machine, bus_write, &dummy, 2);
    
    dummy = desc_len[1];
    soc_bus_access_func((uint32_t)&blk.used_ptr->ring[used_idx % blk.queue_num].len, machine, bus_write, &dummy, 4);

    soc_bus_access_func((uint32_t)&blk.used_ptr->flags, machine, bus_read, &dummy, 4);
    used_flags = (uint16_t)dummy;

    used_idx++;
    soc_bus_access_func((uint32_t)&blk.used_ptr->idx, machine, bus_write, &used_idx, 2);
    // printf("used_idx: 0x%08x\n", used_idx);

    if(avail_flags == 1)
        return;
    
    interrupt = true;

    // blk.used_ptr[]
    // soc_bus_access_func(blk., machine, bus_read, &dummy, 4);

}


bool blk_bus_access_func(uint32_t addr, priv_level priv, bus_access access, uint32_t* val, uint8_t len){
    uint32_t dummy;
    // sleep(1);
    if(len != 4){
        printf("BLK addr misaligned or length != 4, dying uwu\n");
        core_dump();
        exit(-1);

    }
    // usleep(100000);

    if(access == bus_write){
        switch (addr){
            case RW_DEVICE_STATUS_ADDR:
                blk.status = *val;
                return true;
            
            case W_DEVICE_FEATURES_SEL_ADDR:
                blk.device_features_sel = *val;
                return true;

            case W_DRIVER_FEATURES_SEL_ADDR:
                return true;

            case W_DRIVER_FEATURES_ADDR:
                return true;

            case W_QUEUE_SEL_ADDR:
                blk.queue_sel = *val;
                return true; 

            case W_QUEUE_NUM_ADDR:
                blk.queue_num = *val;
                return true;
            
            case W_QUEUE_DESC_LOW_ADDR:
                blk.queue_desc_low = *val;
                blk.desc_ptr = (struct virtq_desc*)*val;
                return true;

            case W_QUEUE_DESC_HIGH_ADDR:
                return true;

            case W_QUEUE_AVAIL_LOW_ADDR:
                blk.queue_avail_low = *val;
                blk.avail_ptr = (struct virtq_avail*)*val;
                return true;

            case W_QUEUE_AVAIL_HIGH_ADDR:
                return true;

            case W_QUEUE_USED_LOW_ADDR:
                blk.queue_used_low = *val;
                blk.used_ptr = (struct virtq_used*)*val;
                return true;

            case W_QUEUE_USED_HIGH_ADDR:
                return true;
            
            case W_QUEUE_NOTIFY_ADDR:
                blk.queue_notify = *val;
                blk_process_req();
                init = true;
                return true;

            case W_INTERRUPT_ACK_ADDR:
                if(*val == 1)
                    interrupt = false;
                // blk_process_req();
                return true;
            
            case RW_QUEUE_READY_ADDR:
                blk.queue_ready = *val;
                return true;

            default:
                return false;
        }
    }
    else{
        switch(addr){
            case R_MAGIC_ADDR:
                *val = BLK_MAGIC;
                return true;
            case R_DEVICE_VERSION_ADDR:
                *val = DEVICE_VERSION;
                return true;
            case R_DEVICE_ID_ADDR:
                *val = DEVICE_ID;
                return true;
            case R_VENDOR_ID_ADDR:
                *val = VENDOR_ID;
                return true;
            
            case R_DEVICE_FEATURES_ADDR:
                if(blk.device_features_sel == 1)
                    *val = 1;
                else
                    *val = 0;
                return true;
            
            case RW_QUEUE_READY_ADDR:
                *val = blk.queue_ready;
                return true;

            case R_QUEUE_NUM_MAX_ADDR:
                *val = 1024;
                return true;

            case R_CONFIG_GENERATION_ADDR:
                *val = 0;
                return true;

            case R_INTERRUPT_STATUS_ADDR:
                *val = 1;
                return true;

            case 0x100:
                *val = (uint32_t)blk_config.capacity;
                return true;
            
            case 0x104:
                *val = 0;
                return true;

            case RW_DEVICE_STATUS_ADDR:
                *val = blk.status;
                return true;

            default:
                return false;
        }
    }
}

void blk_exit(){
    munmap(disc, disc_size);
}

void blk_init(char* disk_img){
    blk.status = 0;
    blk.queue_ready = 0;
    blk_config.capacity = 1000;

    int fd = open(disk_img, O_RDWR);
    if(fd < 0){
        printf("Unable to open file\n");
        exit(-1);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        exit(-1);
    }

    uint64_t disk_size = st.st_size;
    disc = (uint8_t*)mmap(NULL, disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(disc == (void*)(-1)){
        printf("Unable to create map\n");
        exit(-1);
    }

    blk_config.capacity = ((disk_size - 1) / 512) + 1;
    atexit(blk_exit);

    ram_ptr = memory_get_ram_ptr();
    close(fd);
}

bool blk_int(){
    if(init)
        blk_process_req();
    bool ret_val = interrupt;
    // interrupt = false;
    return ret_val;
}