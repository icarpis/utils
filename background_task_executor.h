#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>



class background_task_executor final
{
public:
    static background_task_executor* get_instance()
    {
        static background_task_executor singleton;
        return &singleton;
    }

    void add_task(std::function<void(void)> task)
    {
        {
            std::lock_guard<std::mutex> lck(m_cv_mtx);
            m_task_q.push(task);
        }
        m_cv.notify_one();
    }

private:
    background_task_executor()
        : m_terminate(false)
    {
        m_thread = std::thread([&]()
            {
                while (true)
                {
                    try
                    {
                        {
                            std::unique_lock<std::mutex> lck(m_cv_mtx);
                            m_cv.wait(lck, [&] {return ((m_task_q.size() != 0) || m_terminate); });

                            if (m_terminate)
                            {
                                break;
                            }
                        }

                        while (1)
                        {
                            std::function<void(void)> cb;
                            {
                                std::lock_guard<std::mutex> lck(m_cv_mtx);
                                if (m_task_q.size() == 0)
                                {
                                    break;
                                }

                                cb = m_task_q.front();
                                m_task_q.pop();
                            }

                            if (cb)
                            {
                                cb();
                            }
                        }
                    }
                    catch (...)
                    {

                    }
                }
            });
    }


    background_task_executor(const background_task_executor&);

    ~background_task_executor()
    {
        try
        {
            {
                std::lock_guard<std::mutex> lck(m_cv_mtx);
                m_terminate = true;
            }
            m_cv.notify_one();

            m_thread.join();
        }
        catch (...)
        {

        }
    }

    std::thread                           m_thread;
    std::mutex                            m_cv_mtx;
    std::condition_variable               m_cv;
    std::queue<std::function<void(void)>> m_task_q;
    bool                                  m_terminate;
};
