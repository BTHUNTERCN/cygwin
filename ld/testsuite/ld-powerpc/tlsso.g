#source: tls.s
#as: -a64
#ld: -shared
#objdump: -sj.got
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Contents of section \.got:
.* (00000000|80870100) (00018780|00000000) 00000000 00000000  .*
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 00000000 00000000 00000000  .*
.* 00000000 00000000 00000000 00000000  .*