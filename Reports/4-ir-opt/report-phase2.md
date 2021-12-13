# Lab4 实验报告

小组成员 姓名 学号

## 实验要求

请按照自己的理解，写明本次实验需要干什么

## 实验难点

* 循环不变式的判断
* 活跃变量分析中phi指令的处理和相应数据结构的设计
* 常量和变量的判断

## 实验设计

* 常量传播
    实现思路：
    相应代码：
    优化前后的IR对比（举一个例子）并辅以简单说明：


* 循环不变式外提
    首先找到所有的的最底层循环：
    
    ```
        //找到所有最内层循环
        std::set<BasicBlock*> innerloopbase;
        for(auto loop=loop_searcher.begin();loop!=loop_searcher.end();loop++){
            auto base=loop_searcher.get_loop_base(*loop);
            auto innerloop=loop_searcher.get_inner_loop(base);
            auto ilp=loop_searcher.get_loop_base(innerloop);
            if(innerloopbase.find(ilp)==innerloopbase.end()){
                innerloopbase.insert(ilp);
            }
        }
    ```
    
    对每个最底层循环查找循环不变式并外提，然后查找外层循环的循环不变式
    
    ```
        for(auto it:innerloopbase){
            auto loop=loop_searcher.get_inner_loop(it);
            while(loop){
                loop_invariant.clear();
                find_loop_invariant(loop);//找到循环不变式
                moveout_loop_invariant(loop,loop_searcher);//循环不变式外提
                loop=loop_searcher.get_parent_loop(loop);//找到外层循环
            }
        }
    ```
    
    在每层循环中，先将所有会改变寄存器值的指令记为changedval，然后对所有changedval进行判断，假如操作数的值都没有发生变化，认为其为循环不变式，将其从changedval中剔除。load指令，call指令即使操作数不变也无法保证为循环不变式子，比如数组元素，phi指令无法外提，均排除。假如一个循环中得到了新的循环不变式，继续循环。
    
    ```
    std::unordered_set<Value*> changedval;
        for(auto BB:*loop){
            for(auto ins:BB->get_instructions()){
                if(!ins->is_void()){
                    changedval.insert(ins);
                }
            }
        }
        bool change=true;
        while(change){
            change=false;
            for(auto BB:*loop){
                for(auto ins:BB->get_instructions()){
                    bool flag=true;
                    if(ins->is_phi()||ins->is_call()||ins->is_void()||ins->is_load()||ins->is_alloca()||changedval.find(ins)==changedval.end()){
                        continue;
                    }
                    for(auto operand:ins->get_operands()){
                        if(changedval.find(operand)!=changedval.end()){
                            flag=false;
                            break;
                        }
                    }
                    if(flag){
                        changedval.erase(ins);
                        change=true;
                        loop_invariant.insert(std::make_pair(BB,ins));
                    }
                }
            }
        }
    ```
    
    外提循环不变式时，对循环入口的所有非本循环基本块前驱，将其最后的br指令删除，将循环不变式加入再添回br指令。
    
    ```
    if(loop_invariant.size()==0) return;//没有循环不变式直接返回
        std::set<BasicBlock*> pre_loop;
        auto loopbase=loop_searcher.get_loop_base(loop);
        for(auto preBB:loopbase->get_pre_basic_blocks()){
            if (loop->find(preBB) == loop->end())
                pre_loop.insert(preBB);
        }
        for(auto pre=pre_loop.begin();pre!=pre_loop.end();pre++){
            auto preBB=*pre;
            auto br=preBB->get_terminator();
            if(br->is_br()){
                preBB->delete_instr(br);//删除br
                for(auto pair:loop_invariant){//外提不变式
                    pair.first->delete_instr(pair.second);
                    preBB->add_instruction(pair.second);
                }
                preBB->add_instruction(br);//添回br
            }
        }
    ```
    
    优化前后的IR对比（举一个例子）并辅以简单说明：
    
    test.cminus:
    
    ```
    void main(void) {
        int a;
        int b;
        while(a<5){
            b=2+2;
            a=a+1;
        }
    }
    ```
    
    没有外提时：
    
    ```
    ; ModuleID = 'cminus'
    source_filename = "test.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      br label %label2
    label2:                                                ; preds = %label_entry, %label7
      %op12 = phi i32 [ %op10, %label7 ], [ undef, %label_entry ]
      %op13 = phi i32 [ %op8, %label7 ], [ undef, %label_entry ]
      %op4 = icmp slt i32 %op12, 5
      %op5 = zext i1 %op4 to i32
      %op6 = icmp ne i32 %op5, 0
      br i1 %op6, label %label7, label %label11
    label7:                                                ; preds = %label2
      %op8 = add i32 2, 2
      %op10 = add i32 %op12, 1
      br label %label2
    label11:                                                ; preds = %label2
      ret void
    }
    ```
    
    外提之后：
    
    ```
    ; ModuleID = 'cminus'
    source_filename = "test.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      %op8 = add i32 2, 2
      br label %label2
    label2:                                                ; preds = %label_entry, %label7
      %op12 = phi i32 [ %op10, %label7 ], [ undef, %label_entry ]
      %op13 = phi i32 [ %op8, %label7 ], [ undef, %label_entry ]
      %op4 = icmp slt i32 %op12, 5
      %op5 = zext i1 %op4 to i32
      %op6 = icmp ne i32 %op5, 0
      br i1 %op6, label %label7, label %label11
    label7:                                                ; preds = %label2
      %op10 = add i32 %op12, 1
      br label %label2
    label11:                                                ; preds = %label2
      ret void
    }
    
    ```
    
    本来在label7中的%op8 = add i32 2, 2即为循环不变式，将其外提到了label_entry中
    
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
