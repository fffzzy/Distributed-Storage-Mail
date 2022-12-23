# README

#### Please follow steps below to run PennCloud for Team 07



### Step1: Install gRPC

Follow the link [gRPC quick start](https://grpc.io/docs/languages/cpp/quickstart/) to install gRPC, we used default ```MY_INSTALL_PATH=$home/.local```



### Step 2: Compile

#### HTTP Service

##### 2.1 HTTP Loadbalancer

```shell
# Compile under T07/HTTP\ Server
make loadbalancer
```

##### 2.2 SMTP + Mail service

```shell
# Compile under T07/HTTP\ Server
mkdir build
cd build
cmake ..
make -j
```

##### 

#### KVStore Service

```shell
# Compile under T07/KVStore
mkdir build
cd build
cmake ..
make -j
```



### Step 3: Run

#### Loadbalancer

The default port is 8009, we you visit ```localhost:8009/```, the load balancer will randomly redirect you to another running http service.

```shell
# Run under T07/HTTP\ Server
./loadbalancer # default port is 8009
```

#### STMP

```shell
# Run under T07/HTTP\ Server/build
./mail
```

#### HTTP Service nodes

Start three http nodes, the load balancer will randomly(disable cash in browser) pick one of port. 

```shell
# Run under T07/HTTP\ Server/build
./server -v -p 8019
./server -v -p 8029
./server -v -p 8039
```

#### KVStore Coordinator(master node)

```shell
# Run under T07/KVStore/build
./kvstore_master -v
```

#### KVStore Worker nodes

In our ```Config/serverlist.txt```, we have pre-configured 7 nodes in total, the first line is the IP address and port number for master nodes and the following 6 are for 2 clusters which contains 3 replicas respectively. Hence, we need to start 6 worker nodes.

```shell
# Run under T07/KVStore/build
# please put -v before node index
./kvstore_node -v 1 # node of cluster 1
./kvstore_node -v 2 # node of cluster 1
./kvstore_node -v 3 # node of cluster 1
./kvstore_node -v 4 # node of cluster 2
./kvstore_node -v 5 # node of cluster 2
./kvstore_node -v 6 # node of cluster 2
```



### Step 4: Browser

We recommend using **Firefox** browser, since loadbalancer for front-end may not work in other browsers.

Open **Firefox** and go to ```localhost:8009/```. 