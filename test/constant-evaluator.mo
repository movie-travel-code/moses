func add(lhs : int, rhs : int) -> int
{
	return lhs + rhs * 2 - 40 + lhs * (rhs - rhs/10);
}
const global = 10;

// �������ǿ��Խ� "num" evaluate ��Ϊ213
var num = add(global, 20) + 23;

// �������ǿ��Խ� "num > 0" evaluate ��Ϊ true
if (num > 0)
{
	num = 0;
}