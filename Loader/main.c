// Library Includes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

// Globals

#define GAME_NAME "DkkStm.exe"
#define BUILD_VER "DKCedit: Ver 1.0 .%04d.%02d/%02d %s"
#define BUILD_DATE "Jan 04 2024"
#define BUILD_TIME "19:33:06"

#define OLD_DATE_OFFSET 0x4B47D8
#define OLD_TIME_OFFSET 0x4B47F8
#define OLD_STRING_OFFSET 0x279C7
#define NEW_STRING_OFFSET 0x00ADCA35

#define MOD_SIZE_ADDR 0x5DEA24

#define MAX_STRING 255
#define SECTION_OFFSET 0x15E
#define SECTION_HEADER_OFFSET 0x378
#define DEFAULT_SECTIONS 7
#define SECTION_NAME 0x74696465636b642e
#define VIRTUAL_OFFSET 0xB05000 //0xAFE000
#define VIRTUAL_SIZE 0x2000
#define NEW_SECTION_SIZE 0x1000
#define RAW_OFFSET 0x5DEA00 //0x5D6E00
#define CHARACTERISTICS_MAGIC 0xE0000040
#define IMAGE_SIZE 0xB07000 //0xB00000
#define IMAGE_OFFSET 0x1A8

#define DEFAULT_MOD_START 0x5DEA28
#define DEFAULT_MOD_START_VIRT 0xB05028

// Function Prototypes

void load_mod(FILE* game);
void mod_bootloader(FILE* game);

// Main

void main(int argc, char** argv){
    // print logo
    // looks awesome, btw
    printf("                    ___           ___           ___                                             \n");
    printf("     _____         /|  |         /\\__\\         /\\__\\         _____                              \n");
    printf("    /::\\  \\       |:|  |        /:/  /        /:/ _/_       /::\\  \\       ___           ___     \n");
    printf("   /:/\\:\\  \\      |:|  |       /:/  /        /:/ /\\__\\     /:/\\:\\  \\     /\\__\\         /\\__\\    \n");
    printf("  /:/  \\:\\__\\   __|:|  |      /:/  /  ___   /:/ /:/ _/_   /:/  \\:\\__\\   /:/__/        /:/  /    \n");
    printf(" /:/__/ \\:|__| /\\ |:|__|____ /:/__/  /\\__\\ /:/_/:/ /\\__\\ /:/__/ \\:|__| /::\\  \\       /:/__/     \n");
    printf(" \\:\\  \\ /:/  / \\:\\/:::::/__/ \\:\\  \\ /:/  / \\:\\/:/ /:/  / \\:\\  \\ /:/  / \\/\\:\\  \\__   /::\\  \\     \n");
    printf("  \\:\\  /:/  /   \\::/~~/~      \\:\\  /:/  /   \\::/_/:/  /   \\:\\  /:/  /   ~~\\:\\/\\__\\ /:/\\:\\  \\    \n");
    printf("   \\:\\/:/  /     \\:\\~~\\        \\:\\/:/  /     \\:\\/:/  /     \\:\\/:/  /       \\::/  / \\/__\\:\\  \\   \n");
    printf("    \\::/  /       \\:\\__\\        \\::/  /       \\::/  /       \\::/  /        /:/  /       \\:\\__\\  \n");
    printf("     \\/__/         \\/__/         \\/__/         \\/__/         \\/__/         \\/__/         \\/__/  \n");

    // get the exe path from the args
    char input[MAX_STRING];
    strcpy(input, argv[1]);
    strcat(input, "\\");
    strcat(input, GAME_NAME);

    // open game file
    FILE *game;
    // if the file does not open, assume name is invalid and exit
    if (game = fopen(input, "rb+")){
    } else {
        printf("Invalid filename\n");
        exit(-1);
    }

    // go to the location of the short that says the section count
    fseek(game, SECTION_OFFSET, SEEK_SET);
    short sections;
    fread(&sections, 2, 1, game);
    printf("Sections found = %d\n", sections);

    // if the section count is equal to the default, run the bootloader,
    // else, assume the bootloader was already applied and start installing mods
    if(sections == DEFAULT_SECTIONS){
        printf("Default amount of sections in PE found\n");
        mod_bootloader(game);

        // once you run the bootloader, start installing mods
        goto start_mod_loop;
    }
    else {
        // loop over each mod and attempt to apply it
    start_mod_loop:;
        // Setup
        // every path after the default and the exe is to a mod folder containing the mod.bin, functions.txt, and variables.txt
        int mods_left = argc - 2;
        int curr_id = 2;

        // track the current working directory so we can return here later
        char old_path[256];
        getcwd(old_path, 256);

        // Main loop
    mod_loop:; //C syntax (╯°□°)╯︵ ┻━┻
        // exit condition: no mods left to install
        if (mods_left <= 0)
            goto finish;

        // read the folder path
        strcpy(input, argv[curr_id]);
        // if the folder does not exist, report the error and continue the loop, ignoring the invalid mod
        if (chdir(input) != 0) {
            printf("Invalid path. Include the full path\n");
            mods_left -= 1;
            curr_id += 1;
            goto mod_loop;
        }

        // apply the mod to the game
        load_mod(game);

        // return to original working directory
        chdir(old_path);

        // if we are out of mods, exit loop
        if (mods_left <= 0) {
            goto finish;
        // otherwise, check the next mod
        } else {
            mods_left -= 1;
            curr_id += 1;
            goto mod_loop;
        }
    }
    // execution is complete, stop the installer
finish:
    fclose(game);
    // most important line. DO NOT REMOVE
    printf(":3\n");
}

// Functions

// adds the DKCedit section to the EXE
// this section is where all the mod code gets added to
// also update the build info on the title screen
void mod_bootloader(FILE* game) {
    // increment the section count
    printf("You chose the bootloader. Writing new section count\n");
    fseek(game, SECTION_OFFSET, SEEK_SET);
    short new_sections = DEFAULT_SECTIONS + 1;
    fwrite(&new_sections, 2, 1, game);
    
    // create the entry in the section table for the new section
    // this includes: 
    // name (dkcedit), 
    // virtual size (size of section in memory; can be arbitrarily large),
    // virtual address (an arbitrary location to place the section in memory),
    // size of raw data (size of the section in the actual file; can be as large as the virtual size),
    // pointer to raw data (location in actual file of the section data; should be placed at the end of the game file),
    // THREE WORDS OF NOTHING IMPORTANT
    // characteristic (set the section to be readable, writable, executable, and as initialized data; idk why, but its what purrygamer made it be, and it works)
    printf("Section Count is now %d. Moving on to creating section name.\n", new_sections);
    fseek(game, SECTION_HEADER_OFFSET, SEEK_SET);
    // name
    unsigned long long name = SECTION_NAME;
    fwrite(&name, 8, 1, game);
    // virtual size
    unsigned int sec_field = VIRTUAL_SIZE;
    fwrite(&sec_field, 4, 1, game);
    // virtual address
    sec_field = VIRTUAL_OFFSET;
    fwrite(&sec_field, 4, 1, game);
    // size of raw data
    sec_field = NEW_SECTION_SIZE;
    fwrite(&sec_field, 4, 1, game);
    // pointer to raw data
    sec_field = RAW_OFFSET;
    fwrite(&sec_field, 4, 1, game);
    // bunch of nothing
    sec_field = 0;
    fwrite(&sec_field, 4, 1, game);
    fwrite(&sec_field, 4, 1, game);
    fwrite(&sec_field, 4, 1, game);
    // characteristics
    sec_field = CHARACTERISTICS_MAGIC;
    fwrite(&sec_field, 4, 1, game);

    // update image size
    // idk why, but its what do
    printf("Section header added, now adding new Image size\n");
    fseek(game, IMAGE_OFFSET, SEEK_SET);
    sec_field = IMAGE_SIZE;
    fwrite(&sec_field, 4, 1, game);

    // write in build information
    // this would ideally appear on the title screen
    char empty = 0;
    char debug_info[] = BUILD_VER;
    fseek(game, 0, SEEK_END);
    fwrite(debug_info, sizeof(debug_info), 1, game);
    for (int i = 0; i < NEW_SECTION_SIZE - sizeof(debug_info); i++) {
        fwrite(&empty, 1, 1, game);
    }
    fseek(game, OLD_DATE_OFFSET, SEEK_SET);
    char new_date_info[] = BUILD_DATE;
    fwrite(new_date_info, sizeof(new_date_info), 1, game);
    fseek(game, OLD_TIME_OFFSET, SEEK_SET);
    char new_time_info[] = BUILD_TIME;
    fwrite(new_time_info, sizeof(new_time_info), 1, game);
    fseek(game, OLD_STRING_OFFSET, SEEK_SET);
    uint32_t new_jump = NEW_STRING_OFFSET;
    fwrite(&new_jump, sizeof(uint32_t), 1, game);
}

// check if 4 byte array is equal to one of our special nop register conditions
uint8_t check_nop_rax(uint8_t buffer[4]) {
    return buffer[0] == 0x48 && buffer[1] == 0x0F && buffer[2] == 0x1F && buffer[3] == 0xC0;
}
uint8_t check_nop_rbx(uint8_t buffer[4]) {
    return buffer[0] == 0x48 && buffer[1] == 0x0F && buffer[2] == 0x1F && buffer[3] == 0xC3;
}
uint8_t check_nop_rcx(uint8_t buffer[4]) {
    return buffer[0] == 0x48 && buffer[1] == 0x0F && buffer[2] == 0x1F && buffer[3] == 0xC1;
}

// move buffer used to find nop-reg instructions forward by one byte
void shuffle_nop_finder(uint8_t buffer[4], uint8_t new_val) {
    buffer[0] = buffer[1];
    buffer[1] = buffer[2];
    buffer[2] = buffer[3];
    buffer[3] = new_val;
}

// use the nop-reg conditions and our input files to adjust the mod binary, and inject it into the game
void load_mod(FILE* game) {
    // Setup
    FILE* variables;
    FILE* functions;
    FILE* mod;
    uint8_t cur_byte;
    uint32_t new_code_start;
    uint32_t cur_instruction_addr;
    uint32_t file_address;

    // input validation
    // check if all of our mod files exist
    printf("Checking if mod files exist... ");
    if (variables = fopen("variables.txt", "r")){
    } else{
        printf("Missing or corrupted variables.txt file\n");
        return;
    }
    if (functions = fopen("functions.txt", "r")){
    } else{
        printf("Missing or corrupted functions.txt file\n");
        return;
    }
    if (mod = fopen("mod.bin", "rb+")){
    } else{
        printf("Missing or corrupted mod.bin file\n");
        return;
    }
    printf("ok\n");

    // place current mod after the prior mod, or at the start of the section
    printf("Looking for spot in dkkstm.exe to place code...");
    fseek(game, MOD_SIZE_ADDR, SEEK_SET);
    // we track the space used as we write in the mod's bytes to the game
    uint32_t space_used;
    fread(&space_used, sizeof(uint32_t), 1, game);
    printf("phys: %x | virt %x\n", DEFAULT_MOD_START + space_used, DEFAULT_MOD_START_VIRT + space_used);
    // track the start of the current mod section
    file_address = DEFAULT_MOD_START + space_used;
    cur_instruction_addr = DEFAULT_MOD_START_VIRT + space_used;
    new_code_start = DEFAULT_MOD_START_VIRT + space_used;

    // setup main loop
    uint8_t nop_finder[4];
    printf("Writing mod code...");
    fseek(game, DEFAULT_MOD_START + space_used, SEEK_SET);

    // main write loop
    while(fread(&cur_byte, 1, 1, mod) == 1) {
        // increment the nop-reg finder
        shuffle_nop_finder(nop_finder, cur_byte);

        // check if we have a nop-reg instruction at the current byte
        // if have a nop-rax, then we write in the original callsite of the function we replaced
        // or, if we middle injected, write in 5 nops
        if(check_nop_rax(nop_finder)) {
            //Don't forget the last nop byte
            fwrite(&cur_byte, 1, 1, game);
            space_used++;
            cur_instruction_addr++;
            file_address++;

            // if the call site isnt in the file, just write in nops
            char callsite_found = 1;
            char call_virt[255];
            if (fgets(call_virt, 255, functions) == NULL) {
                printf("Original function address not found. Call offset was not generated\n");
                callsite_found = 0;
            }

            //Put in call (0xE8) byte
            fread(&cur_byte, 1, 1, mod);
            // write nop if no callsite
            if (callsite_found == 0)
                cur_byte = 0x90;
            fwrite(&cur_byte, 1, 1, game);
            space_used++;
            cur_instruction_addr++;
            file_address++;

            // get the original callsite address
            // our call offset will be 4 bytes after the location
            char *end;
            uint32_t target_call = (uint32_t)strtoul(call_virt, &end, 16);
            if (*end != '\0' && *end != '\n') {
                printf("Invalid formatting in functions file. Mod failed to install (space was still used)\n");
                goto mod_end;
            }
            int call_offset = target_call - (cur_instruction_addr + 4);

            // write nops if no callsite was in the file
            if (callsite_found == 0)
                call_offset = 0x90909090;
            fwrite(&call_offset, sizeof(int), 1, game);

            // discard the 4 placeholder bytes of the mod file
            int discard;
            fread(&discard, sizeof(int), 1, mod);

            // inform user if callsite was made
            if (callsite_found == 1)
                printf("Call offset generated: %x\n", call_offset);

            // update the used space
            space_used += 4;
            cur_instruction_addr += 4;
            file_address += 4;
        
        // if we have a nop-rbx, write in the address of the variable we want to pull from memory
        } else if (check_nop_rbx(nop_finder)) {
            //Don't forget the last nop byte
            fwrite(&cur_byte, 1, 1, game);
            space_used++;
            cur_instruction_addr++;
            file_address++;

            //First three bytes of mov instruction
            fread(&cur_byte, 1, 1, mod);
            fwrite(&cur_byte, 1, 1, game);
            space_used++;
            cur_instruction_addr++;
            file_address++;
            fread(&cur_byte, 1, 1, mod);
            fwrite(&cur_byte, 1, 1, game);
            space_used++;
            cur_instruction_addr++;
            file_address++;
            fread(&cur_byte, 1, 1, mod);
            fwrite(&cur_byte, 1, 1, game);
            space_used++;
            cur_instruction_addr++;
            file_address++;

            // read variable address from variable.txt
            char var_virt[255];
            if(fgets(var_virt, 255, variables) == NULL){
                printf("Incomplete (or corrupted) variables file. Mod failed to install (space was still used)\n");
                goto mod_end;
            }
            char *end;
            uint32_t target_var = (uint32_t)strtoul(var_virt, &end, 16);
            if (*end != '\0' && *end != '\n') {
                printf("Invalid formatting in variables file. Mod failed to install (space was still used)\n");
                goto mod_end;
            }
            int var_offset = target_var - (cur_instruction_addr + 4);

            // write address to the game
            fwrite(&var_offset, sizeof(int), 1, game);

            // discard the 4 placeholder bytes
            int discard;
            fread(&discard, sizeof(int), 1, mod);

            // increment the mod size params
            printf("Variable offset generated: %x\n", var_offset);
            space_used += 4;
            cur_instruction_addr += 4;
            file_address += 4;

        // if we have a nop-rcx, overwrite the designated callsite with our own new mod function
        } else if(check_nop_rcx(nop_finder)) {
            // finish out nop
            fwrite(&cur_byte, 1, 1, game);
            space_used++;
            cur_instruction_addr++;
            file_address++;

            // get the virtual and physical offsets for the function
            printf("Replacing old call pointer...");
            char physical_buffer[255];
            char virtual_buffer[255];
            if(fgets(physical_buffer, 255, functions) == NULL){
                printf("Incomplete (or corrupted) functions file. Mod failed to install (space was still used)\n");
                goto mod_end;
            }
            if(fgets(virtual_buffer, 255, functions) == NULL){
                printf("Incomplete (or corrupted) functions file. Mod failed to install (space was still used)\n");
                goto mod_end;
            }
            char *endptr;
            uint32_t target_address = (uint32_t)strtoul(virtual_buffer, &endptr, 16);
            if (*endptr != '\0' && *endptr != '\n') {
                printf("Invalid formatting in functions file. Mod failed to install (space was still used)\n");
                goto mod_end;
            }
            uint32_t physical_address = (uint32_t)strtoul(physical_buffer, &endptr, 16);
            if (*endptr != '\0' && *endptr != '\n') {
                printf("Invalid formatting in functions file. Mod failed to install (space was still used)\n");
                goto mod_end;
            }

            // at the physical address we want to replace, write in the virtual offset to the new mod code
            fseek(game, physical_address, SEEK_SET);
            int offset = cur_instruction_addr - target_address;
            fwrite(&offset, sizeof(int), 1, game);
            printf("%x\n", offset);
            fseek(game, file_address, SEEK_SET);

        // otherwise, just write the byte in unchanged
        } else {
            fwrite(&cur_byte, 1, 1, game);
            space_used++;
            cur_instruction_addr++;
            file_address++;
        }
    }
    printf("ok\n");

    // finished injecting the given mod
    mod_end:
    printf("Mod injected, closing files\n");
    // write the total amount of used data to the mod size address
    fseek(game, MOD_SIZE_ADDR, SEEK_SET);
    fwrite(&space_used, sizeof(uint32_t), 1, game);

    // close the mods files
    fclose(mod);
    fclose(functions);
    fclose(variables);
    return;
}