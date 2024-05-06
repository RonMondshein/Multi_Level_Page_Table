#include "os.h"
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

/*
    input: 
    uint64_t pt as the physical page number of the page table root under the assumption that the page table is returned by alloc_page_frame(),
    uint64_t vpn as the virtual page number the caller wants to map or unmap (VPN, not VA),
    uint64_t ppn as the physical page number the caller wants to map vpn to, or NO_MAPPING if the caller wants to unmap vpn.
    functionality: create a mapping from vpn to ppn in the page table rooted at pt, or unmap vpn if ppn is NO_MAPPING.
*/
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){

    uint64_t vpn_parts[5]; /*VPN as written in the assignment has 5 parts:
    frame - 4KB, PTE - 8B - 512 = 2^9 children
    9 bits per child in 45 bits in the VPN => 45/9=5 parts/ levels
    */
   /*The relevent 9 bits for each part */
    vpn_parts[0] = (vpn >> 36) & 0x1ff;
    vpn_parts[1] = (vpn >> 27) & 0x1ff;
    vpn_parts[2] = (vpn >> 18) & 0x1ff;
    vpn_parts[3] = (vpn >> 9) & 0x1ff;
    vpn_parts[4] = vpn & 0x1ff; /*PPN place*/

    uint64_t* PT_PA[5];
    
    pt = pt<<12; /*PT address*/

    PT_PA[0]  = (uint64_t*)(phys_to_virt(pt)); /*First step- the root PT*/
    if(ppn == NO_MAPPING){
        /*GOAL: the vpn's mapping (if exists) should be destroyed!*/
        for (int i = 0; i < 4; i++)
        {
            if (((PT_PA[i][vpn_parts[i]]) & 1 )== 0 )
            {
                /* There isn't such of vpn to destroyed*/
                return;   
            }
            PT_PA[i + 1] = (uint64_t*)(phys_to_virt(PT_PA[i][vpn_parts[i]] - 1)); /*Next level*/
            if (PT_PA[i + 1] == NULL)
            {
                /*There isn't such of PT address*/
                return;   
            } 
        }
        if (PT_PA[4] != NULL)
        {
            /*Make the PPN unvaliad*/
            PT_PA[4][vpn_parts[4]] = NO_MAPPING;
            return;   
        }
    }
    else
    {
        /*GOAL: the vpn's mapping should be created*/
        for (int i = 0; i < 4; i++)
        {
            if (PT_PA[i] == NULL)
            {
                return;   
            }
            if (((PT_PA[i][vpn_parts[i]])&1) == 0 || PT_PA[i][vpn_parts[i]] == NO_MAPPING)
            { 
                /* There isn't such of vpn, a new one in created*/
                uint64_t temp_frame = alloc_page_frame();
                /*From PPA to PTE */
                PT_PA[i][vpn_parts[i]] = temp_frame << 12;
                /*Valid_bit = 1*/
                PT_PA[i][vpn_parts[i]] = PT_PA[i][vpn_parts[i]] + 1;
            }
            /*else: the address is valiad*/
            PT_PA[i + 1] = (uint64_t*)(phys_to_virt(PT_PA[i][vpn_parts[i]] - 1)); 
        }
        PT_PA[4][vpn_parts[4]] = (ppn << 12)  + 1; /*update leaf, make it PTE and add valiad bit*/
    }
}

/*
    input: 
    uint64_t pt as the physical page number of the page table root under the assumption that the page table is returned by alloc_page_frame(),
    uint64_t vpn as the virtual page number the caller wants to map or unmap,
    output:
    uint64_t ppn that vpn is mapped to, or NO_MAPPING if no mapping exists  
*/
uint64_t page_table_query(uint64_t pt, uint64_t vpn){

    uint64_t vpn_parts[5]; /*VPN as written in the assignment has 5 parts:
    frame - 4KB, PTE - 8B - 512 = 2^9 children
    9 bits per child in 45 bits in the VPN => 45/9=5 parts/ levels
    */
    /*The relevent 9 bits for each part */
    vpn_parts[0] = (vpn >> 36) & 0x1ff;
    vpn_parts[1] = (vpn >> 27) & 0x1ff;
    vpn_parts[2] = (vpn >> 18) & 0x1ff;
    vpn_parts[3] = (vpn >> 9) & 0x1ff;
    vpn_parts[4] = vpn & 0x1ff; /*PPN place*/

    uint64_t* PT_PA[5];

    pt = pt<<12; /*PT address*/
    PT_PA[0]  = (uint64_t*)(phys_to_virt(pt));
    for (int i = 0; i < 4; i++)
    {
            if (PT_PA[i] == NULL)
            {
                /*There isn't such of PT- never should get here */
                return NO_MAPPING;
            }
            if (((PT_PA[i][vpn_parts[i]])&1) == 0)
            {
                /*Valaid bit is off*/
                return NO_MAPPING;
            }
            if ((PT_PA[i][vpn_parts[i]]) == NO_MAPPING)
            {
                 return NO_MAPPING;
            }
            /*else: the address is valiad*/
            PT_PA[i + 1] = (uint64_t*)(phys_to_virt(PT_PA[i][vpn_parts[i]] - 1));
    } 

    if (((PT_PA[4][vpn_parts[4]])&1) == 0 || PT_PA[4][vpn_parts[4]] == NO_MAPPING)
    {
        /*Valaid bit is off or the value equals to NO_MAPPING*/
        return NO_MAPPING;
    }
    /*return the relavent ppn*/
    return PT_PA[4][vpn_parts[4]] >> 12;
}
