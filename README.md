## Inter Process Communication
### Contributor
Zeyu Chen(chenzy@gatech.edu), Yaohong Wu(ywu669@gatech.edu)

### How to use

```
./kill_ipcs.sh
cmake .
make
./ipc
```
Make sure to run the code under authority. Before each run, run `kill_ipcs.sh` to 
guarantee the message queue is cleaned. 

### Output
Output files are under `output` dir. The filename is named after the process pid.
