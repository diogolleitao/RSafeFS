<div id="top"></div>


[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]



<!-- PROJECT LOGO -->
<br />
<div align="center">
<!-- 
  <a href="https://github.com/diogolleitao/RSafeFS">
    <img src="docs/images/architecture.svg" alt="Logo" width="80" height="80">
  </a>
-->

  <h3 align="center">RSafeFS</h3>

  <p align="center">
    RSafeFS: A Modular File System for Remote Storage
    <br />
    <a href="https://github.com/diogolleitao/RSafeFS"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/diogolleitao/RSafeFS">View Demo</a>
    ·
    <a href="https://github.com/diogolleitao/RSafeFS/issues">Report Bug</a>
    ·
    <a href="https://github.com/diogolleitao/RSafeFS/issues">Request Feature</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#rsafefs">RSafeFS</a></li>
        <li><a href="#layers-and-drivers">Layers and drivers</a></li>
        <li><a href="#configuration-examples">Configuration examples</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

RSafeFS is a software-defined file system based on SafeFS. SafeFS is a file system with a modular architecture featuring stackable layers that can be combined to construct a customized file system. The SafeFS architecture allows users to specialize their data store to their specific needs by choosing the combination of layers.
For more information regarding SafeFS, you may read this [paper](https://repositorio.inesctec.pt/server/api/core/bitstreams/85db7293-c524-40cc-8fc8-fdb0701ce377/content).

RSafeFS extends this system by providing a complete implementation in C++ and adding remote storage capability. 
The prototype is implemented in the user space using the [FUSE interface](https://github.com/libfuse/libfuse).

The following image shows a possible architecture of two RSafeFS instances. One acts as a client and the other as a server. The client is composed of three layers, while the server is composed of two layers. 
The request generated by FUSE is propagated through the client layers and then transmitted to the server to do the same in its layers. After all the layers have processed the request, the reply gets propagated in inverse order.

![Architecture](docs/images/architecture.svg)

The current implementation of RSafeFS provides several layers that include mechanisms based on data and metadata caching, read-ahead, and remote procedure calls (RPC) to enable client-server communication.

Each layer can provide a set of drivers to provide different approaches. For instance, different cache evictions in the caches layers, different communication protocols in the RPC layer, or different encryption mechanisms in the encryption layer.
This mechanism allows for an even more flexible and modular file system approach.


<p align="right">(<a href="#top">back to top</a>)</p>



<!-- LAYERS AND DRIVERS -->
### Layers and Drivers

These are the layers available:
- `rpc_client` The RPC client layer sends requests to a server and waits for its reply, using remote procedure calls (RPC).
- `data_cache` The data caching layer aims to reduce the number of data read operations sent to the following layers by keeping in memory blocks of readen data.
- `metadata_cache` The metadata caching layer aims to reduce the number of operations on metadata that reach the storage layers by using metadata stored in memory.
- `read_ahead` The read-ahead layer aims to increase read performance by reading ahead data before it is needed.
- `local` The local layer sends requests to an existing file system.

Next are detailed all the parameters for the configuration file and how to configure the layers. Some parameters are optional. If not specified, the RSafeFS instance uses the default values.

#### RPC client configuration (`rpc_client`)
| Parameter        |           Required            |  Type  | Description                                                                                                                                                                                                                                                                                                                                                                      |
| :--------------- | :---------------------------: | :----: | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `server_address` |      :white_check_mark:       | String | Server's address (e.g., `10.0.0.1:50051`)                                                                                                                                                                                                                                                                                                                                        |
| `mode`           | :negative_squared_cross_mark: | String | Available options: synchronous (`sync`), asynchronous (`async`). The synchronous client sends requests to the server and waits for their reply. The asynchronous client delays sending write operations to the server until certain events happen (e.g., until the memory usage limit is reached or the application explicitly performs operations that cause data flushes) |

The following parameters are only valid if the mode is asynchronous
| Parameter         |           Required            |  Type   | Description                                                                                                                         |
| :---------------- | :---------------------------: | :-----: | :---------------------------------------------------------------------------------------------------------------------------------- |
| `cache_size`      | :negative_squared_cross_mark: | Integer | Max cache size to hold without sending requests to the server                                                                       |
| `block_size`      | :negative_squared_cross_mark: | Integer | Size of the blocks to be sent to the server                                                                                         |
| `threads`         | :negative_squared_cross_mark: | Integer | Number of threads that handle the async requests/replies                                                                            |
| `flush_threshold` | :negative_squared_cross_mark: |  Float  | Percentage of cache size. After this threshold have been reached, the client starts to send requests to the server (flush the data) |

#### RPC server configuration (`rpc_server`)
| Parameter             |           Required            |  Type   | Description                                     |
| :-------------------- | :---------------------------: | :-----: | :---------------------------------------------- |
| `server_address`      |      :white_check_mark:       | String  | Server's address (e.g., `0.0.0.0:50051`)        |
| `n_queues`            | :negative_squared_cross_mark: | Integer | Number of completion queues                     |
| `n_pollers_per_queue` | :negative_squared_cross_mark: | Integer | Number of threads polling each completion queue |

#### Data cache configuratio (`data_cache`)
| Parameter         |           Required            |  Type   | Description                                                                                                                             |
| :---------------- | :---------------------------: | :-----: | :-------------------------------------------------------------------------------------------------------------------------------------- |
| `size`            | :negative_squared_cross_mark: | Integer | Cache size (in bytes)                                                                                                                   |
| `block_size`      | :negative_squared_cross_mark: | Integer | Size of cached (in bytes)                                                                                                               |
| `time_out`        | :negative_squared_cross_mark: | Integer | Period that each block can be considered valid (in seconds)                                                                             |
| `eviction_policy` | :negative_squared_cross_mark: | String  | Avilable options: random (`rnd`), least recently used (`lru`). The algorithm that decides which element to evict when the cache is full |

#### Metadata cache configuration (`metadata_cache`)
| Parameter         |           Required            |  Type   | Description                                                                                                                             |
| :---------------- | :---------------------------: | :-----: | :-------------------------------------------------------------------------------------------------------------------------------------- |
| `size`            | :negative_squared_cross_mark: | Integer | Cache size (in bytes)                                                                                                                   |
| `time_out`        | :negative_squared_cross_mark: | Integer | Period that each metadata can be considered valid (in seconds)                                                                          |
| `eviction_policy` | :negative_squared_cross_mark: | String  | Avilable options: random (`rnd`), least recently used (`lru`). The algorithm that decides which element to evict when the cache is full |
 

#### Read ahead configuration (`read_ahead`)
| Parameter |           Required            |  Type   | Description                              |
| :-------- | :---------------------------: | :-----: | :--------------------------------------- |
| `size`    | :negative_squared_cross_mark: | Integer | Size of read ahead to be used (in bytes) |

#### Local configuration (`local`)
| Parameter |           Required            |  Type  | Description                                                                                |
| :-------- | :---------------------------: | :----: | :----------------------------------------------------------------------------------------- |
| `path`    |      :white_check_mark:       | String | Valid path to a directory to be exported to the clients                                    |
| `mode`    | :negative_squared_cross_mark: | String | Available options: mirrors the existing file system (`local`), NFS-like operations (`nfs`) |



<p align="right">(<a href="#top">back to top</a>)</p>



<!-- CONFIGURATION EXAMPLES -->
## Configuration examples 

In the folder `config_examples/`, there are various `.yml` files with examples of how layers and drivers can be stacked. The order of the layers is the order they appear in the configuration file. So, the first layer to appear in the configuration file will be the first layer in the stack.

As an example, the [sync_client.yml](config_examples/sync_client.yml) file has the following configuration:

```yml
rpc_client:
  server_address: localhost:50051
  mode: sync
metadata_cache:
  size: 16777216 # 16 MiB
  time_out: 30 # 30 seconds
  eviction_policy: rnd # random eviction
```

This example is a valid client configuration. The stack is composed of two layers. The first is an RPC client layer, and the other is a metadata caching layer. As explained above, the first layer (`rpc_client`) receives the server address that the client will use to communicate with and a mode that indicates that the client should use the synchronous mode to send requests to the server.


The [server.yml](config_examples/server.yml) file is a valid configuration for a server.

```yml
local:
  path: /tmp # exported directory
  mode: local # passthrough
rpc_server:
  server_address: 0.0.0.0:50051
  n_queues: 1
  n_pollers_per_queue: 4
```

The stack is composed of one layer in this configuration, named `local`. This local layer uses the [passthrough example](https://github.com/libfuse/libfuse/blob/master/example/passthrough.c) from the libfuse repository. This layer mirrors the existing file system, and it is implemented by just *passing through* all requests to the corresponding user-space libc functions. With this configuration, the server instance will export the `/tmp` directory to the clients. The `rpc_server` is not a layer but configured as one. It's like an engine that will send the requests to the following layers as the FUSE Library does in the clients.



<p align="right">(<a href="#top">back to top</a>)</p>


<!-- GETTING STARTED -->
## Getting Started

To deploy an RSafeFS instance, follow the next steps:

### Prerequisites

- A C++ compiler that supports the standard 20.

- CMake, at least version 3.22.

RSafeFS currently relies on this libraries:
- [libfuse (v2.9.9)](https://github.com/libfuse/libfuse.git) or [macFUSE (v4.2.4)](https://osxfuse.github.io)
- [grpc (v1.37.0)](https://github.com/grpc/grpc.git)
- [spdlog (v1.8.0)](https://github.com/gabime/spdlog.git)
- [fmt (v8.0.1)](https://github.com/fmtlib/fmt.git)
- [CLI11 (v2.1.2)](https://github.com/CLIUtils/CLI11.git)
- [yaml-cpp (v0.7.0)](https://github.com/jbeder/yaml-cpp.git)
- [asio (v1.22.1)](https://github.com/chriskohlhoff/asio.git)
- [googletest (v1.11.0)](https://github.com/google/googletest.git)


All, except libfuse, are installed at compile time. So, you only need to install libfuse before compiling RSafeFS.

On ubuntu, you can install the libfuse dependency using the following command:
```bash
apt install libfuse-dev 
```

And on macOS, if [homebrew](https://brew.sh) is available, use the following command:
```bash
brew install macfuse
```

### Installation

To install the RSafeFS, run the following commands:

1. Clone de repository
```bash
git clone https://github.com/diogolleitao/RSafeFS.git
```

2. Building
```bash
cmake -B RSafeFS/build -S RSafeFS
cmake --build RSafeFS/build 
```

3. Testing
```bash
ctest --test-dir RSafeFS/build/tests
```

If all the steps are successful, it should produce a binary named `rsafefs`. Next, there are some examples of how to use it.


<p align="right">(<a href="#top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

The `help` option shows the available options.

```
$ ./src/rsafefs --help
RSafeFS: A Modular File System for Remote Storage
Usage: ./src/rsafefs [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -c,--conf TEXT:FILE REQUIRED
                              configuration file
  -m,--mount TEXT:DIR         mount point (required only on clients)
  -d,--debug                  Show debug info
```

To run a client instance, compile the code as described in the previous section and run the following command: 

```bash
./rsafefs --conf=config_examples/sync_client.yml --mount=/mnt/pont
```

Where `/mnt/point` is the mount point for your instance of RSafeFS, as referred to in the help message, this option is required to run a client instance.
`config_examples/sync_client.yml` is the configuration file to be loaded.

You can deploy a server by specifying the following command:

```bash
./rsafefs --conf=config_examples/server.yml
```

As for the client, the server receives the configuration file in the `--conf` option.

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- ROADMAP -->
## Roadmap

- Layers
  - [X] RPC Layer (client and server)
  - [X] Read ahead
  - [X] Data cache
  - [X] Metadata cache
  - [ ] Compression
  - [ ] Encryption
  - [ ] Distributed layer (to create a distributed file system)
- Drivers
  - Caches (data and metadata) eviction policies
    - [X] LRU - Least Recently Used
    - [X] RND - Random 
    - [ ] LFU
  - RPC Frameworks
    - [X] gRPC
    - [ ] other RPC libraries (Cap'n Proto, Thirft, ...)

See the [open issues](https://github.com/diogolleitao/RSafeFS/issues) for a complete list of proposed features (and known issues).

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- CONTRIBUTING -->
## Contributing

Contributions make the open-source community a fantastic place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion to improve this, please fork the repo and create a pull request. You can also open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/amazing-feature`)
3. Commit your Changes (`git commit -m 'Add some amazing-feature'`)
4. Push to the Branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- LICENSE -->
## License

Distributed under the GPL v2 License. See [LICENSE](LICENSE) for more information.

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Diogo Leitão - diogolleitao@gmail.com

Project Link: [https://github.com/diogolleitao/RSafeFS](https://github.com/diogolleitao/RSafeFS)

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/diogolleitao/RSafeFS.svg?style=for-the-badge
[contributors-url]: https://github.com/diogolleitao/RSafeFS/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/diogolleitao/RSafeFS.svg?style=for-the-badge
[forks-url]: https://github.com/diogolleitao/RSafeFS/network/members
[stars-shield]: https://img.shields.io/github/stars/diogolleitao/RSafeFS.svg?style=for-the-badge
[stars-url]: https://github.com/diogolleitao/RSafeFS/stargazers
[issues-shield]: https://img.shields.io/github/issues/diogolleitao/RSafeFS.svg?style=for-the-badge
[issues-url]: https://github.com/diogolleitao/RSafeFS/issues
[license-shield]: https://img.shields.io/github/license/diogolleitao/RSafeFS.svg?style=for-the-badge
[license-url]: https://github.com/diogolleitao/RSafeFS/blob/main/LICENSE
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/diogolleitao
[product-screenshot]: images/screenshot.png
