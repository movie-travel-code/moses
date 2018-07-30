# In-Memory IR

���Ǽ������հ���LLVM����IR����ƣ���LLVM IR���м򻯵õ���moses IR�������ص��ע��moses�ж�ֵ���ص����⡣�������ǽ���һ��In-Memory IR����ʽ��in-memory�ĺ������IR������ڴ��зֲ��ģ���ʵҲ����ͨ��һϵ�е�Object��������IR����������IR�ڱ��������ռ�к���Ҫ�ĵ�λ��IR����������ǰ�洦��õ�������������Ϣ���ں���Ĵ����Ż�������ʱ��������ֻ�ο�IR���ɣ�����Ҫ�ٻع�ͷȥ����Դ�롣

����ͼ�ǳ�����ֱ�۵ı�ʾ���ܹ��ܺõķ�ӳ������Ľṹ���������еļ����ƽ̨��ֻ���������ԵĻ�����ԡ���������ͳ����˶�IR��ʽ��ѡ����ѡ������IR������ͼIR������IR����IR�Ƚ��ܷ�ӳ�����Ե��﷨�ṹ��ֻ�������﷨�ṹ�������ɸ����ܵĴ����ô����Ǻܴ���Ȼ�﷨�ṹ�����ɸ����ܴ����ô����Ǻܴ󣬵��ǿ������ṹ�Դ����������ã�����˵CFGͼ����IR��˵ʼ���ǲ��ɻ�ȱ��һ���֣���

moses IR�� *in-memory* ��ʽ����˵��һ�� �����IR�� ����ʽ����Ϊ�м��ʹ�� *BasicBlock* ����֯���е� **instruction**���ڸ����׶εĴ����п��Ժܷ���Ĵ� *in-memory* ����ʽ�л�ȡ��CFG��Ϣ��moses IR���ı���ʽ������LLVM IR�����е�CFG��Ϣ�����ı��е� *label* ��ʾ�ġ�

> ����������ಿ����ʾ����ʽ��������CFG��Ϊ֧���Ż������еĴ������ͨ���ӿ�����������CFG������ʼ����������������������CFGͼ�Ͻ��еģ����� *instruction scheduling* Ҳ��ҪCFG������������ı����ȴ�������λ����ġ�ȫ�ּĴ�������Ҳ������CFG����������������ִ��Ƶ����Σ��Լ��ںδ�������ض�ֵ��load��storeָ�

> Ϊ�˼�¼������������������ϸ�ڣ��������������д�߶���IR����˱�ͼ��ϣ��Լ�¼�������Ϣ��������Ϊ��Щ����IR��һ���֡� -��Engineering a compiler��

����moses IRû���ṩCall Graph�Ĺ�����Call Graph���ڽ��й��̼侲̬������˵�ǳ���Ҫ��

----------

## IR�е���
### Value
Value �� IR �����ж���Ļ��࣬���Ա�ʾ *function*��*Instruction*��*Block*��*GlobalVariable*�ȡ�
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
��������������ǿ��Կ��� Value �ķ����У�ֻ�� ***FunctionVal***��***BasicBlockVal***��***InstructionVal*** �������Ĵ���ʵ�壬�����Ķ�����Ϊ��������Ϣ���ڵģ����� ***TypeVal***��***ConstantVal***��***GlobalVariableVal***�ȡ�

ÿ�� *Value* ����һ�� *uselist* ����ʾ��ǰֵ������ *instruction*��*function*��*block*�����ڱ����ٸ��ط�ʹ���š����� *function* ������Ϊ *call instruction* �� operand��*BasicBlock* ������Ϊ *br instruction* �� operand��

----------
## In-Memory IR


