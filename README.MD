Notes
- Day 1 - 29/4/2024
    Videos Covered:  Day 1

Notes for Videos:
Day 1
Added C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.38.33130\bin\Hostx64\x64 to the PATH for cl.exe
Added /k c:\users\gg\desktop\handmade\misc\shell.bat to CMD launch options.f


Day 4
Building on the commmand line article: https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170

Day 10: QueryPerformanceCounter and RDTSC

QueryPerformanceCounter: https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
    QueryPerformanceFrequency: https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
RDTSC:
On Intel CPUs the RDTSC register it's used to measure how many clock cycles it takes to run a certain set of instructions:
"Reads the current value of the processor’s time-stamp counter (a 64-bit MSR) into the EDX:EAX registers. The processor monotonically increments the time-stamp counter MSR every clock cycle and resets it to 0 whenever the processor is reset" - Intel 64 and IA-32 Architectures Manual Software Developer's Manual

Considerations of using it directly:
- Intel processors uses `out-of-order execution`, where instructions are not necessarily performed in the order they appear.
- If the clock frequency varies, the result may not be accurate.
- If the OS do something else in between the execution of the program, the result may not be accurate.
- Portability issues
- No promise that the timestamp of multiple CPUs will be synchronized.
- Probably only useful if limiting the program to run on one CPU.


using f32 is twice as fast as f64
Based on Ivy Bridge
Both of these take two arguments of 64 bits each and take 5 cycles to complete and 1 cycle if executing them all at once.
MULPS -> f32 128-bits / 32 bits => 4 f32 into one register
MULPD -> f64 128-bits / 64 bits => 2 f32 into one register