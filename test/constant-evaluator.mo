func add(lhs : int, rhs : int) -> int
{
	return lhs + rhs * 2 - 40 + lhs * (rhs - rhs/10);
}
const global = 10;

// Evalute "num" directly to 213
var num = add(global, 20) + 23;

// Evalute "num > 0" directly to true
if (num > 0)
{
	num = 0;
}