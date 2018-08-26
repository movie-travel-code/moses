func fac(parm:int) -> int {
  var result = 1;
  while(parm > 1) {
    result *= parm--;
  }
  return result; 
}

var result = fac(10);
print(result);