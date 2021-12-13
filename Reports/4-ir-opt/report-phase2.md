# Lab4 实验报告

| 小组成员 |  姓名  |    学号    |
| :------: | :----: | :--------: |
|    1     |  汪震  | PB19000078 |
|    2     |  蒋皓  | PB19000060 |
|    3     | 方俊浩 | PB19000052 |

## 实验要求

请按照自己的理解，写明本次实验需要干什么

## 实验难点

- 活跃变量分析中 phi 指令的处理和相应数据结构的设计
- 常量和变量的判断

## 实验设计

- 循环不变式外提
  首先找到所有的的最底层循环：

  ```cpp
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

  ```cpp
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

  在每层循环中，先将所有会改变寄存器值的指令记为 changedval，然后对所有 changedval 进行判断，假如操作数的值都没有发生变化，认为其为循环不变式，将其从 changedval 中剔除。load 指令，call 指令即使操作数不变也无法保证为循环不变式子，比如数组元素，phi 指令无法外提，均排除。假如一个循环中得到了新的循环不变式，继续循环。

  ```cpp
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

  外提循环不变式时，对循环入口的所有非本循环基本块前驱，将其最后的 br 指令删除，将循环不变式加入再添回 br 指令。

  ```cpp
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

  优化前后的 IR 对比（举一个例子）并辅以简单说明：
  test.cminus:

  ```cpp
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

  ```llvm
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

  ```llvm
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

  本来在 label7 中的%op8 = add i32 2, 2 即为循环不变式，将其外提到了 label_entry 中

- 常量传播

  1.  实现思路：遍历 module 中每个函数每个基本块中的所有语句，根据指令的类型进行处理。
  2.  load, store 指令：
      1. 思路：处理全局变量在块内的常量传播。遇到 store 指令时，判断是否为全局变量，存储值是否为常量。如果均满足，将全局变量存入`global_is_const(std::unordered_set<GlobalVariable*>)`，将全局变量-值存入`global2*val`，分别为`std::unordered_map<GlobalVariable*, int> global2ival`和`std::unordered_map<GlobalVariable*, float> global2fval`，根据全局变量的类型进行区分。之后遇到 load 指令时，判断是否为全局变量，如果是，判断是否在`global_is_const`集合中，如果在，那么后续所有用到这个变量的语句，全部用对应常量进行替换，并将该 load 语句加入`wait_delete`等待后续删除。
      2. 相应代码：
      ```c++
      case Instruction::load: {
         GlobalVariable* global_var =
             dynamic_cast<GlobalVariable*>(operands[0]);
         if (global_var && global_is_const.find(global_var) !=
                               global_is_const.cend()) {
             if (global_var->get_type()
                     ->get_pointer_element_type()
                     ->is_integer_type()) {
                 instr->replace_all_use_with(ConstantInt::get(
                     global2ival[global_var], m_));
                 wait_delete.emplace_back(instr);
             } else if (global_var->get_type()
                            ->get_pointer_element_type()
                            ->is_float_type()) {
                 instr->replace_all_use_with(ConstantFP::get(
                     global2fval[global_var], m_));
                 wait_delete.emplace_back(instr);
             }
         }
      } break;
      case Instruction::store: {
         GlobalVariable* global_var =
             dynamic_cast<GlobalVariable*>(operands[1]);
         if (global_var) {
             if (global_var->get_type()
                     ->get_pointer_element_type()
                     ->is_integer_type()) {
                 ConstantInt* intptr =
                     dynamic_cast<ConstantInt*>(operands[0]);
                 if (intptr) {
                     int val = intptr->get_value();
                     global2ival[global_var] = val;
                     global_is_const.insert(global_var);
                 } else {
                     global2ival.erase(global_var);
                     global_is_const.erase(global_var);
                 }
             } else if (global_var->get_type()
                            ->get_pointer_element_type()
                            ->is_float_type()) {
                 ConstantFP* fptr =
                     dynamic_cast<ConstantFP*>(operands[0]);
                 if (fptr) {
                     float val = fptr->get_value();
                     global2fval[global_var] = val;
                     global_is_const.insert(global_var);
                 } else {
                     global2fval.erase(global_var);
                     global_is_const.erase(global_var);
                 }
             }
         }
      } break;
      ```
  3.  br 指令：
      1. 思路：判断分支条件是否为常量，如果是常量，根据常量值生成无条件跳转指令。
      2. 相应代码
      ```c++
      case Instruction::br: {
         if (operands.size() == 3) {
             ConstantInt* val =
                 dynamic_cast<ConstantInt*>(operands[0]);
             if (val) {
                 BasicBlock *if_true, *if_false;
                 if_true =
                     dynamic_cast<BasicBlock*>(operands[1]);
                 if_false =
                     dynamic_cast<BasicBlock*>(operands[2]);
                 if (val->get_value())
                     BranchInst::create_br(if_true, bb);
                 else
                     BranchInst::create_br(if_false, bb);
                 wait_delete.emplace_back(instr);
             }
         }
      } break;
      ```
  4.  zext 指令：
      1. 思路：判断需要拓展的是否为整型常量，如果是，那么对于后续所有使用到拓展后变量的表达式，用常量替换掉。
      2. 相应代码：
         ```c++
         case Instruction::zext: {
             ConstantInt* intptr =
                 dynamic_cast<ConstantInt*>(operands[0]);
             if (intptr) {
                 instr->replace_all_use_with(
                     ConstantInt::get(intptr->get_value(), m_));
                 wait_delete.emplace_back(instr);
             }
         } break;
         ```
  5.  fptosi, sitofp 指令：
      1. 思路：判断需要进行转换的是否为常量，如果是，那么对后续语句进行常量替换。
      2. 相应代码：
      ```c++
      case Instruction::fptosi: {
         ConstantFP* fptr =
             dynamic_cast<ConstantFP*>(operands[0]);
         if (fptr) {
             instr->replace_all_use_with(
                 ConstantInt::get(int(fptr->get_value()), m_));
             wait_delete.emplace_back(instr);
         }
      } break;
      case Instruction::sitofp: {
         ConstantInt* intptr =
             dynamic_cast<ConstantInt*>(operands[0]);
         if (intptr) {
             instr->replace_all_use_with(
                 ConstantFP::get(intptr->get_value(), m_));
             wait_delete.emplace_back(instr);
         }
      } break;
      ```
  6.  其余 add, sub, mul, sdiv, cmp, fadd, fsub, fmul, fdiv, fcmp 指令
      1. 思路：由于操作类似，都放到 default 中，交给`ConstFolder::compute`进行处理，`compute`计算出相应的常量值，之后回到`run`中对后续语句进行常量替换。
      2. 相应代码：
         ```c++
         default: {
             // include add, sub, mul, sdiv, cmp
             // fadd, fsub, fmul, fdiv, fcmp
             if (operands.size() == 2) {
                 Constant *lval, *rval;
                 lval = dynamic_cast<Constant*>(operands[0]);
                 rval = dynamic_cast<Constant*>(operands[1]);
                 if (lval && rval) {
                     auto result =
                         floder_.compute(instr, lval, rval);
                     instr->replace_all_use_with(result);
                     wait_delete.emplace_back(instr);
                 }
             }
         } break;
         ```
         ```cpp
         Constant* ConstFolder::compute(Instruction* instr, Constant* value1,
                                        Constant* value2) {
             auto constant_int_ptr1 = dynamic_cast<ConstantInt*>(value1);
             auto constant_int_ptr2 = dynamic_cast<ConstantInt*>(value2);
             if (constant_int_ptr1 && constant_int_ptr2) {
                 // both integer
                 int c_value1 = constant_int_ptr1->get_value();
                 int c_value2 = constant_int_ptr2->get_value();
                 switch (instr->get_instr_type()) {
                     case Instruction::add:
                         return ConstantInt::get(c_value1 + c_value2, module_);
                     case Instruction::sub:
                         return ConstantInt::get(c_value1 - c_value2, module_);
                     case Instruction::mul:
                         return ConstantInt::get(c_value1 * c_value2, module_);
                     case Instruction::sdiv:
                         if (c_value2 != 0)
                             return ConstantInt::get((int)(c_value1 / c_value2),
                                                     module_);
                     case Instruction::cmp: {
                         auto cmp_instr = dynamic_cast<CmpInst*>(instr);
                         switch (cmp_instr->get_cmp_op()) {
                             case CmpInst::EQ:
                                 return ConstantInt::get(c_value1 == c_value2, module_);
                             case CmpInst::NE:
                                 return ConstantInt::get(c_value1 != c_value2, module_);
                             case CmpInst::GT:
                                 return ConstantInt::get(c_value1 > c_value2, module_);
                             case CmpInst::GE:
                                 return ConstantInt::get(c_value1 >= c_value2, module_);
                             case CmpInst::LT:
                                 return ConstantInt::get(c_value1 < c_value2, module_);
                             case CmpInst::LE:
                                 return ConstantInt::get(c_value1 <= c_value2, module_);
                             default:
                                 return nullptr;
                         }
                     }
                     default:
                         return nullptr;
                 }
             } else {
                 // both are float
                 auto constant_fp_ptr1 = dynamic_cast<ConstantFP*>(value1);
                 auto constant_fp_ptr2 = dynamic_cast<ConstantFP*>(value2);
                 float c_value1, c_value2;
                 c_value1 = constant_fp_ptr1->get_value();
                 c_value2 = constant_fp_ptr2->get_value();
                 switch (instr->get_instr_type()) {
                     case Instruction::fadd:
                         return ConstantFP::get(c_value1 + c_value2, module_);
                     case Instruction::fsub:
                         return ConstantFP::get(c_value1 - c_value2, module_);
                     case Instruction::fmul:
                         return ConstantFP::get(c_value1 * c_value2, module_);
                     case Instruction::fdiv:
                         if (c_value2 != 0.0)
                             return ConstantFP::get(c_value1 / c_value2, module_);
                     case Instruction::fcmp: {
                         auto cmp_instr = dynamic_cast<FCmpInst*>(instr);
                         switch (cmp_instr->get_cmp_op()) {
                             case FCmpInst::EQ:
                                 return ConstantInt::get(c_value1 == c_value2, module_);
                             case FCmpInst::NE:
                                 return ConstantInt::get(c_value1 != c_value2, module_);
                             case FCmpInst::GT:
                                 return ConstantInt::get(c_value1 > c_value2, module_);
                             case FCmpInst::GE:
                                 return ConstantInt::get(c_value1 >= c_value2, module_);
                             case FCmpInst::LT:
                                 return ConstantInt::get(c_value1 < c_value2, module_);
                             case FCmpInst::LE:
                                 return ConstantInt::get(c_value1 <= c_value2, module_);
                             default:
                                 return nullptr;
                         }
                     }
                     default:
                         return nullptr;
                 }
             }
         }
         ```
  7.  优化前后的 IR 对比（举一个例子）并辅以简单说明：
      源代码为

      ```c
      int a;
      float b;

      int test(void) {
          float c;
          int d;
          float e;
          a = 3;
          b = 3.14;
          c = a + b;
          d = 2 * a;
          e = c / d;
          if (e < c) {
              c = e * c;
          }
          return d;
      }

      int main(void) {
          test();
          return 0;
      }
      ```

      没开 ConstPropagation 时，生成的 IR

      ```llvm
      @a = global i32 zeroinitializer
      @b = global float zeroinitializer
      declare i32 @input()

      declare void @output(i32)

      declare void @outputFloat(float)

      declare void @neg_idx_except()

      define i32 @test() {
      label_entry:
        store i32 3, i32* @a
        store float 0x40091eb860000000, float* @b
        %op3 = load i32, i32* @a
        %op4 = load float, float* @b
        %op5 = sitofp i32 %op3 to float
        %op6 = fadd float %op5, %op4
        %op7 = load i32, i32* @a
        %op8 = mul i32 2, %op7
        %op11 = sitofp i32 %op8 to float
        %op12 = fdiv float %op6, %op11
        %op15 = fcmp ult float %op12,%op6
        %op16 = zext i1 %op15 to i32
        %op17 = icmp ne i32 %op16, 0
        br i1 %op17, label %label18, label %label22
      label18:        ; preds = %label_entry
        %op21 = fmul float %op12, %op6
        br label %label22
      label22:        ; preds = %label_entry, %label18
        %op24 = phi float [ %op6, %label_entry ], [ %op21,  %label18 ]
        ret i32 %op8
      }
      define i32 @main() {
      label_entry:
        %op0 = call i32 @test()
        ret i32 0
      }
      ```

      开了之后的生成的 IR

      ```llvm
      @a = global i32 zeroinitializer
      @b = global float zeroinitializer
      declare i32 @input()

      declare void @output(i32)

      declare void @outputFloat(float)

      declare void @neg_idx_except()

      define i32 @test() {
      label_entry:
        store i32 3, i32* @a
        store float 0x40091eb860000000, float* @b
        br label %label18
      label18:        ; preds = %label_entry%label_entry
        br label %label22
      label22:        ; preds = %label_entry, %label18
        %op24 = phi float [ 0x40188f5c40000000, %label_entry ], [ 0x40192210e0000000, %label18 ]
        ret i32 6
      }
      define i32 @main() {
      label_entry:
        %op0 = call i32 @test()
        ret i32 0
      }
      ```

      完成了以下常量传播

      1. 根据 a, b 的常量值，先将 a 转换为 float 类型(%op5)，再计算出 c 的值(%op6)，完成两次常量传播。
      2. 使用 a 的常量值，计算出 d 的常量值(%op8)，完成一次常量传播。
      3. 将 d 转换为 float 类型(%op11)，再计算出 e 的常量值(%op12)，完成两次常量传播。
      4. 对 c, e 进行比较，将结果拓展到 32 位，与 0 进行比较，确定分支跳转方向，由于均为常量，可以定值，最后生成一个无条件跳转指令`br label %label22`。

* 活跃变量分析
  对不同种类的指令进行分析，将变量加入到 use 集合和 def 集合中，并利用动态转换判断是否为常量(如下 store 指令操作)

  ```cpp
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

  对于 phi 指令，设计 phi_uses 结构进行记录，记录的值为变量和对应进入的 BasicBlock。

  ```cpp
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

  获取了每个 BasicBlock 的 use 集合和 def 集合后，按照书上的算法计算每个 BasicBlock 的 in 集合和 def 集合

  ```cpp
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

1. 了解到编译器如何利用 pass 进行代码优化。
2. 知道了 SSA 生成的大体流程，其核心是 phi 指令的生成。
3. 实现了常量传播，还有进一步优化的空间，比如全局变量块间传播，其关键是循环的处理，还有数组常量的传播等。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

无
