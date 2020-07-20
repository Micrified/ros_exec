ros_executor_prototype: ros_executor_prototype.c ros_queue.c ros_static_allocator.c ros_exec_shm.c ros_task_set.c ros_inet.c
	gcc -std=gnu11 -D_GNU_SOURCE -D_XOPEN_SOURCE=500 -o $@ $^ -lpthread -lrt -lm

ros_request_simulator: ros_request_simulator.c
	gcc -std=gnu11 -D_GNU_SOURCE -D_XOPEN_SOURCE=500 -o $@ $^ -lpthread

clean: ros_executor_prototype ros_request_simulator
	rm $^