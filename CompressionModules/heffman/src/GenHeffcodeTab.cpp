#include "../include/GenHeffcodeTab.h"

GenHeffcodeTab::GenHeffcodeTab(Huffman* heffcore):
heffman(heffcore)
{ }

void GenHeffcodeTab::work(DataConnector* datacmnctor)
{
    outBlocks = datacmnctor->getOutputBlocks();
    heffman->mergeTtabs();
    heffman->genHefftree();
    heffman->saveCodeInTab();

    treeToPlatUchar(outBlocks->at(0));
}

void GenHeffcodeTab::treeToPlatUchar(sfc::block_t& outBlock)
{
    std::stack<HeffTreeNode*> stack;
    root = heffman->getTreeRoot();
    stack.push(root);
    outBlock.push_back('F');
    while(stack.empty() == false)
    {
        auto cur = stack.top();
        stack.pop();
        if(cur->isLeaf == false)
            outBlock.push_back('r');
        else
            outBlock.push_back('l');
        outBlock.push_back(cur->data);
        stack.push(cur->right);
        stack.push(cur->left);
    }
}
