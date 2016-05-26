# Moses��Moses IRʾ��
---
## ��������
moses�ĺ����������´��룺
```
func add(parm : int) -> int
{
    return parm;
}
add(10);
```
��Ӧ��moses IR������ʾ��
```
define i32 @func(i32 %parm) 
{
    1% = alloca i32
    store i32 %parm, i32* %1
    %2 = load i32* %1
    ret i32 %2
}
%1 = call i32 @func(i32 10)
```


----------
## if-else
moses�е�if-elseԴ��ʾ����
```
var num = 0;
if (num == 0)
{
    num = 1;
}
else
{
    num = 0;
}
```
��Ӧ��moses IR����������ʾ��
```
%num = alloca i32
store i32 0, i32* %num
%1 = load i32* %num
2% = cmp eq i32 %2, 0
br i1 %2, label %3, label %4

; <label>:3
store i32 1, i32* %num
br label%5

; <label>:4
store i32 9, i32* %num
br label %5

; <label>:5
```
������moses IR�����п��Կ�����labelҲ�ǻ���뵽���ֵı���ģ�ֻ�ǲ�����moses IR��ʾ��д��label��������ע�͵���ʽչ�ֵģ���


----------
## while-loop
moses��while-loopʾ������������ʾ��
```
func add(parm : int) -> int
{
    var sum = 0;
    while parm > 0
    {
        sum += parm--;
    }
    return sum;
}
```
��Ӧ��moses IR�Ĵ���������ʾ��
```
define i32 @add(i32 %parm)
{
    %1 = alloca i32
    %sum = alloca i32
    store i32 %parm, i32* %1
    store i32 0, i32* %sum
    br label %2
    
; <label>:2
    %3 = load i32* %1
    %4 = cmp gt i32 %3, 0
    br i1 %4, label %5, label %10

; <label>:5
    %6 = load i32* %1
    %7 = add i32 %6, -1
    store i32 %7, i32* %1
    %8 = load i32* %sum
    %9 = add i32 %8, %6
    store i32 %9, i32* %sum
    br label %2

; <label>:10
    %11 = load i32* %sum
    ret i32 %11
}
```
������Ҳ���Կ���moses IR��������RISC�ļܹ���


----------
## Class Type
��moses�У�class�Ķ���������ʾ��
```
class Point
{
    var start : int;
    var end : int;
};

Point p;
p.start = 0;
p.end = 1;
```
��Ӧ��moses IR����������ʾ��
```
%Point = type {i32, i32}

%p = alloca %Point
%1 = getelementptr &Point* %p, i32 0, i32 0
store i32 0, i32* %1
%2 = getelementptr %Point* %p, i32 0, i32 1
store i32 1, i32* %2
```
class Type����Ƕ��class type��������´�����ʾ��
```
class XY
{
    var start : int;
    var end : int;
};
class Point
{
    var XY : xy;
    var height : int;
};
var p : Point;
p.xy.start = 0;
p.xy.end = 1;
n.height = 10;
```
��Ӧ��moses IR���´�����ʾ��
```
%Point = type { %XY, i32}
%XY = type { i32, i32 }

%n = alloca %Point
%1 = getelementptr %Point* %n, i32 0, i32 0
%2 = getelementptr %XY* %1, i32 0, i32 0
store i32 0, i32* %2
%3 = getelementptr %Point* %n, i32 0, i32 0
%4 = getelementptr %XY* %3, i32 0, i32 1
store i32 1, i32* %4
%5 = getelementptr %Point* %n, i32 0, i32 1
store i32 10, i32* %5
```
�� moses IR ���û��Զ������������� moses �е��������ͣ�����������Ҳ���Կ�����ʵֻҪ�ṹ�ϵȼۣ����ǿ����໥��ֵ�ģ�ע�⣺Ŀǰmoses��֧�ַ��ʿ��ƣ�������ڷ���Ȩ�޵Ļ�����Ҫ���Ƿ���Ȩ�ޣ������⣬**bool** ������moses IR����ͨ�� **i1** ��ʾ�ġ�


----------
## Anonymous-Type
��moses��Anonymoust-type��ʾ������������ʾ��
```
// ����һ��Anonymous-type�ı�����ͨ�������Ƶ���num������һ��ȷ�е�Anonymous-type
var num = {12, false};
// ʹ��{length, flag}��num�е�ֵ�������
var {length, flag} = num;
```
��moses�У���������ı�������const���͵ġ���Ӧ��moses IR����������ʾ��
```
%num = alloca {i32, i1}
%start = alloca constant i32
%end = alloca constant i32

;���������ַ�ʽ��
;(1) ʹ�� getelementptr + store
;(2) ʹ�� load + insertvalue + store
;������õ��ǵ�һ��
;�ڶ��ֵķ�ʽ��ֱ�ӽ� %num ���ڴ� load����ʱֵ�ϣ��޸���ʱֵ��Ȼ����װ�ػ�ȥ

%1 = getelementptr {i32, i1}* %num, i32 0, i32 0
store i32 12, i32* %1
%2 = getelementptr {i32, i1}* %num, i32 0, i32 1
store i32 0, i32* %2

;���������ַ�ʽ��
;��1��ʹ��getelementptr + load + store
;��2��ʹ��load + extractvalue + store
; ʹ��C����ģ���ʱ�򣬲��õ��ǵ�һ�֡���һ����ʵ�ǱȽϸ�Ч��
; ��Ϊload�����ǽ������ṹ��load��һ����ʱֵ�У�Ȼ��ʹ��extractvalue����ʱֵ
; ��ȡ��ָ�����ݣ�Ȼ����store�ص��ڴ档���ڶ��������load
%3 = getelementptr {i32, i1}* %num, i32 0, i32 0
%4 = load i32* %3
store i32 %4, i32* %start

%5 = getelementptr {i32, i1}* %num, i32 0, i32 1
%6 = load i32* %5
store i32 %6, i32* %end
```


----------
## ��������
������moses IR�ĺ�������ʱ����Ҫ̽��һ��LLVM IR�ж��ں�������ֵ����ơ�

### LLVM�������أ�����ֵ�󲻴���8�ֽڣ�
������һ��C���ԵĴ��롣
```
struct Point
{
    int start;
    int end;
};

struct Point add()
{
    struct Point p;
    p.start = 0;
    p.end = 1;
    return p;
}
```
�������Դ�������ǿ��Կ�������ֵp�Ĵ�СΪ���� *int* �Ĵ�С��ʹ�� *clang* ���б���õ���LLVM IR������ʾ��
```
%struct.Point = type { i32, i32 }

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i32(i8* nocapture, i8* nocapture readonly, i32, i32, i1) #1

; Function Attrs: nounwind
define i64 @add() #0 
{
    %1 = alloca %struct.Point, align 4
    %p = alloca %struct.Point, align 4
    
    %2 = getelementptr inbounds %struct.Point* %p, i32 0, i32 0
    store i32 0, i32* %2, align 4
    
    %3 = getelementptr inbounds %struct.Point* %p, i32 0, i32 1
    store i32 1, i32* %3, align 4
    
    %4 = bitcast %struct.Point* %1 to i8*
    %5 = bitcast %struct.Point* %p to i8*
    
    call void @llvm.memcpy.pi08.pi08.i32(i8* %4, i8* %5, i32 8, i32 4, i1 false)
    
    %6 = bitcast %struct.Point* %1 to i64*
    %7 = load i64* %6, align 1
    ret i64 %7
}
```
LLVM����������ֻ�ܷ��ص�ֵ����ʱֵ������ʹ�� *struct* ֻҪ������8�ֽڣ�������ת���� *i64* �ĵ�ֵ��Ȼ�󷵻ء�LLVM��֧����ԭʼ�ڴ��Ͻ���������ָ��ת����ԭ��δ֪��ԭʼ�ڴ治�����ٱ��õ���������������Ҫ�� *stack* ����һ���µ��ڴ�ռ䣬Ȼ��ԭʼ���ݿ�����ȥ�������ڴ��Ͻ���ָ��ת����ת����i64ָ�룬Ȼ�� *load* ����ʱֵ�У����շ��ء�

### LLVM�������أ�����8�ֽڣ����Ż��棩
��LLVM����ֵ����8�ֽڵ�ʱ�򣬻��� *caller stack frame* �з���һ����ʱ�ڴ棬�����ڴ��ַ��Ϊ�������ݸ� *callee*��Ȼ���� *callee* �н������ص�ֵʹ�����ú��� *llvm.memcpy()* ��������ʱ�ڴ��У�Ȼ�󷵻�void��ʾ������������ʾ��
```
struct XY
{
    int start;
    int mid;
    int end;
};

struct XY func()
{
    struct XY xy;
    xy.start = 0;
    xy.mid = 1;
    xy.end = 2;
    return xy;
}
```
��Ӧ��LLVM IR������ʾ��
```
%struct.XY = type {i32, i32, i32}

; Function Attrs: nounwind
define void @func(%struct.XY* noalias sret %agg.result) #0
{
    %xy = alloca %struct.XY, align 4
    
    %1 = getelementptr inbounds %struct.XY* %xy, i32 0, i32 0
    store i32 0, i32* %1, align 4
    
    %2 = getelementptr inbounds %struct.XY* %xy, i32 0, i32 1
    store i32 1, i32* %2, align 4
    
    %3 = getelementptr inbounds %struct.XY* %xy, i32 0, i32 2
    store i32 2, i32* %3, align 4
    
    %4 = bitcast %struct.XY* %agg.result to i8*
    %5 = bitcast %struct.XY* %xy to i8*
    
    call void @llvm.memcpy.pi08.pi08.i32(i8* %4, i8* %5, i32 12, i32 4, i1 false)
    ret void
}
```
��LLVM IR�п������Կ������޸��˺��� *signature* ���� *callee* �ڲ��������ص�ֵ������ *caller* ����ʱ�ڴ��С�

### LLVM�������أ�����8�ֽڣ��Ż��棩
��������ʹ��C++����д��Դ��������ʾ��
```
class A
{
public:
    int mem_1;
    int mem_2;
    int mem_3;
}

A func()
{
    A a;
    a.mem_1 = 10;
    a.mem_2 = 10;
    a.mem_3 = 10;
    return a;
}

int main()
{
    A a = func();
    return 0;
}
```
��Ӧ��moses IR�Ż��汾���£�
```
%class.A = type { i32, i32, i32 }

; Function Attrs: nounwind
define void @"\01?func@@YA?AVA@@XZ"(%class.A* noalias sret %agg.result) #0
{
    %1 = getelementptr inbounds %class.A* %agg.result, i32 0, i32 0
    store i32 10, i32* %1, align 4
    
    %2 = getelementptr inbounds %class.A* %agg.result, i32 0, i32 1
    store i32 10, i32* %2, align 4
    
    %3 = getelementptr inbounds %class.A* %agg.result, i32 0, i32 2
    store i32 10, i32* %3, align 4
    ret void
}

; Function Attrs: nounwind
define i32 @main() #0
{
    %1 = alloca i32, align 4
    %a = alloca %class.A, align 4
    store i32 0, i32* %1
    call void @"\01?func@@YA?AVA@@XZ"(%class.A* sret %a)
    ret i32 0
}
```
�� *main()* �����е��� *func()* ���������ڷ���ֵ�����ô�ͳ�� *eax* �� *ebx* �Ĵ��������ݣ������� *caller stack frame* �д�������ʱ�ڴ棬ֻ�������ʱ�ڴ���� *caller* �б����� *func()* ��ֵ�ı�������ʵһ�δ����ھ���Clang���뵽LLVM IR֮�󣬺�����������ʵ�Ѿ�����Ӧ�ĸ����ˣ��������ﺯ�� *func()* �� *signature* �Ѿ����ı��ˡ�
### moses IR��������
��ʵ[LLVM 2.3][1]���Ѿ�֧�ֶ�ֵ���أ�multiple return value, MRV���ˣ�����ϸ�ڲμ���ƪ[�ռ�][2]��

moses�����������������ض�ֵ��ʾ����������ͼ��ʾ��
```
func add(parm1 : int, parm2 : int) -> {int, int}
{
    return {parm1, parm2};
}
```
moses IR���ڶ�ֵ�ķ��أ����LLVM��ʵ�֣���С�ڵ���8�ֽڵ�����¿���ƴ��һ��i64��С��ֵ���޸ĺ��� *signature* Ϊi64��С���ڴ���8�ֽ�ʱ���� *caller stack frame* �д���һ����ʱ�ڴ�����Ȼ��ֵ�������������С� 
moses IR���ڷ��ض�ֵ��֧��������ʾ��
```
define {i32, i32} @add(i32 %parm1, i32 %parm2)
{
    %1 = load i32* %parm1
    %2 = load i32* %parm2
    �������� LLVM ����������
    ret { i32, i32 } { %1, %2 }
}
```
��IR�в�����ֱ�ӽ��ڴ��е�����ֵ���д��ݣ����� *load* ����ʱ������ʹ����ʱ��������ֵ�Ĵ��ݡ����������ǽ�����ֵ����Go���Զ�ֵ���صĻ���ʵ�֣�����ͼ��ʾ��

![�˴�����ͼƬ������][3]

moses���ڶ�ֵ���أ�MRV����Դ��������ʾ��
```
func add(parm1 : int, pamr2 : int) -> {int, int}
{
    return {parm1, parm2};
}
var {start, end} = add(10, 10);
```

��Ӧ��moses IRʵ�����´�����ʾ��
```
; �޸�function signature
define void @add(i32 %parm1, i32 %parm2, i32* %ret1, i32* %ret2)
{
    %1 = load i32* %parm1
    %2 = load i32* %parm2
    store %1 i32* %ret1
    store %2 i32* %ret2 
    ret void
}
; �ڲ����Ϸ����ٺ�һ����ʱ�ڴ棬Ȼ���޸� function signature ������ʱ�ڴ�ĵ�ַ���ݸ� add() ����

; Ϊstart, end����ջ���ڴ�
%start = alloca i32
%end = alloca i32

; Ϊ��ʱ��������ջ���ڴ�
%ret1 = alloca i32
%ret2 = alloca i32

; �޸�function signature
call void add(i32 10, i32 10, %32* ret1, %i32* ret2)

; ��ʱ����ֵ�Ѿ��ŵ� ret1 �� ret2λ������
; ������Ҫ�����ݿ����� %start %end��
%1 = load i32* %ret1
%2 = load i32* 
```
ע������Ĵ�����������target-code generation��ʱ�򣬿��ܻᰴ�շ���ֵ�Ĵ�С�������Ż���register�С�

���ڽṹ�����͵ķ��������ڶ�ֵ���أ�Ҳ���� *caller stack frame* �д���һ����ʱ�ڴ棬���������ʱ�ڴ���Ϊ��ת��ͬ���������ֵҲ�����Ż����Ĵ����У�����ʵ���Ҫ�������ñ�֤����Ϊ�Ժ�moses֧�ֶ� *translate unit* ��ʱ����Ҫ�� *caller* �� *callee* ��������Լ���ģ�calling convention����

�� LLVM ���� *First Class Value* �ĸ��ͬ����moses IRҲ�����ָ������ *First Class Value* ���������£�
> * Integer
> * Structure

��ʱֻ��������ֵ������Ϊ��������ֵ�����ַ���ο����� [LLVM 2.3][4]��


  [1]: http://llvm.org/releases/2.3/docs/ReleaseNotes.html
  [2]: http://nondot.org/sabre/LLVMNotes/MultipleReturnValues.txt
  [3]: https://tiancaiamao.gitbooks.io/go-internals/content/zh/images/3.2.funcall.jpg
  [4]: http://nondot.org/sabre/LLVMNotes/FirstClassAggregates.txt