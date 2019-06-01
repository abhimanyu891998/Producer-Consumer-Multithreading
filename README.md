# Producer_Consumer_Multithreading
Program written in C to implement producer consumer interaction in a multi-threaded program. It counts the frequencies of selected keywords in a document and generates the resulting statistics. Demonstrates the interaction between master and worker threads while utilising the concept of semaphores to handle critical sections. 
Usage: - Compile the program using ```gcc -o thrwordcnt thrwordcnt.c```

- Run ```./thrwordcnt [number of workers] [number of buffers] [target plaintext file] [keyword file]```



