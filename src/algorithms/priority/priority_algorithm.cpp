#include "algorithms/priority/priority_algorithm.hpp"

#include <cassert>
#include <stdexcept>
#include <sstream>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the priority algorithm.
*/

PRIORITYScheduler::PRIORITYScheduler(int slice) {
    if (slice != -1) {
        throw("PRIORITY must have a timeslice of -1");
    }
    time_slice = -1;
}

std::shared_ptr<SchedulingDecision> PRIORITYScheduler::get_next_thread() {
    int total_queue_size = size();
    std::shared_ptr<Thread> thread = nullptr;
    std::string message = "";
    if(system_queue.size() != 0){
        message = "Selected from SYSTEM queue. ";
        message = message + message_builder();
        thread = system_queue.front();
        system_queue.pop();
        message = message + " -> " + message_builder();
    }
    else if(interactive_queue.size() != 0){
        message = "Selected from INTERACTIVE queue. ";
        message = message + message_builder();
        thread = interactive_queue.front();
        interactive_queue.pop();
        message = message + " -> " + message_builder();
    }
    else if(normal_queue.size() != 0){
        message = "Selected from NORMAL queue. ";
        message = message + message_builder();
        thread = normal_queue.front();
        normal_queue.pop();
        message = message + " -> " + message_builder();
    }
    else if(batch_queue.size() != 0){
        message = "Selected from BATCH queue. ";
        message = message + message_builder();
        thread = batch_queue.front();
        batch_queue.pop();
        message = message + " -> " + message_builder();
    }
    std::shared_ptr<SchedulingDecision> sd = std::make_shared<SchedulingDecision>();
    sd->thread = thread;
    sd->explanation = message;
    sd->time_slice = time_slice;
    return sd;

}

std::string PRIORITYScheduler::message_builder(){
    return "[S: " + std::to_string(system_queue.size()) + " I: " + std::to_string(interactive_queue.size()) + 
        " N: " + std::to_string(normal_queue.size()) + " B: " + std::to_string(batch_queue.size()) + "]";
}

void PRIORITYScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    //add the thread to the appropriate queue
    if(thread->priority == SYSTEM){
        system_queue.push(thread);
    }
    else if(thread->priority == INTERACTIVE){
        interactive_queue.push(thread);
    }
    else if(thread->priority == NORMAL){
        normal_queue.push(thread);
    }
    else{
        batch_queue.push(thread);
    }
}

size_t PRIORITYScheduler::size() const {
    return system_queue.size() + interactive_queue.size() + normal_queue.size() + batch_queue.size();
}
