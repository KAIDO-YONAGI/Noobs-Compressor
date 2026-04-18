#include "../include/LoadHeffcodeTab.h"

LoadHeffcodeTab::LoadHeffcodeTab(Huffman* heffcore):
heffman(heffcore)
{ }

void LoadHeffcodeTab::work(DataConnector* dataConnector)
{
    inBlocks = dataConnector->getInputBlocks();

    root = spawnTree(inBlocks->at(0));
    heffman->receiveTreeRoot(root);
}

HeffTreeNode* LoadHeffcodeTab::spawnTree(sfc::block_t& inBlock)
{
    std::stack<HeffTreeNode*> stack;

    auto iterIb = inBlock.cbegin();
    if(*iterIb != 'F')
    {
        //TODO: 解析编码表异常，验证错误
    }
    ++iterIb;
    while(iterIb != inBlock.cend())
    {
        HeffTreeNode *node = NULL;
        if(iterIb + 1 == inBlock.cend())
        {
            //TODO: 解析编码表异常，数据缺失
        }
        if(*iterIb == 'r')
        {
            node = new HeffTreeNode(*++iterIb, 0, false);
            stack.push(node);
        }
        else if(*iterIb == 'l')
        {
            node = new HeffTreeNode(*++iterIb, 0, true);
            while(!stack.empty() && connectNode(stack.top(), node))
            {
                stack.pop();
            }
        }
        if(node == NULL)
        {
            //TODO: 解析编码表异常，创建节点失败
        }
        ++iterIb;
    }

    while(stack.size() != 1)
        stack.pop();
    return stack.top();
}

bool LoadHeffcodeTab::connectNode(HeffTreeNode* p, HeffTreeNode* c)
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
