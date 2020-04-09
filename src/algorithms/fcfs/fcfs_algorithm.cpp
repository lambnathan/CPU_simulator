#include "algorithms/fcfs/fcfs_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the FCFS algorithm.
*/

FCFSScheduler::FCFSScheduler(int slice) {
    if (slice != -1) {
        throw("FCFS must have a timeslice of -1");
    }
}

std::shared_ptr<SchedulingDecision> FCFSScheduler::get_next_thread() {
    int num_threads = thread_queue.size();
    std::shared_ptr<Thread> thread;
    if(num_threads == 0){
        thread = nullptr;
    }
    else{
        thread = thread_queue.front();
        thread_queue.pop();
    }
    std::shared_ptr<SchedulingDecision> sd;
    sd->thread = thread;
    sd->explanation = "Selected from " + std::to_string(num_threads) + " threads. Will run to completion of burst.";
    sd->time_slice = -1;
    return sd;
}

void FCFSScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    thread_queue.push(thread);
}

size_t FCFSScheduler::size() const {
    return thread_queue.size();
}