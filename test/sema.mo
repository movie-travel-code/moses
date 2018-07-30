func add( lhs : {int, int}) -> {int, int}
{
	var {lhs1, lhs2} = lhs;
	return {lhs1 + lhs2, lhs1 + lhs2};
}

var {lhs, rhs} = add({10, false});

func sub() -> int
{
	var a1 = 10;
	var num = {a1 + 10, 20};
	var {test1, test2 } = add(num);
	return test1;
}

if (lhs > rhs)
{
	lhs += rhs;
}
