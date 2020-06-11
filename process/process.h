#pragma once

struct PCB;

namespace Process
{
    void activate_page_directory(PCB* pcb);
    void activate(PCB* pcb);
    void execute(void* file_name, const char* process_name);
};  // namespace Process