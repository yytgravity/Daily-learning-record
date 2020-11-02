#include <iostream>
#include <vector>
#include <cstring>
#include <sys/ptrace.h>
#include "debugger.h"
#include "breakpoint.h"
#include <wait.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include "your_path/linenoise.h"
#include "registers.h"

bool is_prefix(const std::string &s, const std::string &of){
    if (s.size() > of.size()) return false;
    return std::equal(s.begin(),s.end(),of.begin());
}

std::vector<std::string> split(const std::string &s, char delimiter){
    std::vector<std::string> out{};
    std::stringstream ss {s};
    std::string item;

    while (std::getline(ss,item,delimiter)){
        out.push_back(item);
    }
    return out;
}

void debugger::run_process() {
    ptrace(PTRACE_CONT,m_pid, nullptr, nullptr);

    int wait_status;
    auto options = 0;
    waitpid(m_pid,&wait_status,options);
}

void debugger::continue_execution() {
    step_over_breakpoint();
    ptrace(PTRACE_CONT,m_pid, nullptr, nullptr);
    wait_for_signal();
}

uint64_t debugger::get_pc(){
    return get_register_value(m_pid,reg::rip);
}

void debugger::set_pc(uint64_t pc) {
    set_register_value(m_pid,reg::rip,pc);
}

void debugger::step_over_breakpoint() {
    // -1
    auto possible_brakepoint_location = get_pc() - 1 ;
    if (m_breakpoints.count(possible_brakepoint_location)) {
        auto& bp = m_breakpoints[possible_brakepoint_location];

        if (bp.is_enabled()) {
            auto previous_instruction_address = possible_brakepoint_location;
            set_pc(previous_instruction_address);

            bp.disable();
            ptrace(PTRACE_SINGLESTEP,m_pid, nullptr, nullptr);
            wait_for_signal();
            bp.enable();
        }
    }
}

void debugger::wait_for_signal() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid,&wait_status,options);
}

void debugger::handle_command(const std::string &line){
    auto args = split(line,' ');
    auto command = args[0];

    if (is_prefix(command,"continue")){
        continue_execution();
    }
    else if(is_prefix(command,"run")){
        run_process();
    }
    else if(is_prefix(command,"break")){
        std::string addr {args[1],2}; // b 0x66666, is 0x66666?
        set_brakepoint_at_address(std::stol(addr,0,16));
    }
    else if(is_prefix(command,"register")) {
        if (is_prefix(args[1], "dump")) {
            dump_registers();
        }
        else if (is_prefix(args[1], "read")) {
            std::cout << get_register_value(m_pid,get_register_from_name(args[2])) << std::endl;
        }
        else if (is_prefix(args[1], "write")) {
            std::string val {args[3], 2};
            set_register_value(m_pid, get_register_from_name(args[2]),std::stol(val,0,16));
        }
    }
    else if (is_prefix(command,"strp")) {
        step_in();
    }
    else if (is_prefix(command,"next")) {
        step_over();
    }
    else if (is_prefix(command,"finish")) {
        step_out();
    }
    else{
        std::cerr << "Unknown command\n";
    }
}

void debugger::run() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid,&wait_status,options);

    char* line = nullptr;
    while ((line = linenoise("zdb> ")) != nullptr){
        handle_command(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void breakpoint::enable() {
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    m_saved_data = static_cast<uint8_t>(data & 0xff);
    uint64_t int3 = 0xcc;
    uint64_t data_with_int3 = ((data & ~0xff) | int3);
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);

    m_enabled = true;
}

void breakpoint::disable() {
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    auto restored_data = ((data & ~0xff) | m_saved_data);
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, restored_data);

    m_enabled = false;
}

void debugger::set_brakepoint_at_address(std::intptr_t addr) {
    std::cout << "Set breakpoint at address 0x" << std::hex << addr << std::endl;
    breakpoint bp {m_pid, addr};
    bp.enable();
    m_breakpoints[addr] = bp;
}

void debugger::dump_registers() {
    for (const auto& rd : g_register_descriptors) {
        if (rd.name == "orig_rax"){
            std::cout << rd.name << "\t" << " 0x"
                      << std::setfill('0') << std::setw(16) << std::hex << get_register_value(m_pid,rd.r) << std::endl;
            continue;
        }
        std::cout << rd.name << "\t\t" << " 0x"
                             << std::setfill('0') << std::setw(16) << std::hex << get_register_value(m_pid,rd.r) << std::endl;
    }
}

void debugger::single_step_instruction() {
    ptrace(PTRACE_SINGLESTEP,m_pid, nullptr, nullptr);
    wait_for_signal();
}

void debugger::single_step_instruction_with_breakpoint_check() {
    if (m_breakpoints.count(get_pc())) {
        step_over_breakpoint();
    }else{
        single_step_instruction();
    }
}

uint64_t debugger::read_memory(uint64_t address) {
    return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void debugger::write_memory(uint64_t address, uint64_t value) {
    ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

void debugger::step_out() {
    auto frame_pointer = get_register_value(m_pid, reg::rbp);
    auto return_address = read_memory(frame_pointer+8);

    bool should_remove_breakpoint = false;
    if (!m_breakpoints.count(return_address)) {
        set_brakepoint_at_address(return_address);
        should_remove_breakpoint = true;
    }

    continue_execution();

    if (should_remove_breakpoint) {
        remove_breakpoint(return_address);
    }
}

dwarf::line_table::iterator debugger::get_line_entry_from_pc(uint64_t pc) {
    for (auto &cu : m_dwarf.compilation_units()) {
        if (die_pc_range(cu.root()).contains(pc)) {
            auto &lt = cu.get_line_table();
            auto it = lt.find_address(pc);
            if (it == lt.end()) {
                throw std::out_of_range{"Cannot find line entry"};
            }
            else {
                return it;
            }
        }
    }

    throw std::out_of_range{"Cannot find line entry"};
}

dwarf::die debugger::get_function_from_pc(uint64_t pc) {
    for (auto &cu : m_dwarf.compilation_units()) {
        if (die_pc_range(cu.root()).contains(pc)) {
            for (const auto& die : cu.root()) {
                if (die.tag == dwarf::DW_TAG::subprogram) {
                    if (die_pc_range(die).contains(pc)) {
                        return die;
                    }
                }
            }
        }
    }

    throw std::out_of_range{"Cannot find function"};
}

void debugger::step_in() {
    auto line = get_line_entry_from_pc(get_pc()) -> line;

    while (get_line_entry_from_pc(get_pc())->line == line) {
        single_step_instruction_with_breakpoint_check();
    }

    auto line_entry = get_line_entry_from_pc(get_pc());
}

void debugger::step_over() {
    auto func = get_function_from_pc(get_pc());
    auto func_entry = at_low_pc(func);
    auto func_end = at_high_pc(func);

    auto line = get_line_entry_from_pc(func_entry);
    auto start_line = get_line_entry_from_pc(get_pc());

    std::vector<std::intptr_t> to_delete{};

    while (line->address < func_end) {
        if (line->address != start_line->address && !m_breakpoints.count(line->address)){
            set_brakepoint_at_address(line->address);
            to_delete.push_back(line->address);
        }
        ++line;
    }

    auto frame_pointer = get_register_value(m_pid, reg::rbp);
    auto return_address = read_memory(frame_pointer+8);
    if (!m_breakpoints.count(return_address)) {
        set_brakepoint_at_address(return_address);
        to_delete.push_back(return_address);
    }

    continue_execution();

    for (auto addr : to_delete){
        remove_breakpoint(addr);
    }
}

void debugger::remove_breakpoint(std::intptr_t addr) {
    if (m_breakpoints.at(addr).is_enabled()) {
        m_breakpoints.at(addr).disable();
    }
    m_breakpoints.erase(addr);
}

void execute_debugee (const std::string& prog_name) {
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        std::cerr << "Error in ptrace\n";
        return;
    }
    execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}

int main(int argc, char* argv[]) {
    if (argc < 2){
        std::cerr << "Program name not specified";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();
    if (pid == 0){
        execute_debugee(prog);
    }
    else if (pid >= 1){
        debugger dbg(prog,pid);
        dbg.run();
    }
    return 0;
}
