# In-Memory IR

我们几乎是照搬了LLVM关于IR的设计，将LLVM IR进行简化得到了moses IR，其中重点关注了moses中多值返回的问题。这里我们介绍一下In-Memory IR的形式，in-memory的含义就是IR如何在内存中分布的，其实也就是通过一系列的Object对象来将IR串联起来。IR在编译过程中占有很重要的地位，IR几乎囊括了前面处理得到的所有有用信息，在后面的代码优化及生成时，编译器只参考IR即可，不需要再回过头去查阅源码。

树和图是程序最直观的表示，能够很好的反映出程序的结构，但是现有的计算机平台都只能运行线性的汇编语言。所以这里就出现了对IR形式的选择，是选择线性IR，还是图IR或者树IR。树IR比较能反映到语言的语法结构，只是这种语法结构对于生成高性能的代码用处不是很大。虽然语法结构对生成高性能代码用处不是很大，但是控制流结构对代码生成有用（所以说CFG图对于IR来说始终是不可或缺的一部分）。

moses IR的 *in-memory* 形式可以说是一种 “混合IR” 的形式，因为中间会使用 *BasicBlock* 来组织其中的 **instruction**，在各个阶段的处理中可以很方便的从 *in-memory* 的形式中获取到CFG信息。moses IR的文本形式类似于LLVM IR，其中的CFG信息是由文本中的 *label* 表示的。

> 编译器的许多部分显示或隐式地依赖于CFG。为支持优化而进行的代码分析通常从控制流分析和CFG构建开始（例如数据流分析就是在CFG图上进行的）。而 *instruction scheduling* 也需要CFG才能理解各个块的被调度代码是如何汇流的。全局寄存器分配也依赖于CFG，才能理解各个操作执行频度如何，以及在何处插入对特定值的load和store指令。

> 为了记录编译器必须编码的所有细节，大多数编译器编写者都向IR添加了表和集合，以记录额外的信息。我们认为这些表是IR的一部分。 -《Engineering a compiler》

另外moses IR没有提供Call Graph的构建，Call Graph对于进行过程间静态分析来说非常重要。

----------

## IR中的类
### Value
Value 是 IR 中所有对象的基类，可以表示 *function*、*Instruction*、*Block*、*GlobalVariable*等。
```
// LLVM IR is strongly typed, every instruciton has a specific type it expectes as each operand.
// For example, store requires that the address's type will always be 
// a pointer to the type of the value being stored.
enum class ValueTy
{
    TypeVal,
    ConstantVal,
    ArgumentVal,
    InstructionVal,
    BasicBlockVal,
    FunctionVal,
    GlobalVariableVal
};
```
从上面代码中我们可以看出 Value 的分类中，只有 ***FunctionVal***、***BasicBlockVal***、***InstructionVal*** 是真正的代码实体，其他的都是作为辅助性信息存在的，例如 ***TypeVal***、***ConstantVal***、***GlobalVariableVal***等。

每个 *Value* 都有一个 *uselist* 来表示当前值（包括 *instruction*、*function*、*block*）正在被多少个地方使用着。例如 *function* 可以作为 *call instruction* 的 operand，*BasicBlock* 可以作为 *br instruction* 的 operand。

----------
## In-Memory IR


