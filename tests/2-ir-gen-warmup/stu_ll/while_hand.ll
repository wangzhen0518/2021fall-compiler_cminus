; ModuleID = 'while.c'
source_filename = "while.c"

define i32 @main() {
    ; default return value 0
    %1 = alloca i32
    ; alloc a, i
    %2 = alloca i32 ; a
    %3 = alloca i32 ; i
    store i32 0, i32* %1
    store i32 10, i32* %2 ; a
    store i32 0, i32* %3 ; i
    br label %4

4: ; preds = %0, %7
    %5 = load i32, i32* %3
    %6 = icmp slt i32 %5, 10
    br i1 %6, label %7, label %12

7: ; preds = %4
    ; i = i + 1
    %8 = load i32, i32* %3
    %9 = add i32 %8, 1
    store i32 %9, i32* %3
    ; a = a + i
    %10 = load i32, i32* %2
    %11 = add i32 %10, %9
    store i32 %11, i32* %2
    br label %4

12: ; preds = %4
    %13 = load i32, i32* %2
    ret i32 %13
}