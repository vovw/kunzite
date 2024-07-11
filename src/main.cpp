#include "gui.h"
#include "llm.h"
#include <thread>


int main() {
    LLM llm;
    std::thread llm_thread(&LLM::run, &llm);
    
    int result = run_gui(llm);
    
    llm.stop();
    llm_thread.join();
    
    return result;
}
