func compare(lhs : int, rhs : int) -> bool
{
	if (lhs > rhs)
	{
		return true;
	}
	else
	{
		return false;
	}
}

func add(lhs : int, rhs : int) -> int
{
	return lhs + rhs;
}

const num : int;
num = 10;

compare(0, 10);
add(12, 13);

var num1 : int;
var num2 : int;
var flag : bool;
num1 = 11;
num2 = 12;
flag = true;
var num = 10;
add (num1 + num2 + flag, num);
