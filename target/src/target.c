#include <stdio.h>

int main(int argc, char *argv[]){
	sample_struct sample = {
		.foo = 1,
		.bar = 2
	};
	printf("foo: %d, bar: %d\n", sample.foo, sample.bar);
	return 0;
}
