# moses
Moses is a simple programming language. Moses referring to the swift , but much simpler.

Like function definition.
func add(lhs : int, rhs : int) -> int
{
	return lhs + rhs;
}

Variable declaration.
var num : int; // or var num = 10;
If you do not provide the type to a variable , moses will perform simple type inference.

Moses currently supports only two control structures(if-else and while-loop).
if lhs < rhs 
{
	return rhs;
}
else
{
	return lhs;
}

Moses is an object-oriented language. We can define new class type ,like C/C++ does.
class A
{
	var num : int;
	var flag : bool;
};

Moses is a new programming language , I am gradually enrich it.

