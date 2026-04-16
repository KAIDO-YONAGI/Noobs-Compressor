#include "../include/GenHeffcodeTab.h"

GenHeffcodeTab::GenHeffcodeTab(Heffman* heffcore):
heffman(heffcore)
{ }

void GenHeffcodeTab::work(Datacmnctor* datacmnctor)
{
    out_blocks = datacmnctor->get_output_blocks();    
    heffman->merge_ttabs();
    heffman->gen_hefftree();
    heffman->save_code_inTab();

    tree_to_plat_uchar(out_blocks->at(0));
}

void GenHeffcodeTab::tree_to_plat_uchar(sfc::block_t& out_block)
{
    std::stack<Hefftreenode*> stack;
    root = heffman->getTreeRoot();
    stack.push(root);
    out_block.push_back('F');
    while(stack.empty() == false)
    {
        auto cur = stack.top();
        stack.pop();
        if(cur->isleaf == false)
            out_block.push_back('r');
        else
            out_block.push_back('l');
        out_block.push_back(cur->data);    
        stack.push(cur->right);
        stack.push(cur->left);
    }
}

