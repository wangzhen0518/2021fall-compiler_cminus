; ModuleID = 'assign.c'
source_filename = "assign.c"

; main Function
define i32 @main() {
    %1 = alloca [10 x i32]
    ; get a[0]
    %2 = getelementptr inbounds [10 x i32], [10 x i32]*  %1, i32 0, i32 0
    ; store a[0]
    store i32 10, i32* %2
    ; get a[1]
    %3 = getelementptr inbounds [10 x i32], [10 x i32]*  %1, i32 0, i32 1
    ; calculate a[1]
    %4 = load i32, i32* %2
    %5 = mul i32 %4, 2
    ; store a[1]
    store i32 %5, i32* %3
    ; get a[1]
    %6 = load i32, i32* %3
    ; return
    ret i32 %6
}
