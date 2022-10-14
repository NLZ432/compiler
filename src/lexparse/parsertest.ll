; ModuleID = 'WPLC.ll'
source_filename = "WPLC.ll"

@0 = private unnamed_addr constant [4 x i8] c"HI\0A\00", align 1
@1 = private unnamed_addr constant [5 x i8] c"HI2\0A\00", align 1

declare i8* @printf(...)

declare i8* @printf.1()

define i32 @main(i32 %0, i8** %1) {
entry:
  %i = alloca i32, align 4
  store i32 3, i32* %i, align 4
  %i1 = load i32, i32* %i, align 4
  %2 = add nsw i32 %i1, 2
  store i32 %2, i32* %i, align 4
  br i1 false, label %selectbloc, label %condbloc

selectbloc:                                       ; preds = %entry
  %3 = call i8* (...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @0, i32 0, i32 0))
  br label %continue

condbloc:                                         ; preds = %entry
  br i1 true, label %selectbloc2, label %condbloc3

selectbloc2:                                      ; preds = %condbloc
  %4 = call i8* (...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @1, i32 0, i32 0))
  br label %continue

condbloc3:                                        ; preds = %condbloc
  br label %continue

continue:                                         ; preds = %selectbloc2, %selectbloc, %condbloc3
  %i4 = load i32, i32* %i, align 4
  %i5 = load i32, i32* %i, align 4
  %5 = add nsw i32 %i4, %i5
  store i32 %5, i32* %i, align 4
  ret i32 0
}
