// SafeQueue.h
#pragma once
// 线程安全阻塞队列（管程形态）——移植自 ThreadLab，模板化以承载流水线的任务/结果两类消息。
//
// 语义铁律：
//   等待谓词 = 「非空 或 已结束」（析取，决定要不要继续睡）；
//   退出判据 = 「空 且 已结束」（合取，waitPop 返回 false 即此态）；
//   setDone 持锁置位后唤醒全部（只唤醒一个，其余睡眠者会永久挂睡）；
//   结束后 push 拒绝（返回 false），与调用顺序构成「关闭后不入队」的双层保证。
// 实现直接写在类内（隐式 inline），多个翻译单元各自包含也不会重定义。
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

template <typename T>
class SafeQueue
{
public:
    /* 放入一件（按右值接参，内部 move）。已结束后拒绝，返回 false */
    bool push(T &&value)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (isDone)
                return false;
            queue.push(std::move(value));
        }
        conditionVariable.notify_one(); // 解锁后再通知，别抱着锁唤醒
        return true;
    }

    /* 阻塞取队头。返回 true = 正常出货；false = 已结束且排空（退出信号，不是出错）。
     * 排空契约：setDone 之后已入队元素仍逐件交付，取完才返回 false */
    bool waitPop(T &value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        conditionVariable.wait(lock, [this]
                               { return !queue.empty() || isDone; });

        if (!queue.empty())
        {
            value = std::move(queue.front());
            queue.pop();
            return true; // ① 正常出货
        }
        return false; // ② 空 且 已结束
    }

    /* 标记「不再有新货进来」，唤醒所有等待者 */
    void setDone()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            isDone = true;
        }
        conditionVariable.notify_all();
    }

private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool isDone = false;
};
