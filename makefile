MODULE_big = tran_extension
EXTENSION = tran_extension
DATA = tran_extension--0.0.5.sql 
PGFILEDESC = "TRANManager extension for postgresql"
PG_CONFIG = $(PG_HOME)/bin/pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

OBJS = tran_extension.o
PG_CFLAGS += -liconv
#EUCKR 조건부 컴파일
#EUCKR 의 경우 NULL값 반환
EUCKR := $(shell echo ${LANG} | grep -i euckr)

ifneq ($(EUCKR), )
	PG_CFLAGS += -D_EUCKR
else
	PG_CFLAGS += 
endif

include $(PGXS)
