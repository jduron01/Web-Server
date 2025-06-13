# Web-Server
Implementation of a web server written both in C and Rust to compare language differences and performance. Only supports HTTP and currently supported HTTP methods are GET and POST requests. Will add support for other features in the future.

## Performance Comparison
Apache Benchmark tool was used to compare the performance between the C and Rust implementation. To run the benchmarks,
1. Install Apache Benchmark
``` bash
sudo apt install apache2-utils
```
2. Run the following command once one of the server programs is running. A file in the public directory can be used.
``` bash
ab -n 100000 -c 100 -k http://localhost:8080/<file name>
```
