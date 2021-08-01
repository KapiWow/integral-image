#include <task_pool/task_pool.hh>

void TaskPool::init (int thread_count) {
    pool.reserve(thread_count);
    for(int i = 0; i != thread_count; ++i){
        pool.emplace_back(std::thread(&TaskPool::task_loop, this));
    }
}

void TaskPool::push(Task* task) {
    num_task_executing++;
    std::unique_lock<std::mutex> lock{mtx};
    tasks.push(task);
    cv.notify_one();
}

void TaskPool::wait() {
    while (num_task_executing > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds{10});
    }
}

void TaskPool::stop() {
    stopped = true;
    while (num_thread_alive > 0) {
        std::unique_lock<std::mutex> lock{mtx};
        cv.notify_all();
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds{10});
    }
    for (auto& thread : pool) {
        thread.join();
    }

    pool.clear();
}

void TaskPool::task_loop() {
    std::unique_lock<std::mutex> lock{mtx};
    num_thread_alive++;
    cv.wait(lock, [&] () {
        while (!tasks.empty()) {
		    Task* task = tasks.front();
		    tasks.pop();
            lock.unlock();
            task->execute();
            delete task;
            num_task_executing--;
            lock.lock();
        }
        return stopped.load();
    });
    num_thread_alive--;
}
