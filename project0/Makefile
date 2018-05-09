# NAME: Boyuan He
# EMAIL: boyuan_heboyuan@live.com
# ID: 004791432

.SILENT:

default:
	gcc -Wall -Wextra -g lab0.c -o lab0

check:
	gcc -Wall -Wextra -g lab0.c -o lab0
	
	echo "general_test"> general_test.txt

	./lab0 < general_test.txt &>/dev/null;\
	if [[ $$? -ne 0 ]];\
	then \
		echo "normal test fail, should return 0 but return $$?"; \
	else \
		echo "normal test passed"; \
	fi

	echo "success input" > input.txt;\
	./lab0 --input=input.txt &>/dev/null;\
	if [[ $$? -ne 0 ]];\
	then \
		echo "success input test fail, should return 0 but return $$?"; \
	else \
		echo "success input test passed"; \
	fi

	touch output.txt;\
	./lab0 --output=output.txt < general_test.txt &>/dev/null;\
	if [[ $$? -ne 0 ]];\
	then \
		echo "success output test fail, should return 0 but return $$?"; \
	else \
		echo "success output test passed"; \
	fi

	echo "wrong input" > w_input.txt;\
	chmod -r w_input.txt;\
	./lab0 --input=w_input.txt &> /dev/null; \
	if [[ $$? -ne 2 ]];\
	then\
		echo "wrong input test fail, should return 2 but return $$?";\
	else \
		echo "wrong input test passed";\
	fi

	touch w_output.txt;\
	chmod -w w_output.txt;\
	./lab0 --output=w_output.txt< general_test.txt &> /dev/null; \
	if [[ $$? -ne 3 ]];\
	then\
		echo "wrong output test fail, should return 3 but return $$?";\
	else \
		echo "wrong output test passed";\
	fi

	./lab0 --segfault --catch &> /dev/null;\
	if [[ $$? -ne 4 ]];\
	then\
		echo "catch test fail, should return 4 but return $$?";\
	else\
		echo "catch test passed";\
	fi

clean:
	rm -f lab0 *.o *.txt *.tar.gz

dist:
	tar -czvf lab0-004791432.tar.gz lab0.c Makefile README *png