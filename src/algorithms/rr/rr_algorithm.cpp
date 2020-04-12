#include "algorithms/rr/rr_algorithm.hpp"

#include <cassert>
#include <stdexcept>
#include <sstream>

/*
    Here is where you should define the logic for the round robin algorithm.
*/

RRScheduler::RRScheduler(int slice) {   
    if(slice <= 0){
        time_slice = 3;
    }
    else{
        time_slice = slice;
    }
}

std::shared_ptr<SchedulingDecision> RRScheduler::get_next_thread() {
    int num_threads = thread_queue.size();
    std::shared_ptr<Thread> thread;
    if(num_threads == 0){
        thread = nullptr;
    }
    else{
        thread = thread_queue.front();
        thread_queue.pop();
    }
    std::shared_ptr<SchedulingDecision> sd = std::make_shared<SchedulingDecision>();
    sd->thread = thread;
    sd->explanation = "Selected from " + std::to_string(num_threads) + " threads. Will run for at most " + std::to_string(time_slice) + " ticks.";
    sd->time_slice = this->time_slice;
    return sd;
}

void RRScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    thread_queue.push(thread);
}

size_t RRScheduler::size() const {
    return thread_queue.size();
}
