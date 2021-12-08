CC = g++
# Think of test libs as telling what libraries to include 
TESTLIBS = -lz -lpthread -pthread # -lgtest  -lgtest_main 
OBJS = SimpleHeader.o #test_header.o
HEADERDIRS = SimpleHeader.h bouner.cc
CCFLAGS = --std=c++11 

# In order to rename the executable file generated rename bouncer on line 11
# In order to run the make file run 'make' in the terminal (make sure you're in the 
# same directory as the Makefile without the single quotations
bouncer: $(OBJS)
	$(CC) -o $@ -L. $(OBJS) $(TESTLIBS) $(HEADERDIRS)

%.o : %.cc
	$(CC) $(CCFLAGS) -c $<
	$(CC) $(CCFLAGS) -MM -MP -MT $@ $< > $(basename $@).d


.PHONY : clean
clean :
	rm -f *.o *~ *.d bouncer


## include the generated dependency files
-include $(addsuffix .d,$(basename $(OBJS)))
