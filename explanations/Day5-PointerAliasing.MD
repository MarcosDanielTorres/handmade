```
/*
Pointer Aliasing.
When two pointers could point to the same memory and the compiler doesn't know if a write
to one of these pointers might effect a read from the other pointer.

int X = 1;
int Y = 2;
int Z = 3;
int W = 4;

Move(&X, &Y, &Z, &Y)
void Move(int* A, int* B, int* C, int* D) {
	*A = *B;	// LOAD B AND WRITE A
	*D = 5;		// If this weren't here, the compiler could optimize the next line
	*C = *B;	// MUST LOAD B AGAIN AND WRITE C

	// It would be preferable that the compiler optimize this away (loading B once), but it can't.
}
*/
```