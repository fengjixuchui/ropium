#include "compiler.hpp"
#include "arch.hpp"
#include "exception.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

using std::cout;
using std::endl; 
using std::string;

namespace test{
    namespace compiler{
        unsigned int _assert_ropchain(ROPChain* ropchain, const string& msg){
            if( ropchain == nullptr){
                cout << "\nFail: " << msg << endl << std::flush; 
                throw test_exception();
            }
            delete ropchain;
            return 1; 
        }
        
        unsigned int _assert_no_ropchain(ROPChain* ropchain, const string& msg){
            if( ropchain != nullptr){
                cout << "\nFail: " << msg << endl << std::flush; 
                throw test_exception();
            }
            return 1; 
        }

        unsigned int direct_match(){
            unsigned int nb = 0;
            ArchX86 arch;
            GadgetDB db;
            ROPCompiler comp = ROPCompiler(&arch, &db);
            ROPChain* ropchain;

            // Available gadgets
            vector<RawGadget> raw;
            raw.push_back(RawGadget(string("\x89\xf9\xbb\x01\x00\x00\x00\xc3", 8), 1)); // mov ecx, edi; mov ebx, 1; ret
            raw.push_back(RawGadget(string("\x89\xC8\xC3", 3), 2)); // mov eax, ecx; ret
            raw.push_back(RawGadget(string("\x89\xC3\xC3", 3), 3)); // mov ebx, eax; ret
            raw.push_back(RawGadget(string("\x01\xf0\x89\xc3\xc3", 5), 4)); // add eax, esp; mov ebx, eax; ret
            raw.push_back(RawGadget(string("\xbb\x04\x00\x00\x00\xc3", 6), 5)); // mov ebx, 4; ret
            raw.push_back(RawGadget(string("\x83\xc0\x04\x89\xc3\xc3", 6), 6)); // add eax, 4; mov ebx, eax; ret
            raw.push_back(RawGadget(string("\x8b\x59\xf7\x89\xd8\xc3", 6), 7)); // mov ebx, [ecx-9]; mov eax, ebx; ret
            raw.push_back(RawGadget(string("\x03\x39\xc3", 3), 8)); // add edi, [ecx]; ret
            raw.push_back(RawGadget(string("\xb9\x0a\x00\x00\x00\xc3", 6), 9)); // mov ecx, 10; ret
            raw.push_back(RawGadget(string("\x89\x0f\x89\x5e\xfd\xc3", 6), 10)); // mov [edi], ecx; mov [esi-3], ebx; ret
            raw.push_back(RawGadget(string("\xbe\x16\x00\x00\x00\xc3", 6), 11)); // mov esi, 22; ret
            raw.push_back(RawGadget(string("\xbf\x78\x56\x34\x12\xc3", 6), 12)); // mov edi, 0x12345678; ret
            raw.push_back(RawGadget(string("\x01\x21\xc3", 3), 13)); // add [ecx], esp; ret
            raw.push_back(RawGadget(string("\x33\x79\xf6\xc3", 4), 14)); // xor edi, [ecx-10]; ret
            raw.push_back(RawGadget(string("\x83\xc9\xff\xc3", 4), 15)); // or ecx, 0xffffffff; ret
            raw.push_back(RawGadget(string("\x21\x49\xf7\xc3", 4), 16)); // and [ecx-9], ecx; ret
            raw.push_back(RawGadget(string("\x01\x1E\xC3", 3), 17)); // add [esi], ebx; ret

            db.analyse_raw_gadgets(raw, &arch);

            // Test basic queries
            ropchain = comp.compile("eax = ecx");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" ebx = 4");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" ebx = eax + 4");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" ebx = eax + esi ");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" ebx = [ ecx - 0x9] ");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" ebx = [ 1] ");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" [edi] = ecx");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" [ esi-   0x3] = ebx");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" [19] = ebx");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" [0x12345678] = ecx");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" edi += [ecx]");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" edi ^= [0]");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" [ecx+0x000 ] += esp  \t\t\n\t  ");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" ecx = -1");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" [22] += 4");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");

            return nb;
        }

        unsigned int indirect_match(){
            unsigned int nb = 0;
            ArchX86 arch;
            GadgetDB db;
            ROPCompiler comp = ROPCompiler(&arch, &db);
            ROPChain* ropchain;
            Constraint constr;
            constr.bad_bytes.add_bad_byte(0xff);

            // Available gadgets
            vector<RawGadget> raw;
            raw.push_back(RawGadget(string("\x89\xf9\xbb\x01\x00\x00\x00\xc3", 8), 1)); // mov ecx, edi; mov ebx, 1; ret
            raw.push_back(RawGadget(string("\x89\xC8\xC3", 3), 2)); // mov eax, ecx; ret
            raw.push_back(RawGadget(string("\x89\xC3\xC3", 3), 3)); // mov ebx, eax; ret
            raw.push_back(RawGadget(string("\xb9\xad\xde\x00\x00\xc3", 6), 4)); // mov ecx, 0xdead; ret
            raw.push_back(RawGadget(string("\x5f\x5e\x59\xc3", 4), 5)); // pop edi; pop esi; pop ecx; ret
            raw.push_back(RawGadget(string("\x89\xE8\xFF\xE6", 4), 6)); // mov eax, ebp; jmp esi
            raw.push_back(RawGadget(string("\x89\xF1\xFF\xE0", 4), 7)); // mov ecx, esi; jmp eax
            raw.push_back(RawGadget(string("\x5A\x59\xC3", 3), 8)); // pop edx; pop ecx; ret
            raw.push_back(RawGadget(string("\x8B\x40\x08\xC3", 4), 9)); // mov eax, [eax + 8]; ret
            raw.push_back(RawGadget(string("\x8D\x4B\x08\xC3", 4), 10)); // lea ecx, [ebx + 8]; ret
            raw.push_back(RawGadget(string("\x8D\x40\x20\xFF\xE1", 5), 11)); // lea eax, [eax + 32]; jmp ecx;
            raw.push_back(RawGadget(string("\x89\x43\x08\xC3", 4), 12)); // mov [ebx + 8], eax; ret

            db.analyse_raw_gadgets(raw, &arch);

            // Test mov_reg_transitivity
            ropchain = comp.compile("eax = edi");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile("ebx = edi");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");

            // Test mov_cst_transitivity
            ropchain = comp.compile("eax = 0xdead");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile("ebx = 0xdead");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            
            // Test mov_cst pop
            ropchain = comp.compile(" edi =   -2");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" eax = 0x12345678  ");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            
            // Test generic adjust jmp
            ropchain = comp.compile(" ebx =  ebp");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" eax =  esi");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            
            // Test adjust load
            ropchain = comp.compile(" eax =  [ebx+16]");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            ropchain = comp.compile(" eax =  [eax+40]");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");

            // Test src transitivity
            ropchain = comp.compile(" [ebx+8] =  ebp");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            
            // Test adjust store
            ropchain = comp.compile(" [eax+8] =  ecx");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");

            return nb;
        }
        
        unsigned int store_string(){
            unsigned int nb = 0;
            
            ArchX86 arch;
            GadgetDB db;
            ROPCompiler comp = ROPCompiler(&arch, &db);
            ROPChain* ropchain;
            Constraint constr;
            constr.bad_bytes.add_bad_byte(0xff);

            // Available gadgets
            vector<RawGadget> raw;
            raw.push_back(RawGadget(string("\x89\xf9\xbb\x01\x00\x00\x00\xc3", 8), 1)); // mov ecx, edi; mov ebx, 1; ret
            raw.push_back(RawGadget(string("\x89\xC8\xC3", 3), 2)); // mov eax, ecx; ret
            raw.push_back(RawGadget(string("\x89\xC3\xC3", 3), 3)); // mov ebx, eax; ret
            raw.push_back(RawGadget(string("\xb9\xad\xde\x00\x00\xc3", 6), 4)); // mov ecx, 0xdead; ret
            raw.push_back(RawGadget(string("\x5f\x5e\x59\xc3", 4), 5)); // pop edi; pop esi; pop ecx; ret
            raw.push_back(RawGadget(string("\x89\xE8\xFF\xE6", 4), 6)); // mov eax, ebp; jmp esi
            raw.push_back(RawGadget(string("\x89\xF1\xFF\xE0", 4), 7)); // mov ecx, esi; jmp eax
            raw.push_back(RawGadget(string("\x5A\x59\xC3", 3), 8)); // pop edx; pop ecx; ret
            raw.push_back(RawGadget(string("\x8B\x40\x08\xC3", 4), 9)); // mov eax, [eax + 8]; ret
            raw.push_back(RawGadget(string("\x8D\x4B\x08\xC3", 4), 10)); // lea ecx, [ebx + 8]; ret
            raw.push_back(RawGadget(string("\x8D\x40\x20\xFF\xE1", 5), 11)); // lea eax, [eax + 32]; jmp ecx;
            raw.push_back(RawGadget(string("\x89\x43\x08\xC3", 4), 12)); // mov [ebx + 8], eax; ret

            db.analyse_raw_gadgets(raw, &arch);

            // Test adjust store
            ropchain = comp.compile(" [0x1234] =  'lala'");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");
            
            ropchain = comp.compile(" [0x1234] =  'lalatotoo\\x00'");
            nb += _assert_ropchain(ropchain, "Failed to find ropchain");

            return nb;
        }
        
        unsigned int incorrect_match(){
            unsigned int nb = 0;
            ArchX86 arch;
            GadgetDB db;
            ROPCompiler comp = ROPCompiler(&arch, &db);
            ROPChain* ropchain;
            Constraint constr;
            constr.bad_bytes.add_bad_byte(0xff);

            // Test when adjust gadget clobbers reg that must be set
            // Here gadget 2 and 3 both modify ecx
            vector<RawGadget> raw;
            raw.push_back(RawGadget(string("\x89\xF1\xFF\xE0", 4), 1)); // mov ecx, esi; jmp eax
            raw.push_back(RawGadget(string("\x59\xC3", 2), 2)); // pop ecx; ret
            raw.push_back(RawGadget(string("\x58\x59\xC3", 3), 3)); // pop eax; pop ecx; ret
            db.analyse_raw_gadgets(raw, &arch);
            ropchain = comp.compile(" ecx =  esi");
            nb += _assert_no_ropchain(ropchain, "Found ropchain but no ropchain should exist");
            
            // Test when adjust gadget clobbers input register
            // Here gadget 2 can set eax but modifies esi
            db.clear();
            raw.clear();
            raw.push_back(RawGadget(string("\x89\xF1\xFF\xE0", 4), 1)); // mov ecx, esi; jmp eax
            raw.push_back(RawGadget(string("\x5E\x58\xC3", 3), 2)); // pop esi; pop eax; ret
            raw.push_back(RawGadget(string("\xC3", 1), 3)); // ret
            db.analyse_raw_gadgets(raw, &arch);
            ropchain = comp.compile(" ecx =  esi");
            nb += _assert_no_ropchain(ropchain, "Found ropchain but no ropchain should exist");

            return nb;
        }
        
        unsigned int function_call_x86(){
            unsigned int nb = 0;
            ArchX86 arch;
            GadgetDB db;
            ROPCompiler comp = ROPCompiler(&arch, &db);
            ROPChain* ropchain;
            Constraint constr;
            constr.bad_bytes.add_bad_byte(0xff);
            // Test when adjust gadget clobbers reg that must be set
            // Here gadget 2 and 3 both modify ecx
            vector<RawGadget> raw;
            raw.push_back(RawGadget(string("\x58\xFF\xE0", 3), 1)); // pop eax; jmp eax
            raw.push_back(RawGadget(string("\xC3", 1), 2)); // ret
            raw.push_back(RawGadget(string("\x59\xC3", 2), 3)); // pop ecx; ret
            raw.push_back(RawGadget(string("\x83\xC4\x0C\xC3", 4), 4)); // add esp, 12; ret
            db.analyse_raw_gadgets(raw, &arch);

            // X86 CDECL ABI
            ropchain = comp.compile(" 0x1234(42)", nullptr, ABI::X86_CDECL);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");

            ropchain = comp.compile(" 0x12345678(42, -1, 43)", nullptr, ABI::X86_CDECL);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");

            ropchain = comp.compile(" 0()", nullptr, ABI::X86_CDECL);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            // X86 STDCALL ABI
            ropchain = comp.compile(" 0x1234(42)", nullptr, ABI::X86_STDCALL);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");

            ropchain = comp.compile(" 0x12345678(42, -1, 43)", nullptr, ABI::X86_STDCALL);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");

            ropchain = comp.compile(" 0()", nullptr, ABI::X86_STDCALL);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            return nb;
        }

        unsigned int function_call_x64(){
            unsigned int nb = 0;
            ArchX64 arch;
            GadgetDB db;
            ROPCompiler comp = ROPCompiler(&arch, &db);
            ROPChain* ropchain;
            Constraint constr;
            constr.bad_bytes.add_bad_byte(0xff);
            // Test when adjust gadget clobbers reg that must be set
            // Here gadget 2 and 3 both modify ecx
            vector<RawGadget> raw;
            raw.push_back(RawGadget(string("\x58\xFF\xE0", 3), 1)); // pop rax; jmp rax
            raw.push_back(RawGadget(string("\xC3", 1), 2)); // ret
            raw.push_back(RawGadget(string("\x5F\xC3", 2), 3)); // pop rdi; ret
            raw.push_back(RawGadget(string("\x48\x83\xC4\x08\xC3", 5), 4)); // add rsp, 8; ret
            raw.push_back(RawGadget(string("\x5E\xC3", 2), 5)); // pop rsi; ret
            raw.push_back(RawGadget(string("\x5A\xC3", 2), 6)); // pop rdx; ret
            raw.push_back(RawGadget(string("\x59\xC3", 2), 7)); // pop rcx; ret
            raw.push_back(RawGadget(string("\x41\x58\xc3", 3), 8)); // pop r8; ret
            raw.push_back(RawGadget(string("\x41\x59\x59\xc3", 4), 9)); // pop r9; pop rcx; ret
            raw.push_back(RawGadget(string("\x48\x89\xef\xc3",4), 10)); // mov rdi, rbp; ret
            raw.push_back(RawGadget(string("\x48\x83\xC4\x18\xC3", 5), 11)); // add rsp, 24; ret
            raw.push_back(RawGadget(string("\x49\x89\xd1\xc3",4), 12)); // mov r9, rdx; ret
            raw.push_back(RawGadget(string("\x48\x83\xC4\x10\xC3", 5), 13)); // add rsp, 16; ret
            db.analyse_raw_gadgets(raw, &arch);

            // X64 SYSTEM V
            ropchain = comp.compile(" 0x1234(42)", nullptr, ABI::X64_SYSTEM_V);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            ropchain = comp.compile(" 0x1234(1, 2, 3, 4)", nullptr, ABI::X64_SYSTEM_V);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            ropchain = comp.compile(" 0x1234(rbp, 2, 3, 4, 5, 6)", nullptr, ABI::X64_SYSTEM_V);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            ropchain = comp.compile(" 0x1234(rbp, 2, 3, 4, 5, 6, 7, 8)", nullptr, ABI::X64_SYSTEM_V);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            ropchain = comp.compile(" 0x1234(rbp, 2, 3, 4, 5, 6, 7, 8, 9)", nullptr, ABI::X64_SYSTEM_V);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");

            // X64 Microsoft
            ropchain = comp.compile(" 0x1234(42)", nullptr, ABI::X64_MS);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            ropchain = comp.compile(" 0x1234(1, 2, 3, rdx)", nullptr, ABI::X64_MS);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            ropchain = comp.compile(" 0x1234(1, 2, 3, 4, 5, 6)", nullptr, ABI::X64_MS);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");
            
            ropchain = comp.compile(" 0x1234(1, 2, 3, rdx, 5, 6, 7)", nullptr, ABI::X64_MS);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to call function");

            return nb;
        }

        unsigned int syscall_x86(){
            unsigned int nb = 0;
            ArchX86 arch;
            GadgetDB db;
            ROPCompiler comp = ROPCompiler(&arch, &db);
            ROPChain* ropchain;
            Constraint constr;
            constr.bad_bytes.add_bad_byte(0xff);
            vector<RawGadget> raw;
            raw.push_back(RawGadget(string("\x58\xC3", 2), 1)); // pop eax; ret
            raw.push_back(RawGadget(string("\x5B\xC3", 2), 2)); // pop ebx; ret
            raw.push_back(RawGadget(string("\x83\xC5\x20\x0F\x34", 5), 3)); // add ebp, 32; sysenter
            raw.push_back(RawGadget(string("\x59\xC3", 2), 4)); // pop ecx; ret
            raw.push_back(RawGadget(string("\x59\x5A\xC3", 3), 5)); // pop ecx; pop edx; ret
            raw.push_back(RawGadget(string("\x58\x89\xC6\xC3", 4), 6)); // pop eax; mov esi, eax; ret
            raw.push_back(RawGadget(string("\x89\xDA\xC3", 3), 7)); // mov edx, ebx; ret
            db.analyse_raw_gadgets(raw, &arch);

            // X86 Linux
            ropchain = comp.compile(" sys_exit(1)", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");

            ropchain = comp.compile(" sys_execve( 3, 2, 1)", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");

            ropchain = comp.compile(" sys_execve( 3, 2, ebx)", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");

            ropchain = comp.compile(" sys_ptrace( 3, 2, 1, 2)", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");

            ropchain = comp.compile("  sys_123(1, 2, 3) ", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");

            return nb;
        }
        
        unsigned int syscall_x64(){
            unsigned int nb = 0;
            ArchX64 arch;
            GadgetDB db;
            ROPCompiler comp = ROPCompiler(&arch, &db);
            ROPChain* ropchain;
            Constraint constr;
            constr.bad_bytes.add_bad_byte(0xff);
            vector<RawGadget> raw;
            raw.push_back(RawGadget(string("\x58\xC3", 2), 1)); // pop rax; ret
            raw.push_back(RawGadget(string("\x5F\xC3", 2), 2)); // pop rdi; ret
            raw.push_back(RawGadget(string("\x83\xC5\x20\x0F\x05", 5), 3)); // add ebp, 32; syscall
            raw.push_back(RawGadget(string("\x5E\xC3", 2), 4)); // pop rsi; ret
            raw.push_back(RawGadget(string("\x59\x5A\xC3", 3), 5)); // pop rcx; pop rdx; ret
            raw.push_back(RawGadget(string("\x58\x48\x89\xC6\xC3", 5), 6)); // pop rax; mov rsi, rax; ret
            raw.push_back(RawGadget(string("\x48\x89\xF2\xC3", 4), 7)); // mov rdx, rsi; ret
            db.analyse_raw_gadgets(raw, &arch);

            // X64 Linux
            ropchain = comp.compile(" sys_exit(1)", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");
            
            ropchain = comp.compile(" sys_execve(1, 2, 3)", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");
            
            ropchain = comp.compile(" sys_execve(1, 2, rsi)", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");

            ropchain = comp.compile(" sys_0x42(1, 2, rsi)", nullptr, ABI::NONE, System::LINUX);
            nb += _assert_ropchain(ropchain, "Couldn't build ropchain to make syscall");

            return nb;
        }
        
    }
}

using namespace test::compiler; 
// All unit tests 
void test_compiler(){
    unsigned int total = 0;
    string green = "\033[1;32m";
    string def = "\033[0m";
    string bold = "\033[1m";
    
    // Start testing 
    cout << bold << "[" << green << "+" << def << bold << "]" << def << std::left << std::setw(34) << " Testing ROP compiler... " << std::flush;  
    total += direct_match();
    total += indirect_match();
    total += function_call_x86();
    total += function_call_x64();
    total += syscall_x86();
    total += syscall_x64();
    total += store_string();
    total += incorrect_match();
    // Return res
    cout << "\t" << total << "/" << total << green << "\t\tOK" << def << endl;
}
