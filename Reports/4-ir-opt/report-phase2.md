# Lab4 实验报告

小组成员 姓名 学号

## 实验要求

请按照自己的理解，写明本次实验需要干什么

## 实验难点

* 活跃变量分析中phi指令的处理和相应数据结构的设计
* 常量和变量的判断

## 实验设计

* 常量传播
    实现思路：
    相应代码：
    优化前后的IR对比（举一个例子）并辅以简单说明：


* 循环不变式外提
    实现思路：
    相应代码：
    优化前后的IR对比（举一个例子）并辅以简单说明：
* 活跃变量分析
    对不同种类的指令进行分析，将变量加入到use集合和def集合中，并利用动态转换判断是否为常量(如下store指令操作)
    
    ```
    if (instr->is_store()) {
        auto l_val =
            dynamic_cast<StoreInst *>(instr)->get_lval();
        auto r_val =
            dynamic_cast<StoreInst *>(instr)->get_rval();
        if ((def.find(l_val) == def.end()) &&
            (dynamic_cast<ConstantInt *>(l_val) == nullptr)) {
            use.insert(l_val);
        }
        if ((def.find(r_val) == def.end()) &&
            (dynamic_cast<ConstantInt *>(r_val) == nullptr) &&
            (dynamic_cast<ConstantFP *>(r_val) == nullptr)) {
            use.insert(r_val);
        }
    ```
    对于phi指令，设计phi_uses结构进行记录，记录的值为变量和对应进入的BasicBlock。
    ```
    if (instr->is_phi()) {
        for (int i = 0; i < instr->get_num_operand() / 2; i++) {
            auto val = instr->get_operand(2 * i);
            if (def.find(val) == def.end() &&
                dynamic_cast<ConstantInt *>(val) == nullptr &&
                dynamic_cast<ConstantFP *>(val) == nullptr) {
                use.insert(val);
                phi_uses[bb].insert(
                    {val, dynamic_cast<BasicBlock *>(
                              instr->get_operand(2 * i + 1))});
            }
        }
        if (use.find(instr) == use.end()) {
            def.insert(instr);
    }
    ```
    获取了每个BasicBlock的use集合和def集合后，按照书上的算法计算每个BasicBlock的in集合和def集合
    ```
    for (auto bb : func->get_basic_blocks()) {
        std::set<Value *> empty_set;
        live_in[bb] = empty_set;
        live_out[bb] = empty_set;
    }
    bool is_change = true;
    while (is_change) {
        is_change = false;
        for (auto bb : func->get_basic_blocks()) {
            for (auto s : bb->get_succ_basic_blocks()) {
                for (auto b : live_in[s]) {
                    if (phi_uses.find(s) == phi_uses.end() ||
                        phi_uses[s].find(b) == phi_uses[s].end() ||
                        phi_uses[s][b] == bb) {
                        live_out[bb].insert(b);
                    }
                }
            }
            std::set<Value *> new_in_bb = live_out[bb];
            for (auto op : bb2def[bb]) {
                if (new_in_bb.find(op) != new_in_bb.end()) {
                    new_in_bb.erase(op);
                }
            }
            for (auto op : bb2use[bb]) {
                new_in_bb.insert(op);
            }
            if (new_in_bb != live_in[bb]) {
                is_change = true;
            }
            live_in[bb] = new_in_bb;
        }
    }
    ```

### 实验总结

此次实验有什么收获

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
