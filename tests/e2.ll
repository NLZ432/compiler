; ModuleID = 'WPLC.ll'
source_filename = "WPLC.ll"

@0 = private unnamed_addr constant [9 x i8] c"%d - %s\0A\00", align 1
@1 = private unnamed_addr constant [13 x i8] c"Go to sleep!\00", align 1
@2 = private unnamed_addr constant [9 x i8] c"Wake up!\00", align 1

declare i8* @printf(...)

declare i8* @printf.1()

define void @printHourInfo(i32 %0, i8* %1) {
entry:
  %x = alloca i32, align 4
  store i32 %0, i32* %x, align 4
  %s = alloca i8*, align 8
  store i8* %1, i8** %s, align 8
  %x1 = load i32, i32* %x, align 4
  %s2 = load i8*, i8** %s, align 8
  %2 = call i8* (...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @0, i32 0, i32 0), i32 %x1, i8* %s2)
  ret void
}

define i1 @isbedtime(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, i32* %n, align 4
  %ret = alloca i1, align 1
  store i1 false, i1* %ret, align 1
  %n1 = load i32, i32* %n, align 4
  %1 = icmp slt i32 %n1, 7
  br i1 %1, label %selectbloc, label %condbloc

selectbloc:                                       ; preds = %entry
  store i1 true, i1* %ret, align 1
  br label %continue

condbloc:                                         ; preds = %entry
  %n4 = load i32, i32* %n, align 4
  %2 = icmp sge i32 %n4, 7
  %n5 = load i32, i32* %n, align 4
  %3 = icmp slt i32 %n5, 23
  %4 = and i1 %2, %3
  br i1 %4, label %selectbloc2, label %condbloc3

selectbloc2:                                      ; preds = %condbloc
  store i1 false, i1* %ret, align 1
  br label %continue

condbloc3:                                        ; preds = %condbloc
  %n8 = load i32, i32* %n, align 4
  %5 = icmp eq i32 %n8, 23
  %n9 = load i32, i32* %n, align 4
  %6 = icmp eq i32 %n9, 24
  %7 = or i1 %5, %6
  br i1 %7, label %selectbloc6, label %condbloc7

selectbloc6:                                      ; preds = %condbloc3
  store i1 true, i1* %ret, align 1
  br label %continue

condbloc7:                                        ; preds = %condbloc3
  br label %continue

continue:                                         ; preds = %selectbloc6, %selectbloc2, %selectbloc, %condbloc7
  %ret10 = load i1, i1* %ret, align 1
  ret i1 %ret10
}

define i32 @program() {
entry:
  %hour = alloca i32, align 4
  store i32 0, i32* %hour, align 4
  br label %condbloc

condbloc:                                         ; preds = %bContinue, %entry
  %hour1 = load i32, i32* %hour, align 4
  %0 = icmp sle i32 %hour1, 24
  br i1 %0, label %loopbloc, label %continuebloc

loopbloc:                                         ; preds = %condbloc
  %hour2 = load i32, i32* %hour, align 4
  %1 = call i1 @isbedtime(i32 %hour2)
  br i1 %1, label %truebloc, label %falsebloc

continuebloc:                                     ; preds = %condbloc
  ret i32 0

truebloc:                                         ; preds = %loopbloc
  %hour3 = load i32, i32* %hour, align 4
  call void @printHourInfo(i32 %hour3, i8* getelementptr inbounds ([13 x i8], [13 x i8]* @1, i32 0, i32 0))
  br label %bContinue

falsebloc:                                        ; preds = %loopbloc
  %hour4 = load i32, i32* %hour, align 4
  call void @printHourInfo(i32 %hour4, i8* getelementptr inbounds ([9 x i8], [9 x i8]* @2, i32 0, i32 0))
  br label %bContinue

bContinue:                                        ; preds = %falsebloc, %truebloc
  %hour5 = load i32, i32* %hour, align 4
  %2 = add nsw i32 %hour5, 1
  store i32 %2, i32* %hour, align 4
  br label %condbloc
}
