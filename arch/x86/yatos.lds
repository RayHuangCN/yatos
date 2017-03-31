SECTIONS {
    . = 0xc0000000;
    
	.text : {
		built-in.o (.start)
		built-in.o (.text)
	}

	.rodata : {
			* (.rodata)
	}

	.data : {
			* (.data)
	}
	__real_data_end = .;
	
	    __bss_start = .;
  .bss ALIGN(4)  : { *(.bss)  *(COMMON) }
     __bss_end = .;
  

}
