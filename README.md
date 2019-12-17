# Dynamic-Memory-Allocator
You will create an allocator for the x86-64 architecture with the following features:   Free lists segregated by size class, using first-fit policy within each size class. Immediate coalescing of blocks on free with adjacent free blocks. Boundary tags to support efficient coalescing. Block splitting without creating splinters. Allocated blocks aligned to "double memory row" (16-byte) boundaries. Free lists maintained using last in first out (LIFO) discipline. Obfuscation of block footers to detect attempts to free blocks not previously obtained via allocation.   You will implement your own versions of the malloc, realloc, and free functions.
