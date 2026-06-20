NAME:=grep

CXX:=c++
CXXFLAGS:=-Wall -Wextra -Werror -std=c++11 -O3 -MMD -MP

SRCS:=grep.cpp
OBJS:=$(SRCS:.cpp=.o)
TOBJS:=$(SRCS:.cpp=_test.o)
DEPS:=$(OBJS:.o=.d)

$(NAME) : $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

test_bin: $(TOBJS)
	$(CXX) $(CXXFLAGS) $(TOBJS) -o $@

%_test.o : %.cpp
	$(CXX) $(CXXFLAGS) -DLOG_LEVEL=0 -c $< -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -DLOG_LEVEL=2 -c $< -o $@


test: test_bin
	@./tester.sh
clean:
	rm -f $(OBJS) $(TOBJS) $(DEPS) $(TDEPS)

fclean: clean
	rm -f $(NAME) test_bin

all: $(NAME)

re: fclean all
	
.PHONY: clean fclean re all

-include $(DEPS) $(TDEPS)
