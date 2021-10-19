; ModuleID = 'fun.c'
source_filename = "fun.c"

define i32 @callee(i32 %0){
    %2 = alloca i32
    store i32 %0, i32* %2
    %3 = load i32, i32* %2
    %4 = mul i32 2, %3
    ret i32 %4
}

define i32 @main(){
    ; store 110
    %1 = alloca i32
    store i32 110, i32* %1
    %2 = load i32, i32* %1
    ; call function callee
    %3 = call i32 @callee(i32 %2)
    ; return
    ret i32 %3
}
