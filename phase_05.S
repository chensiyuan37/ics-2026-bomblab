
./bomb:     file format elf64-x86-64


Disassembly of section .init:

Disassembly of section .plt:

Disassembly of section .plt.sec:

Disassembly of section .text:

0000000000402430 <phase_05>:
  402430:	f3 0f 1e fa          	endbr64
  402434:	55                   	push   %rbp
  402435:	48 89 e5             	mov    %rsp,%rbp
  402438:	41 54                	push   %r12
  40243a:	49 89 f4             	mov    %rsi,%r12
  40243d:	be 02 00 00 00       	mov    $0x2,%esi
  402442:	53                   	push   %rbx
  402443:	48 89 fb             	mov    %rdi,%rbx
  402446:	48 83 ec 10          	sub    $0x10,%rsp
  40244a:	e8 b1 07 00 00       	call   402c00 <valid_u32_fields>
  40244f:	84 c0                	test   %al,%al
  402451:	75 0d                	jne    402460 <phase_05+0x30>
  402453:	31 c0                	xor    %eax,%eax
  402455:	48 83 c4 10          	add    $0x10,%rsp
  402459:	5b                   	pop    %rbx
  40245a:	41 5c                	pop    %r12
  40245c:	5d                   	pop    %rbp
  40245d:	c3                   	ret
  40245e:	66 90                	xchg   %ax,%ax
  402460:	31 c0                	xor    %eax,%eax
  402462:	48 8d 4d e8          	lea    -0x18(%rbp),%rcx
  402466:	48 8d 55 e4          	lea    -0x1c(%rbp),%rdx
  40246a:	be 7f 31 40 00       	mov    $0x40317f,%esi
  40246f:	48 89 df             	mov    %rbx,%rdi
  402472:	e8 69 ed ff ff       	call   4011e0 <__isoc99_sscanf@plt>
  402477:	83 f8 02             	cmp    $0x2,%eax
  40247a:	75 d7                	jne    402453 <phase_05+0x23>
  40247c:	48 8d 75 ec          	lea    -0x14(%rbp),%rsi
  402480:	4c 89 e7             	mov    %r12,%rdi
  402483:	e8 38 ff ff ff       	call   4023c0 <p5_traverse>
  402488:	89 c2                	mov    %eax,%edx
  40248a:	31 c0                	xor    %eax,%eax
  40248c:	39 55 e4             	cmp    %edx,-0x1c(%rbp)
  40248f:	75 c4                	jne    402455 <phase_05+0x25>
  402491:	8b 45 ec             	mov    -0x14(%rbp),%eax
  402494:	39 45 e8             	cmp    %eax,-0x18(%rbp)
  402497:	0f 94 c0             	sete   %al
  40249a:	eb b9                	jmp    402455 <phase_05+0x25>

Disassembly of section .fini:
