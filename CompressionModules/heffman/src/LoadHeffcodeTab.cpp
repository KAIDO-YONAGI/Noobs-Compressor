#include "../include/LoadHeffcodeTab.h"

LoadHeffcodeTab::LoadHeffcodeTab(Heffman* heffcore):
heffman(heffcore)
{ }

LoadHeffcodeTab::~LoadHeffcodeTab()
{ }

void LoadHeffcodeTab::work(Datacmnctor* datacmnctor)
{
    in_blocks = datacmnctor->get_input_blocks();
    
    root = spawn_tree(in_blocks->at(0));
    heffman->receiveTreRroot(root);
}

Hefftreenode* LoadHeffcodeTab::spawn_tree(sfc::block_t& in_block)
{
    std::stack<Hefftreenode*> stack;

    auto iter_ib = in_block.cbegin();
    if(*iter_ib != 'F')
    {
        //TODO: 解析编码表异常，验证错误
    }
    ++iter_ib;
    while(iter_ib != in_block.cend())
    {
        Hefftreenode *node = NULL;
        if(iter_ib + 1 == in_block.cend())
        {
            //TODO: 解析编码表异常，数据缺失
        }
        if(*iter_ib == 'r')
        {
            node = new Hefftreenode(*++iter_ib, 0, false);
            stack.push(node);
        }
        else if(*iter_ib == 'l')
        {
            node = new Hefftreenode(*++iter_ib, 0, true);
            while(!stack.empty() && connectNode(stack.top(), node))
            {
                stack.pop();
            }
        }
        if(node == NULL)
        {
            //TODO: 解析编码表异常，创建节点失败
        }
        ++iter_ib;
    }

    while(stack.size() != 1)
        stack.pop();
    return stack.top();
}

bool LoadHeffcodeTab::connectNode(Hefftreenode* p, Hefftreenode* c)
{
    if(p->left == NULL)
    {
        p->left = c;
        return true;
    }
    if(p->right == NULL)
    {
        p->right = c;
        return true;
    }
    return false;
}

