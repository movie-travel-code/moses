func add(lhs : int, rhs : int) -> int
{
	return lhs + rhs * 2 - 40 + lhs * (rhs - rhs/10);
}
const global = 10;

// 这里我们可以将 "num" evaluate 成为213
var num = add(global, 20) + 23;

// 这里我们可以将 "num > 0" evaluate 成为 true
if (num > 0)
{
	num = 0;
}