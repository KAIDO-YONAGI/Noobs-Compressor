// FileQueue.h
#ifndef FILEQUEUE
#define FILEQUEUE

#include "../include/HeaderReader.h"


class QueueInterface
{
public:
    FileQueue fileQueue;
};

class FileQueue
{
private:
    struct Node
    {
        std::pair<FileDetails, FileCount_Int> data;
        Node *next;
        Node(std::pair<FileDetails, FileCount_Int> &val)
            : data(val), next(nullptr) {}
    };

    Node *frontNode;
    Node *rearNode;
    size_t count;

public:
    FileQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}
    ~FileQueue()
    {     
        while (frontNode)
        { // 循环直到链表为空
            Node *temp = frontNode;
            frontNode = frontNode->next;
            delete temp; // 释放节点内存
        }
        rearNode = nullptr; // 重置尾指针
        count = 0;          // 重置计数器
    }
    void push(std::pair<FileDetails, FileCount_Int> val)
    {
        Node *newNode = new Node(val);
        if (rearNode)
        {
            rearNode->next = newNode;
        }
        else
        {
            frontNode = newNode;
        }
        rearNode = newNode;
        count++;
    } // 不使用引用，因为使用时会在传值时创建pair，会导致常量引用问题
    void pop()
    {
        if (empty())
        {
            return;
        }
        Node *temp = frontNode;
        frontNode = frontNode->next;
        if (frontNode == nullptr)
        {
            rearNode = nullptr;
        }
        delete temp;
        count--;
    }

    std::pair<FileDetails, FileCount_Int> & front()
    {
        if (empty())
        {
            std::cerr << "front()-Error:Queue is empty"
                    << "\n";
            return ;
        }
        return frontNode->data;
    }
        
    std::pair<FileDetails, FileCount_Int> &back()
    {
        if (empty())
        {
            std::cerr << "back()-Error:Queue is empty"
                    << "\n";
            return ;
        }
        return rearNode->data;
    }

    bool empty()
    {
        return count == 0;
    }

    size_t size()
    {
        return count;
    }
    bool fileIsExist(fs::path &outPutFilePath)
    {
        return fs::exists(outPutFilePath);
    }
};





#endif