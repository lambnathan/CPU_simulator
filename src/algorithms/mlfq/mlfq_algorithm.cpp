#include "algorithms/mlfq/mlfq_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the MLFQ algorithm.
*/

MFLQScheduler::MFLQScheduler(int slice) {
    if(slice <= 0){
        time_slice = 3;
    }
    else{
        time_slice = slice;
    }
}

std::shared_ptr<SchedulingDecision> MFLQScheduler::get_next_thread() {
    // TODO
    return nullptr;
}

void MFLQScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    // TODO
}

size_t MFLQScheduler::size() const {
    return queue_0.size() + queue_1.size() + queue_2.size() + queue_3.size() + queue_4.size() + 
        queue_5.size() + queue_6.size() + queue_7.size() + queue_8.size() + queue_9.size();
}
