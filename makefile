ros_executor_prototype: ros_executor_prototype.c ros_queue.c ros_static_allocator.c ros_exec_shm.c ros_task_set.c
	gcc -std=c11 -D_XOPEN_SOURCE=500 -o $@ $^ -lpthread -lrt -lm

clean: ros_executor_prototype
	rm $^