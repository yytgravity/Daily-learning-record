#ifndef DEBUGGER_DEBUGGER_H
#define DEBUGGER_DEBUGGER_H
#include <utility>
#include <string>
#include <linux/types.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/types.h>

#include "breakpoint.h"
#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"

class debugger{
public:
    debugger (std::string prog_name, pid_t pid)
        : m_prog_name {std::move(prog_name)},m_pid{pid} {
        auto fd = open(m_prog_name.c_str(),O_RDONLY);

        m_elf = elf::elf{elf::create_mmap_loader(fd)};
        m_dwarf = dwarf::dwarf{dwarf::elf::create_loader(m_elf)};
    }

    void run();
    void set_brakepoint_at_address(std::intptr_t addr);
    void dump_registers();
    void print_source(const std::string& file_name, unsigned line, unsigned n_lines_context=2);
    void single_step_instruction();
    void single_step_instruction_with_breakpoint_check();
    void step_in();
    void step_over();
    void step_out();
    void remove_breakpoint(std::intptr_t addr);

private:
    void handle_command(const std::string &line);
    void continue_execution();
    void run_process();
    auto get_pc() -> uint64_t ;
    void set_pc(uint64_t pc);
    void step_over_breakpoint();
    void wait_for_signal();
    //auto get_signal_info() -> siginfo_t;

    //void handle_sigtrap(siginfo_t info);

    auto get_function_from_pc(uint64_t pc) -> dwarf::die;
    auto get_line_entry_from_pc(uint64_t pc) -> dwarf::line_table::iterator;

    auto read_memory(uint64_t address) -> uint64_t ;
    void write_memory(uint64_t address, uint64_t value);

    std::string m_prog_name;
    std::unordered_map<std::intptr_t,breakpoint> m_breakpoints;
    pid_t m_pid;
    dwarf::dwarf m_dwarf;
    elf::elf m_elf;
};

#endif //DEBUGGER_DEBUGGER_H
