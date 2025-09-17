#include<stdio.h>
int fib(int n) {
  if(n<2) return n;
  else return fib(n-1)+fib(n-2);
}

int main() {
	int a = fib(40);
  printf("value = %d\n", a);
	return 0;
}