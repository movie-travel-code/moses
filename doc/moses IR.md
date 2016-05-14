# moses IR

------

moses IR�Ƕ�LLVM IR�ļ򻯣�����moses�����������գ�������Ҫ��IR�м�¼�����������յ���Ϣ�������ֽ׶�mosesֻ�������������� **int** �� **bool** ��û��pointer��û��Array�ȸ������ͣ�����IR���ֻȡLLVM IR��һ���Ӽ����ɡ�����ȡ��LLVM IR��һ���Ӽ�������moses IR��ȻҲ��SSA-based���м��ʾ��ʽ�����ǲ�ȡLLVM��ȡ�ķ��������м�������ɵ�ʱ��û��ֱ�����ɱ�׼SSA��ʽ�����Ǻ���ͨ��һ�� **mem2reg** �Ĺ��̽���SSA�Ĵ���promote��ΪSSA��ʽ�Ĵ��롣

> * ����
> * Identifiers
> * High Level Structure
> * Structure Types

------

## ����

moses IR������LLVM IRһ���е���moses����Ҫ�Ľ�ɫ����Ϊmosesʵ�ֵ���ҪĿ�ľ�����Ϥ�����и��ָı䣬����moses�ᾡ���ܵ�Ӧ�ø��ֱ����Ż��������LLVM IR�Ǹ���Ĵ�����ʽ���м��ʾ�����Է���������ʵ�ֺܶ����Ż����

�������Ȼ����AST���ɽ�SSA��ʽ���м��ʾ��Ȼ������ΪSSA��moses IR��Ȼ��ʵ��һ���������������Щbyte code����ͨ������������������յĲ������������ǻ᳢��ֱ�����ɶ����ƣ�������������������ִ���ļ��У����Ƕ����������գ��һ����Ǻܾ�ͨ����������ʵ�ֵ�һ�����м�������ɣ�Ȼ�󹹽�һ���������ʵ�ּ򵥵��������ա�

***Note:��ʵ��moses����û�б�Ҫ����AST�ģ�moses̫�򵥣�����ʹ��SDT�ڲ�����AST������½���code generation.***


----------


## Identifiers
moses IR��identifier��Ҫ�������������Σ�global��local��Gloabl identifiers(functions, global variables, class) ���ַ�'@' ��ͷ. Local identifiers(register names, typs) ���ַ�'%'��ͷ�����⣬����identifiers��˵�����ֲ�ͬ�ĸ�ʽ���ֱ����ڲ�ͬ��Ŀ��:

> * ����ֵ��named values���ɴ����ض�ǰ׺���ַ�����ʾ�����磺%foo, @DivisionByZero, %a.really.long.identifier. 
> * ����ֵ��unamed values���ɴ����ض�ǰ׺��һ���޷�������ʾ�����磺%12, @2, %44.
> * ����

moses IRҪ��identifier����ǰ׺��ԭ��������LLVM��һ�����ֹ�ͱ����ֵ����ֳ�ͻ������������Ժ���Ҫ����Ļ��᷽��һЩ����һ���棬��������ֵ��unamed identifiers����˵���������ڴ����µ���ʱ������ʱ�򣬲���Ҫ���Ƿ��ű��еĳ�ͻ���⡣

moses IR�еı����֣�reserved words���ο�LLVM IR��������������'add', 'ret'�ȣ���������������'void', 'i32'�ȣ��Լ����������֡�����identifier��'@'��'%'��ͷ������identifier����ͱ����ֳ�ͻ��

������moses IR���ڱ��� '%x' ����8��ʾ�����룺

�򵥵ķ�ʽ��
```
%result = mul i32 %x, 8
```

ǿ�������ķ�ʽ��
```
%result = shl i32 %x, 3
```

��ӵķ�ʽ��
```
%0 = add i32 %x, %x
%1 = add i32 %0, %0
%result = add i32 %1, %1
```

�����һ�ַ�ʽ���Կ�����
> * ע����';'��ʼֱ����β��
> * ��������û�и�ֵ������������named values��ʱ����Ҫ�����µ�������ʱ������
> * ������ʱ����ͨ���������������б�š�ע�⣬������ڶ��������Ļ���������Ҳ������š�


----------
## High Level Structure
moses����ƻ��Ƚϼ�ª����֧�ַ�����룬���ɲ�֧��LLVM IR�е�Module Type��Linkage Type��

moses IR�������ֵ��ù�����һ������ͨ�ģ�һ���ǿ��ٵĵ��ù�����
���ù�����������caller��callee���Э������Ҫ�����������ݷ�ʽ��˳�򡢷���ֵ���ݡ�ջ������

 1. �����Ĵ��ݷ�ʽ
 �󲿷�ʱ��x86����ʹ��ջ���ݲ���������ǰcaler pushʵ�Σ�callee����ʱ����ָ֡��Ϊ�ο���ַ��ջ��ȡ��ʵ��ֵ��������Щ��������Ż�ʹ�üĴ������ݲ�������������ܡ�����caller��callee���ڲ����Ĵ��ݱ���Э�̺á�

 2. �����Ĵ���˳��
 caller������ѹ��ջ�У�callee�ٴ�ջ��ȡ�����������ڶ�����ĺ������ã������ǰ��ա��������ҡ����ǡ��������󡱵�˳����ջ��caller��callee����Э�̺á�

 3. ����ֵ�Ĵ���
 ���˲����Ĵ���֮�⣬caller��callee�Ľ�������һ����Ҫ�ķ�ʽ���Ƿ���ֵ��callee�ķ���ֵ��ô����Ҳ��ҪЭ�̺õģ�����һ�������x86�£��򵥵ķ���ֵһ���Ǵ洢�� **eax** �Ĵ����з��ء����ǱϾ� **eax** �������ޣ�������ֵ����4�ֽڣ�5 ~ 8�ֽڣ���ʱ��ʹ�� **edx** ��**eax**���Ϸ��أ�eax�Ĵ������ص�4�ֽڣ�edx���ظ�4�ֽڡ����ڷ��ض��󳬹�8�ֽڵķ������ͣ���ʹ��callerջ�ռ��ڴ�������Ϊ��ת����callee����ǰ������ֵ�Ķ���calleeջ�ϵ���ʱ���������꼴���٣�������callerջ�ռ��С�Ȼ�󷵻ص�caller�У��ٽ���ʱֵ��������λ�á�

 4. ջƽ��
 callee����ʱ��һ�������Լ���ջ�ռ䣬����**���ݵĲ�����Ϊ�������֣�����caller����callee��������ջָ�뵽����ǰ��״̬�����ղ����ռ䣬��ά��ջƽ�⣩**��˫��Ҳ������һ��Լ����
������һ�������ϸ˵����[x86�µ�C�������ù���][1]�����ں���ջ֡����ͼ��ʾ��
![�˴�����ͼƬ������][2]

����moses��˵�����ù��������Ҫ���ǵ�����4�����档moses�������ֵ��ù�����**ccc** �� **fastcc**��

moses�����������´�����ʾ��
```
class base
{
    var start : int;
    var end : int;
};

func add( parm : base ) -> {int, int}
{
    return { parm.start, parm.end };
}

var arg : base;
arg.start = 0;
arg.start = 10;
// num ��һ��������������ڽ���add�ķ���ֵ
var num = add(arg);
```
**moses�����������ͺ���������(anonymous type)����ֵ���壬�����û��Զ�������(class)������������**�����Զ��ڲ���������˵ֻ���������ͣ� **��������(int, bool)** �Լ� **��������(anonymous type)** ��ֵ�Լ��Զ������͵ĵ�ַ��**��������(int, bool)**��ֵ������ʹ��32λֵ����ʾ��Ҳ����˵ **eax** �Ĵ�����ȫ��������Ҫ�󡣶��� **��������(anonymous type)** �Ĳ������ݣ����Բ���**��������(int, bool)**�ķ�ʽ��ʵ�֣����� **{int, int}**�����Կ�������int���͵Ĳ������������ **target architecture**�Ĵ�����������Ļ���ֻ��ʹ��ջ�����в����Ĵ����ˡ�

���Ƕ��ڷ���ֵ��˵�ͱȽ��ر��ˣ�����**��������(int, bool)**��**�û��Զ�������(class)**��Ȼ����ʹ�� **eax** �����з���ֵ�Ĵ��ݣ����Ƕ���**��������(anonymous type)**��˵�Ͳ���ʹ�üĴ��������з����ˡ�����**��������(anonymous type)**�ķ��أ����Բ��� [Go][3] ��ʵ�֡�

��ֵ������ʵ����һ�����﷨���¡���C/C++�еĶ�ֵ���أ�����ͨ��struct��class������а�װ�����ء���C/C++�е�ʵ�֣�����callerջ֡�д���һ����ʱ������Ϊ����תվ����ʵ�֡�moses�з���ֵ��ʵ�ֲο�Go�Ķ�ֵ���ص�ʵ�֡�Go���������ڴ���Ĳ���֮������������λ��������ֱ�ӽ�����ֵ����������λ������f����ǰ���ڴ沼�����£�
```
Ϊret2������λ
Ϊret1������λ
����3
����2
����1 <- sp
```
����ͼ��ʾ��
![�˴�����ͼƬ������][4]

����moses���ǲ��ո÷�ʽʵ�֡�

**ccc** ���ù��������Ǵ�ͳ�ĵ��ù�������������ѹ�Σ�ʹ��ջ�����ݲ�����ʹ��ջ������ֵ�ķ��أ� caller���������ջ�ռ�Ļ��ա�

**fastcc** ���ù�����������ù��������ܵ�ʹ��ε��ñ�ø��졣���磬�����ͷ���ֵ����ͨ���Ĵ��������ݡ�


----------
## Structure Types
moses������Ƶ�ʱ�򣬶���ÿ�����Ͷ��� **Type fingerprint** �ĸ��mosesͨ��type fingerprint��ʵ�ֽṹ���͵ȼۡ�
```
class type1
{
    var num : int;
    var flag : bool;
};

class type2
{
    var mem : int;
    var flag : bool;
};
// type1��type2��type fingerprint��ͬ
// �������͵ı��������໥��ֵ
```
ֻҪtype fingerprint��ͬ�Ϳ����໥��ֵ��������һ��������anonymous type variableֻ�ܸ�ֵ��class type variable��Type Checking�����������ʱ����Ѿ���ɡ�

��ˣ�moses IR�������������ͣ�"identified" �� "literal"������"literal"���Ϳ����ڽṹ��Ψһȷ����Ҳ����˵��Դ���г��ֵĶദ **"{int, int}"** ���ǵȼ۵ģ�����Ҳ�������������������ͬ�ṹ���������ͻ�ͳһָ������һ���������͡���Ȼmoses���ýṹ���͵ȼۣ�����"identified"����ֻ��ͨ��name��Ψһȷ��������class type���Եȼ۵���ͬ����

�������Ͷ�Ӧ��"iteral"���͵��������£�
```
%mytype = anonytype { int, int }
```

һ��"identified"���͵��������£�
```
%mytype = type { int, int }
```
**Note: ��moses���������ʵһֱ�ڻر�һ�����⣬��������Ƕ�ף����磺**
```
class type
{
    var num : int;
    var next : type;
};
```
**�ⷽ���漰�������ָ�������������ʱ���漰��**


----------
## Global Variables
moses������Global Variable�����ʼ��������moses IR����������������

LLVM IR��������C/C++��const�ĸ������const�����ʼ����Ȼ����治���ٽ��иĶ�������м��const�������г�ʼ���Ͳ�������const������**���ɾ�Ȼ��"as there is a store to the variable"**�����෴��moses IR����const���������м��ʼ�������ǳ�ʼ�����const�����Ͳ����ڽ��и�ֵ�ˡ�������������������������ʱ����ġ�

����moses IR������LLVM IR��һ��"trick"�����ǿ��Խ�����const�ı������const�������������û�ж�������ٸ�ֵ�Ļ������ǾͿ��Խ��ñ�����Ϊconst���������Ż���

��ΪSSA values, global variables�ᶨ��һ��ָ��ֵ����ֵ��dominate���������scope������ͬ����������global variable������ָ��ֵָ��global variable content. ͬʱ�������ȫ�ֱ�������ͬ��**"intializer"**�Ļ������Ժϲ���

Syntax:

    @<GlobalVarName> = <global | constant> <Type> [<InitializerConstant>] [, align[Alignment]]

���磺
```
@G = constant int 1, align 4
```


----------
## Functions
moses IR���ں���������Ҫʹ�� **"func"** �ؼ��֣���ѡ��**"calling convention"**�� �������ͣ�return type���� ��������function name������ѡ��"garbage collector name"������moses�Լ���д����������ģ�飬����ָ���ı�Ҫ���������һ�Ի����ţ�{}�м�������Ǻ����壩��

һ�������������һ������飬������ɸú�����CFG(Control Flow Graph)������moses IR�ο�LLVM IR������Ҳ����IR�������ĺ���CFG��ÿһ����������һ��label��ͷ��label��ѡ�������label����ǰ�����鶨����һ�����ű�symbol table entry���һ����������һ��ָ����ɣ���һ��terminatorָ���β������branch��function return�������û�и������鶨����ʾ��label����ô����ʽ���ṩһ������label����������ʱ����ʹ�õ����ֱ����һ��ġ����磬���������ڻ����飨entry block��û���ṩ��ʾ��label����ô��Ϊ����� **"%0" label**����ô��� �������е�һ��������ʱ�����ı�ž��� **%1**��

�����еĵ�һ����������ڽ��뺯��ʱ����ִ�У�����Ҳ��������ǰ�������飨predecessor basic blocks����ͬʱ����û��ǰ�������飬��������ڻ������п϶�û�� **PHI nodes**��

Sytax:
```
func <ResultType> @<FunctionName> ([argument list]) { ... }
```


----------

## Parameter Attributes
LLVM IR�Ƚ�����˼��һ����Parameter Attributes��moses IRҲ���ȡ��������Ȥ�Ĳ��֡�

paramter attributes�Ǻ������е�һ�����ԣ����Ǻͺ��������޹أ������ڽ����﷨�����������ʱ��ȡ���Ķ�Ϊ��Ϣ���������к������Ż����β����ԣ�**"parameter attributes"**����һЩ�ؼ��֣����һ���β��ж�����ԣ��м��Կո������

���磺
```
func i32 @add(i32 returned, ) {}
```
��ǰ��moses IR��ʱֻ֧��һ���β����ԣ�����������չ����
> * returned 
������Ա�ʾ�������κ�����¶��Ὣ���Ӧ��ʵ����Ϊ���ء�

***Note: ��ʵ function Ҳ�����Ӧ�� attribute �����纯���Ƿ�inline���Ƿ���Ҫ�Ż�code size��LLVM IR��Ϊһ��ͨ�õ��м����ԣ�Ҳ�ṩ��codeѡ������ú���"rarely called"�ȵȡ�ֻ��moses��ʱ����Ҫ��ô�����ԡ�***


----------
## Function Attributes
**"Function Attributes"**��Ҫ�����ں����ϸ��Ӷ������Ϣ��������"parameter attribute", "Function Attributes"���ں�����һ���֣����ǲ�����"Function Type"��һ���֡�

���������Ǹ��ں����β��б����һ��ؼ��֡����ָ������ؼ��֣��м��Կո������

���磺
```
func void @f() optsize cold {}
```
> * cold
������Ա����ú������ٱ����á�

**Note: When computing edge weights, basic blocks post-dominated by a cold function call are also cosiderd to be code; and, thus, given low weight.**
> * post-dominate
A node *z* is said to post-dominate a node *n* if all paths to the exit node of the graph starting at *n* must go through z.  -[Dominator (graph theory)][5]

�����ע����������LLVM IR��һ�仰������˵"cold function" post-dominate �Ļ�����ᱻ����ϵ͵�Ȩ�ء�

> * minsize
������Խ����Ż� *pass* �ʹ������� *pass* �ᾡ���� *code size* ������ͣ���ʹӰ�쵽�����������ܡ�

> * norecurse
����������Ա����ú������κ�·���ж�����ֱ�ӻ��ӵĵ�������

> * optsize
������Խ����Ż� *pass* �ʹ������� *pass* ����ʹ�ú��� *code size* ������ͣ������ڽ��� *code size* ��ͬʱ����Ӱ��������ܡ�

> * readnone
������Ա�������ֻͨ��ʵ�������㷵�ؽ����Ҳ���ǻ�ú����ں������в��� *access* ��������� *state*��ͬʱ����ͨ��ָ��ʵ�Σ��û��Զ�������ʵ�Σ�ֱд���ݡ�

> * readonly
�����Ա����ú�������ͨ��ָ��ʵ�Σ��û��Զ�������ʵ�Σ�ֱд���ݣ�����ֻ�������ⲿ״̬��

> * argmemonly
�����Ա�������ֻͨ��ָ��ʵ�ν��зô������


----------

## Type System
moses IR������ϵͳ�ο�LLVM IR���������moses���Ե����ԡ�һ������������ϵͳ������LLVM IR��ʵ�ֺܶ��Ż������ҿɶ��Ը�ǿ��
### Void Type
void���Ͳ���ʾ�κ�ֵ���Ҳ�ռ�ÿռ䡣
Syntax:
```
void
```
### Function Type
���������൱�ں���ǩ��������moses��֧�ֺ���ָ�롢�������أ����Ժ���������moses��û��ʲô��;��

### First Class Type
First Class Type����˵�� moses IR ������Ҫ��һ���֣���Ϊ First Class Type ����� User Defined Type ��˵���ӽ����ײ㡱��

#### Single Value Types
��Щ���͵�ֵ�ڴ����������ĽǶ���������ֱ���ڼĴ����б�ʾ�ģ�����i32�������Ϳ��Ժ����׵��Ż����Ĵ����С�

##### Integer Types
integer��������򵥵����ͣ�����moses��ʱֻ֧��32λint������integer���;���i32������ʹ��i1type��ʾbool���ͣ�����bool���͵ı�ʾ���������ǻ���ϸ���ܵ���
**syntax**:
```
i32
```

#### Structure Type
**Overview**:
structure ���ͱ�ʾ���ڴ��е�һ�����ݵľۺϣ�ֱ�Ӷ�Ӧ�� moses �е� **��������** �� **�û��Զ�������**��

���ڴ��еĽṹ��������Ҫʹ�� **load** �� **store** ָ���ȡ�����⣬ structure �� element ��Ҫͨ�� **getelementptr** ָ���ȡ���ַ���ڼĴ����е� structure ������Ҫͨ�� **extractvalue** �� **insertvalue** ָ���ȡ��

�������ǽ��ܹ���"literal" structur��Ӧ��moses���������ͣ�����������λ�ö��塣�����û��Զ������ͣ�class type��ֻ����top-level���壬�����ж�Ӧ����������

**Note:moses��δ�����ܻ�ʵ�ֽṹ�����ż������ü�����ͨ�����������ڲ���Ա��ռ�ø�С���ڴ�ռ䡣LLVM IR����һ��packed�ĸ�����������ڲ�û��padding�������Ļ�ͬ������ռ�ø��ٵ��ڴ�ռ䡣**

**Syntax:**
```
%mytype = type { i32, i32 }
%atype = anonytype {i32, i32}
```


----------


## Constants
moses IR��LLVM IR����constant�Ĳ��ֽ����˾��򣬰������¼���constant��

### Simple Constants
**Boolean constants**
�ַ���'true'��'false'���� i1 ������˵������Ч�ĳ����ִ���

**Integer constants**
moses��������ֻ��int���ͣ���i32��ʾ��

### Complex Constants
�������Ǽ򵥳���(simple constants)�ĺ͸�С�ĸ��ӳ���(complex constants)����ϡ�

**Structure constants**
�ṹ�峣���ͽṹ�����͵ı�ʾ�����ƣ����磺"{i32 4, i1 false}"����ʵ�ṹ�峣�����������͵ĳ�ʼ�����ʽ�����ơ�


----------
## Global Variable and Function Addresses
moses IR�й���ȫ�ֱ����ͺ����ĵ�ַ������Ч�ģ���Ϊ������ȫ�ֱ������ߵ��ú���ʱ����ͨ�����ߵĵ�ַʵ�ֵġ�
```
@X = global i32 17
@Y = global i32 42
```


----------
## Metadata
LLVM IR�зǳ���Ȥ��һ���־����ṩ��Metadata���ݣ���������Я��һ�������������������һЩ��Ϣ�������֧���Ϳ���Я��һ�� "unpredictable" Metadata������������Ϣ��mose�п��ܼ�����������ʱ�������������


----------
## Instruction Reference
moses IR�������ֲ�ͬ��ָ�����ͣ� *terminator instructions*,  *binary instrucions*, *memory instructions* ������ָ�����͡�

### Terminator Instructions
���������ᵽ���������е�ÿһ�������鶼�� "Terminator" ��β����ʾ��ǰ���������֮��Ӧ��ִ���ĸ������顣��Щ "terminator instructions"�򵥵ز���һ�� 'void' ֵ����Щָ���������**������(control flow)**��������ֵ��

��moses���ս�ָ����������'ret', 'br'.
#### ret instructions

**ret syntax**:
```
ret <type> <value>  ; Return a value from a non-void funcion
ret void            ; Return from void function
```
'ret'ָ�����ڽ�������������һ����ѡֵ���ӵ�ǰ�������ص�caller����������ʽ��'ret'ָ�һ���Ƿ���һ�� *value* ����ɿ�������ת�ƣ���һ����ʽ���Ǽ򵥵���ɿ�������ת�ơ�

**'ret' Arguments**
'ret'ָ������һ����ѡ�ĵ�ʵ�Σ�һ������ֵ��

���磺
```
ret i32 5                       ; Return an integer value of 5
ret void                        ; Return from a void function
ret {i32, i1} {i32 10, i1 true} ; Return a struct of values 4 and true
```

#### 'br' Instruction

**br syntax**:
```
br i1 <cond>, label <iftrue>, label <iffalse>
br label <dest>         ; Unconditional branch
```

'br'ָ�������ں����ڲ���ɿ�������ת�ƣ���һ��������ת�Ƶ���һ�������顣������������ʽ��ָ�һ����������תָ�һ������������תָ�

**br arguments**
������תָ������һ�� 'i1' ���͵�ʵ�κ����� 'label' ����ֵ����������תָ��ֻ����һ�� 'label' ֵ��Ϊ��ת��ַ��

**br semantic**
�� 'br' ��תָ��ִ��֮ǰ����� 'i1' ���͵�ʵ�ν�����ֵ�����ֵΪ�棨true���������������ת�� 'iflabel' ʵ�Ρ���� "cond" Ϊ�٣�false���������������ת�� 'iffalse' labelʵ�Ρ�

���磺
```
Test:
%cond = icmp eq i32 %a, %b
br i1 %cond, label %IfEqual, label %IfUnequal
IfEqual:
ret i32 1
IfUnequal:
ret i32 0
```

### Binary Operations
��Ԫ����������ǳ�����ʹ�õ������������Binary Operation ��Ҫ������ͬ���͵Ĳ�������ִ����������󣬵õ�һ����ֵ������moses������ϵͳ���ܼ򵥣���ʱֻ֧�� *int* ���͵���ӡ�

#### 'add' Instruction

**add syntax:**
```
<result> = add <type> <op1>, <op2>
```
������������п��ܻ������moses��ʱ�������������س�����

#### 'sub Instruction'

**sub syntax:**
```
<result> = sub <type> <op1>, <op2>
```

#### 'mul' Instruction

**mul syntax**:
```
<result> = mul <type> <op1>, <op2>
```

#### 'div' Instruction

**div syntax**:
```
<result> = div <type> <op1>, <op2>
```

#### 'rem' Instruction

**rem syntax**:
```
<result> = rem <type> <op1>, <op2>
```

#### 'and' Instruction

**and syntax:**
```
<result> = and <type> <op1>, <op2>
```

#### 'or' Instruction

**or Instruction:**
```
<result> = or <type> <op1>, <op2>
```

----------
### Aggregate Operations
moses IR֧�ּ��ֶԾۺ����Ͳ�����ָ�

#### 'extractvalue' Instruction
**extractvalue syntax:**
```
<result> = extractvalue <aggregate type> <val>, <idx>{, <idx>}*
```

'extractvalue'ָ���ܹ��ۺ����ͣ��ۺ�������ʱֻ�����ṹ�����ͣ���ȡ�� member ��
 
 **extractvalue arguments:**
 'extractvalue'ָ��ĵ�һ���������ǽṹ�����ͣ���һ���ֲ��������±곣������Щ�±�������ʾ�ṹ���ĳ���ֳ�Ա��'extractvalue'ָ���'getelementptr'���ơ�

���ߵ��������ڣ�
> * ��Ϊ 'extractvalue' ָ���ȡ����ֵ�����Ե�һ���±��ʡ���ˣ�Ĭ��Ϊ0
> * ������һ��index

��Ϊmoses�е��������ͣ��ǲ�������̬��չ�ġ�����LLVM IR�ж�Ӧ 'insertvalue' ��moses�в����ڵġ�

### Memory Access and Addressing Operations
һ������ SSA-based IR ��α�ʾ�ڴ�ʱ����Ҫ��һ�㣬��LLVM�У��ڴ治��SSA��ʽ�ģ��������м�������ɵ�ʱ��Ƚϼ򵥡���һ���ֽ��������moses IR�� read, write, �� allocate �ڴ档

#### 'alloca' Instruction

**alloca syntax:**
```
<result> = alloca <type> [, <ty> <NumElements>]
```

'alloca'ָ����ڵ�ǰ���к�����ջ֡�Ϸ���һ���ڴ棬���ڸú�������ʱ�Զ��ͷţ�������C++�е�RAII������C++��RAIIҪ�����ǿ�ࣩܶ��

**alloca arguments:**
'alloca'ָ���ӵ�ǰ����ջ֡�Ϸ��� **sizeof(<type>) * NumElements** ���ֽڣ������ء��������͡���ָ��ָ���ڴ濪ʼ����� **"NumElements"** ָ���ˣ���ô�ͻ�����ô���Ԫ�ص��ڴ汻���䣬���� **"NumElements"** Ĭ��ָ��Ϊ1������moses��ʱû��array���ͣ����Ҳ�֧��ָ�룬������moses���ܹ�������Ԫ�ؿռ�ĳ����Ǿ����������ͱ����ˡ�

**alloca semantics:**
��ָ�������ڴ棬���ض�Ӧָ�롣�����ǰջ֡�ռ䲻�㣬ִ�и�ָ������δ������Ϊ��'alloca'ָ����Ҫ������ʾ�е�ַ�ġ��Զ�������������������ʱ���ÿռ䱻�Զ����ա�

���磺
```
%ptr = alloca i32
%ptr = alloca i32, i32 4
```

#### 'load' Instruction

**load syntax:**
```
<result> = load <ty>, <ty>* <pointer>
```

ע�⣬��moses��֧��LLVM IR�е� **volatile** �� **atomic**��'load'ָ�����ڴ��ڴ��ж����ݡ�

**load arguments:**
'load'ָ���ʵ������ָ�����ĸ�λ�ÿ�ʼװ�����ݡ�
**Note: If the load is marked as volatile, then the optimizer is not allowed to modify the number or order of execution of this load with other volatile operations. ����moses��֧�� volatile �� atomic ѡ�**

���磺
```
%ptr = alloca i32
store i32 3, i32* %ptr
%val = load i32, i32* %ptr
```

#### 'store' Instruction

**store syntax:**
```
store <ty> <value>, <ty>* <pointer>
```
'store'ָ����Ҫ�������ڴ�д�����ݡ�

**store arguments:**
���� 'store' ָ����˵��������ʵ����Ҫָ����һ�����洢��ֵ��һ���洢��ַ��

**store semantics:**
'store'ָ���� <pointer> ��������Ӧ���ڴ汻 <value> ֵˢ�¡�

���磺
```
%ptr = alloca i32
store i32 3, i32* %ptr
%val = load i32, i32 * %ptr
```

#### 'getelementptr' Instruction

**getelementptr syntax:**
```
<result> = getelementptr <ty>, <ty*> <ptrval>{, <ty> <idx>}*
```
'getelementptr'ָ�����ڻ�ȡ�ۺ�������Ԫ�صĵ�ַ����ָ��ֻ����е�ַ���㣬�������ȡ�ڴ档

**getelementptr arguments:**
��һ��ʵ�����ǻ����ͣ��ڶ�����������һ��ָ�����ַ��ָ�롣ʣ�µ�ʵ�ξ���һϵ�е��±꣬������ʶ�ۺ�������Ԫ�ء��±�Ľ��������������������Ӧ��������C����ָ��������ƫ���������;�����

LLVM IR��Ӧʾ����
```
struct RT {
    char A;
    int B[10][20];
    char c;
}��

struct ST {
    int X;
    double Y;
    struct RT Z;
};

int *foo(struct ST *s) {
    return &s[1].Z.B[5][13];
}
```
��Clang������LLVM code������ʾ��
```
%struct.RT = type {i8, [10 x [20 x i32]], i8}
%struct.ST = type {i32, double, %struct.RT}

define i32* @foo(%struct.ST* %s) nonwind uwtable readnone optsize ssp {
entry:
    %arrayidx = getelementptr inbounds %struct.ST, %struct.ST* %s, i64 1, i32 2, i32 1, i64 5, i64 13
    ret i32* %arrayidx
}
```

����ʾ���еĵ�һ��index�������� '%struct.ST*' ���ͣ��õ����� '%struct.ST' = '{i32, double, %struct.RT}' ���ͣ�һ���ṹ�塣�ڶ���index�������ǽṹ���еĵ�����Ԫ�أ��õ����� '%struct.RT' = '{ i8, [10 x [20 x i32]], i8}' ���ͣ�һ���µĽṹ�塣������index�������ǽṹ���еĵڶ���Ԫ�أ��õ����� '[10 x [20 x i32]]' ���ͣ�һ��array���͡���ȻҲ���Էֱ����index������LLVM code��ʾ��
```
define i32* @foo(%struct.ST* %s) {
    %t1 = getelementptr %struct.ST, %struct.ST* %s, i32 1
    %t2 = getelementptr %struct.ST, %struct.ST* %t1, i32 0, i32 2
    %t3 = getelementptr %struct.RT*, %struct.RT* %t2, i32 0, i32 1
    %t4 = getelementptr [10 x [20 x i32]], [10 x [20 x i32]]* %t3, i32 0, i32 5
    %t5 = getelementptr [20 x i32], [20 x i32]* %t4, i32 0, i32 13
    return i32* %t5
}
```
### Conversion Operations
��moses��ֻ��һ������ת���������������ͣ�anonymous type��ת��Ϊ�û��Զ�������(class type)������������moses IR������һ������ת������� 'bitcast .. to'��

#### 'bitcast .. to' Instruction
**syntax:**
```
<result> = bitcast <ty> <value> to <ty2>
```
'bitcast'ָ���ڲ��ı��κ�λ��ǰ���½�valueת��Ϊ����ty2��LLVM IR�ж��� 'bitcast' ָ��Ҫ��ת�������Ͳ����Ǿۺ����ͣ��������ǽ�������ı�Ϊֻ���ھۺ����͵�ת���������������������û��Զ������͵�ת������

### Other Operations
#### 'icmp' Instruction
**icmp syntax:**
```
<result> = icmp <cond> <ty> <op1>, <op2>
```
'icmp'ָ���������������ıȽϷ���һ��booleanֵ��

**icmp arguments:**
'icmp' ָ������������������һ���� *condition code* ����ִ�к��ֱȽϣ���һϵ�йؼ��ֱ�ʾ�� *condition code* ������ʾ��
```
1. eq: equal
2. ne: not equal
3. gt: greater than
4. ge: greater or equal
5. lt: less than
6. le: less or equal
```
ʣ�µĲ���ʱ�����������ȽϵĲ�������

���磺
```
<result> = icmp eq i32 4, 5     ; yields: result = false
<result> = icmp ne i32* %x, %x  ; yields: reault = false
<result> = icmp lt i32 4, 5     ; yields: result = true
```

#### 'phi' Instruction
**phi syntax:**
```
<result> = phi <ty> [<val0>, <label0>], ...
```
'phi'ָ��������SSAͼ��ʵ�֦սڵ㡣

**phi arguments:**
'phi'ָ��ĵ�һ������ָ���������ͣ������һ��ʵ�ζԣ�ÿһ����һ��ǰ���飨predecessor block���Լ����Ӧ��ֵ��ɡ�'phi'ָ�����λ��ÿһ���������ͷ���������'phi'ָ��Ļ�����

��������ʱ��'phi'ָ���ѡ��ĳ��ִ�е�ǰ�������飨ÿ��ִ�н���һ��ǰ���鱻ִ�У���Ӧ��ֵ������Ϊ��ǰ�������ֵ��

���磺
```
Loop:       ; Infinite Loop that counts from 0 on up ...
%indvar = phi i32 [ 0, %LoopHeader ], [%nextindvar, %Loop ]
%nextindvar = add i32 %indvar, 1
br label %Loop
```

#### 'call' Instruction

**call syntax:**
```
<result> = [tail | musttail | notail ] call [ret attrs] <ty> [<fnty>*] <fnptrval>(<function args>)
```

**call arguments:**
'call'ָ����Ҫһ��ʵ�Σ�

 - tail
 *tail* ��һ����ǣ������� C++ �е� *inline*������Ըú�������ִ��β�����Ż���tail call optimization����*tail* ���ֻ��һ����ʾ�����Ժ��ԡ�
 - musttail
 �� *tail* ��ͬ��*musttail* ��ʾ����Ըú�������ִ��β�����Ż���
��Ӧ�˴κ���������˵��*musttail* ���Ա�֤��ʹ�ú��������Ǻ�������ͼ�е�һ���ݹ黷��һ���֣��ú�������Ҳ���ᵼ��ջ������������
*tail* �� *musttail* ���� callee �����ȡ caller ��ͨ�� alloca �����ڴ�ı��������Ϊ *musttail* �ĵ��ñ����ܹ���������Ҫ��
(1) �������ñ�������� *ret* ָ��
(2) *ret* ָ��ֻ�ܷ���ǰ��ĺ������÷��ص�ֵ���߷��ؿ�
(3) caller �� callee�����ͱ����ܹ�ƥ�䡣
(4) callee �� caller �ĺ������ù���������ͬ

ֻҪ��������Ҫ�󣬱��Ϊ *tail* �ĺ�������һ���ܹ���֤β�����Ż���tail call optimization����
(1) caller �� callee ��ʹ�� *fastcc* ���ù�����
(2) ����������β����tail position������ *ret* ָ������ں������ú��棬���� *ret* ָ��ʹ�ú������÷��ص�ֵ����voidֵ��

 - nontail
 *nontail* ������ֹ�Ըú�������ִ��β�����Ż���
 - ty
 *ty* ��ʾ�ú������õ����ͣ�Ҳ���Ǻ����ķ������͡�
 - fnty
 ����ǩ����
 - fnptrval
 ǰ�������ᵽ��ÿһ��������Ӧһ������ָ�룬�������õ�ʱ��ʹ�õľ��Ǹ�ָ��ֵ��
 - function args
 ʵ���б�����ʵ�ε�������Ҫ�ͺ���ǩ����ƥ�䡣

���磺
```
%retval = call i32 @test(i32 %argc)
%X = tail call i32 @foo()
%Y = tail call fastcc i32 @foo()
call void %foo(i8 97)

%struct.A = type { i32, i8 }
%r = call %struct.A @foo()
```


----------
#Intrinsic Functions
LLVM IR֧�֡����ú�������һ�����Щ������������֪�����ֺ͹��ܣ����Ӧ��moses IRҲ֧����һ�����ʵ��������е��������C�����е� **sizeof()** �ؼ��֣�������C������ *sizeof* ����ȷ����Ϊ�ؼ��֡�

moses IR�е����ú����� **mosesir.** ǰ׺��ͷ�����ǰ׺�Ǳ����֣�Ҳ�����û��Զ��庯�����������ǰ׺��ͷ�����ú�����LLVM�ж���Ϊexternal������Ҳ����˵û�к����壬��ʵ���������ɱ��������߽��������涨�ġ�moses IR�е����ú������٣�Ҳû�����صı�Ҫ��


----------

## Accurate Garbage Collection Intrinsics
����moses������Ҫ�������յ�֧�֣�������ҪһЩ���ú������ṩһЩ�򵥹��еĲ�����

��Щ���ú�����������ȷ��ջ�ϵ� **GC roots**���Լ�������������ʵ����Ҫ *read barriers* �� *write barriers*��moses���������Ͱ�ȫ�ģ�������ǰ����Ҫ���м�������ɵ�ʱ���������Щ���ú�����

### 'mosesir.gcroot' Intrinsic
**mosesir.gcroot syntax:**
```
func void @mosesir.gcroot(i8** %ptrloc, i8* %metadata)
```

**mosesir.gcroot arguments:**
'mosesir.gcroot' ���ú���������������˵���� *GC root* �Ĵ��ڣ�ע��������LLVM��������䲻ͨ˳����������Я��һЩԪ���ݡ���һ��ʵ��ȷ���˰��� *root pointer* ջ�϶���ĵ�ַ���ڶ���ָ�루һ��������������һ��ȫ��ֵ�ĵ�ַ������һЩ�������ص�Ԫ���ݡ�

�ڳ������е�ʱ��*mosesir.gcroot* ���ú������� 'ptrloc' ��λ�ô洢һ����ָ�롣�ڱ����ڼ䣬����������������һЩ��Ϣ���Ա�������ʱϵͳ�ܹ��� *GC safe points* ȷ��������

### 'mosesir.gcread' Intrinsic

**mosesir.gcread syntax:**
```
func i8* @mosesir.gcread(i8* %ObjPtr, i8** %Ptr)
```

�������������кܶ��֣���һ������ʽ��Incremental���������վ����������չ��̺������ߣ�mutator������ִ�У������ߵ�ִ���п��ܻ��޸�ǰ��������������¼�õ���Ϣ��������Ҫ��¼�����߶��� *heap* �Ķ�д��

'mosesir.gcread' ���ú����Ὣ�� *heap* �ڴ�Ķ�������Ϣ����������������'mosesir.gcread' ���ú����� 'load' ָ������ͬ�����塣

### 'mosesir.gcwrite' Intrinsic
**mosesir.gcwrite syntax:**
```
func void @mosesir.gcwrite(i8* %P1, i8* %Obj, i8** %P2)
```

'mosesir.gcwrite'���ú�����ʶ������� 'heap' �ڴ��д��������������������ͨ�������ú���ʵ�� *write barriers* ����ִ������ü�������

'mosesir.gcwrite'���ú������� 'store' ָ������壬���Ǹ����ӡ�

## Code Generator Intrinsics
��Щ���ú��������������أ�LLVM�����˺ܶ������ĺ����е����ڵ��ԣ��еķ���ִ��ĳЩ���������� 'reurnaddress' 'memset.*'�ȡ��˴� moses IR �ݲ��ṩ��Щ������

���� [@movie-travel-code][6]   
2016 �� 05�� 14��    


  [1]: http://blog.csdn.net/phunxm/article/details/8985321
  [2]: http://img.blog.csdn.net/20130529091544921
  [3]: https://tiancaiamao.gitbooks.io/go-internals/content/zh/03.2.html
  [4]: https://tiancaiamao.gitbooks.io/go-internals/content/zh/images/3.2.funcall.jpg
  [5]: https://en.wikipedia.org/wiki/Dominator_%28graph_theory%29
  [6]: http://weibo.com/movietravelcode