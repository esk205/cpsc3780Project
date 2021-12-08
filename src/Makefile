CC = g++11
# Think of test libs as telling what libraries to include 
TESTLIBS = -lgtest  -lgtest_main -lpthread
OBJS = SimpleHeader.o #test_header.o
# HEADERDIRS =
CCFLAGS = --std=c++11

# rename bouncer to rename the executable file generated
# In order to run the make file run 'make' in the terminal (make sure you're in the 
# same directory as the Makefile without the single quotations
bouncer: $(OBJS)
	$(CC) -o $@ -L. $(OBJS) $(TESTLIBS)

%.o : %.cc
	$(CC) $(CCFLAGS) -c $<
	$(CC) $(CCFLAGS) -MM -MP -MT $@ $< > $(basename $@).d


.PHONY : clean
clean :
	rm -f *.o *~ *.d


## include the generated dependency files
-include $(addsuffix .d,$(basename $(OBJS)))
