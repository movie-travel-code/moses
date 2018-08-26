class Node 
{
	var m1 : int;
	var m2 : int;
};

func add(parm:Node) -> int
{
	return parm.m1 + parm.m2;
}

var test:Node;
test.m1 = 10;
test.m2 = 20;
var result = add(test);
print(result);