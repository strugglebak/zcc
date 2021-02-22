	.text
.LC0:
	.string	"%d\n"
printint:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %eax
	movl	%eax, %esi
	leaq	.LC0(%rip), %rdi
	movl	$0, %eax
	call	printf@PLT
	nop
	leave
	ret

	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	$2, %r8
	movq	$3, %r9
	movq	$5, %r10
	imulq	%r9, %r10
	addq	%r8, %r10
	movq	$8, %r8
	movq	$3, %r9
	movq	%r8, %rax
	cqo
	idivq	%r9
	movq	%rax, %r8
	subq	%r8, %r10
	movq	%r10, %rdi
	callregister_print
	movl	$0, %eax
	popq	%rbp
	ret
