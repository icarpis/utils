#ifndef LOCK_FREE_CRITICAL_SECTION_H
#define LOCK_FREE_CRITICAL_SECTION_H

#define TRUE 1
#define FALSE 0
#define N 3 // Number of processes

int turn[N];
int flags[N];

void enter_critical_section(int curr_process_id) {
    for (int count = 0; count < (N - 1); count++) {
        flags[curr_process_id] = count;                 // I think I'm in position "count" in the queue
        turn[count] = curr_process_id;                  // and I'm the most recent process to think I'm in position "count"

        // wait until
        // everyone thinks they're behind me 
        // or someone later than me thinks they're in position "count"
        for (int k = 0; k < N; k++)
        {
            while (((k == curr_process_id) || (flags[k] >= count)) &&
                (turn[count] == curr_process_id))
            {
                // Busy wait
            }
            // now I can update my estimated position to "count"+1
        }

        // now I'm at the head of the queue so I can start my critical section
    }
}

void leave_critical_section(int curr_process_id) {
    // Remove interest in entering the critical section
    flags[curr_process_id] = -1;
}

#endif
