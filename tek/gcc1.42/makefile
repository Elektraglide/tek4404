CFLAGS= +i=config

all: cc1

singlegnulibabi.r:	singlegnulibabi.s
	$(AS) $< +ueo=$@

cc1: cc1.1.r cc1.2.r cc1.3.r cc1.4.r cc1.5.r cc1.6.r cc1.7.r cc1.8.r singlegnulib.r singlegnulibabi.r
	cc +vo=cc1 +x=f +x=b=1M cc1.1.r cc1.2.r cc1.3.r cc1.4.r cc1.5.r cc1.6.r cc1.7.r cc1.8.r singlegnulib.r singlegnulibabi.r


