#include "LoopInvHoist.hpp"

#include <algorithm>

#include "LoopSearch.hpp"
#include "logging.hpp"

void LoopInvHoist::run() {
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, true);
    loop_searcher.run();

    // 接下来由你来补充啦！
    //找到所有最内层循环
    std::set<BasicBlock*> innerloopbase;
    std::set<BasicBlock*> deleteloopbase;
    for(auto loop=loop_searcher.begin();loop!=loop_searcher.end();loop++){
        innerloopbase.insert(loop_searcher.get_loop_base(*loop));
    }
    for(auto loop=loop_searcher.begin();loop!=loop_searcher.end();loop++){
        auto parent=loop_searcher.get_parent_loop(*loop);
        if(parent){
            deleteloopbase.insert(loop_searcher.get_loop_base(parent));
        }
    }
    for(auto BB=deleteloopbase.begin();BB!=deleteloopbase.end();BB++){
        if(innerloopbase.find(*BB)!=innerloopbase.end())
            innerloopbase.erase(*BB);
    }
    for(auto it:innerloopbase){
        auto loop=loop_searcher.get_inner_loop(it);
        while(loop){
            loop_invariant.clear();
            find_loop_invariant(loop);  //找到循环不变式
            moveout_loop_invariant(loop, loop_searcher);  //循环不变式外提
            loop = loop_searcher.get_parent_loop(loop);   //找到外层循环
        }
    }
}
void LoopInvHoist::find_loop_invariant(BBset_t* loop) {
    std::unordered_set<Value*> changedval;
    for (auto BB : *loop) {
        for (auto ins : BB->get_instructions()) {
            if (!ins->is_void()) {
                changedval.insert(ins);
            }
        }
    }
    bool change = true;
    while (change) {
        change = false;
        for (auto BB : *loop) {
            for (auto ins : BB->get_instructions()) {
                bool flag = true;
                if (ins->is_phi() || ins->is_call() || ins->is_void() ||
                    ins->is_load() || ins->is_alloca() ||
                    changedval.find(ins) == changedval.end()) {
                    continue;
                }
                for (auto operand : ins->get_operands()) {
                    if (changedval.find(operand) != changedval.end()) {
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    changedval.erase(ins);
                    change = true;
                    loop_invariant.insert(std::make_pair(BB, ins));
                }
            }
        }
    }
}
void LoopInvHoist::moveout_loop_invariant(BBset_t* loop,
                                          LoopSearch loop_searcher) {
    if (loop_invariant.size() == 0)
        return;  //没有循环不变式直接返回
    std::set<BasicBlock*> pre_loop;
    auto loopbase = loop_searcher.get_loop_base(loop);
    for (auto preBB : loopbase->get_pre_basic_blocks()) {
        if (loop->find(preBB) == loop->end())
            pre_loop.insert(preBB);
    }
    for (auto pre = pre_loop.begin(); pre != pre_loop.end(); pre++) {
        auto preBB = *pre;
        auto br = preBB->get_terminator();
        if (br->is_br()) {
            preBB->delete_instr(br);            //删除br
            for (auto pair : loop_invariant) {  //外提不变式
                pair.first->delete_instr(pair.second);
                preBB->add_instruction(pair.second);
            }
            preBB->add_instruction(br);  //添回br
        }
    }
}
