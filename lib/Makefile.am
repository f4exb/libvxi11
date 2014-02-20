lib_LTLIBRARIES = libvxi11.la

libvxi11_la_SOURCES = clnt_find_services.cc \
    vxi11_xdr.c \
    vxi11_clnt.c \
    vxi11_svc.c \
    vxi11_server.c \
    vxi11_user.cc 

CLEANFILES = vxi11.h vxi11_svc.c vxi11_clnt.c vxi11_xdr.c

RPCGEN = rpcgen

vxi11_xdr.c: $(top_srcdir)/lib/vxi11.x vxi11.h
	$(RPCGEN) -M -c -o vxi11_xdr.c $(top_srcdir)/lib/vxi11.x
	
vxi11_clnt.c:	
	$(RPCGEN) -M -l -o vxi11_clnt.c $(top_srcdir)/lib/vxi11.x
	
vxi11_svc.c:
	$(RPCGEN) -M -m -o vxi11_svc.c $(top_srcdir)/lib/vxi11.x
	
vxi11_server.c:
	$(RPCGEN) -M -Ss -o vxi11_server.c $(top_srcdir)/lib/vxi11.x
	
vxi11.h:	
	$(RPCGEN) -M -h -o vxi11.h $(top_srcdir)/lib/vxi11.x
	cp vxi11.h $(top_srcdir)/lib

#$(BUILT_SOURCES): $(top_srcdir)/lib/vxi11.x
#	$(RPCGEN) -M $(top_srcdir)/lib/vxi11.x

library_includedir=$(includedir)
library_include_HEADERS = clnt_find_services.h \
    vxi11_user.h \
    vxi11.h

