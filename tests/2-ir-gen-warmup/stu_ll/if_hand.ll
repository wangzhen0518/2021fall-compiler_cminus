; ModuleID = 'if.c'
source_filename = "if.c"

define i32 @main() {
    ; %1 seems to be the default return value?
    %1 = alloca i32
    store i32 0, i32* %1
    ; store 5.555 in a
    %2 = alloca float
    store float 0x40163851E0000000, float* %2
    ; branch
    %3 = alloca i32 ; use %3 to store the reture value
    %4 = load float, float* %2
    %5 = fcmp ugt float %4, 1.0
    br i1 %5, label %6, label %7

6: ; preds = %0
    store i32 233, i32* %3
    br label %8

7: ; preds = %0
    store i32 0, i32* %3
    br label %8

8: ; preds = %6, %7
    %9 = load i32, i32* %3
    ret i32 %9
}